#ifndef WD_LOCK_FREE_QUEUE_H
#define WD_LOCK_FREE_QUEUE_H

#include "Queue.h"

#include <stdexcept>

#include <windows.h>

namespace Wado::Queue {

    template <typename T>
    class LockFreeQueue : public Queue<T> {
        public:
            LockFreeQueue(const size_t coreCount) : _coreCount(coreCount) {
                _linkedList.head = static_cast<Queue<T>::Node *>(malloc(sizeof(Queue<T>::Node)));
                if (_linkedList.head == nullptr) {
                    throw std::runtime_error("Could not allocate head element for lock free queue");
                };

                _linkedList.head->next = nullptr;
                _linkedList.head->item = nullptr; // has no item since it's mereley a bookkeeping element 

                _linkedList.tail = _linkedList.head;

                _coreData = static_cast<CoreData *>(calloc(_coreCount, sizeof(CoreData)));

                if (_coreData == nullptr) {
                    throw std::runtime_error("Could not allocate memory for the core data structs in the lock free queue.");
                };

                for (size_t i = 0; i < _coreCount; i++) {
                    new (_coreData + i) CoreData();
                };
            };

            ~LockFreeQueue() noexcept {
                free(_coreData);
            };

            void enqueue(Queue<T>::Item* item) override {
                //std::cout << "Incoming item is: " << item.data << std::endl;
                Queue<T>::Node* node = acquire(item); // if we are re-inserting item, this will reuse a previousl allocated block
                if (node == nullptr) {
                    // if not, allocate it
                    //std::cout << "Had to allocate node" << std::endl;
                    node = static_cast<Queue<T>::Node *>(malloc(sizeof(Queue<T>::Node)));
                };
                size_t coreNumber = (size_t) TlsGetValue(coreNumberTlsIndex);
                //std::cout << "Running on core: " << coreNumber << std::endl;
                node->item = item;
                node->next = nullptr;
                //std::cout << "Node item is: " << node->item->data << std::endl;
                Queue<T>::Node volatile* tempTail = nullptr;
                do {
                    do {
                        tempTail = _linkedList.tail; // get main queue tail
                        (_coreData + coreNumber)->hazardPointer1 = tempTail; // we want to modify the tail so its a hazard for this current processor 
                    } while (tempTail != _linkedList.tail); // when there haven't been any changes and we manage to get the local tail to be the same as the main queue 
                    //std::cout << "Tails matched" << std::endl;
                    Queue<T>::Node volatile* next = tempTail->next; // What if temptail stays null pointer? I need to guard against this 
                    //std::cout << "Found temptail next" << std::endl;
                    // Get the elemtn right after the tail now 
                    if (next != nullptr) { // something else has been added after the tail after we synched the local tail with the global tail
                        // If a new element was added but the tail is laggin behind, catch it up. make the next element the new tail
                        // as long as tail is what we expect it to be locally
                        InterlockedCompareExchangePointer((PVOID volatile *) &_linkedList.tail, (PVOID) next, (PVOID) tempTail);
                        continue; // then move on to next iteration and try again from the beginning with the caught up tail
                    };
                } while (InterlockedCompareExchangePointer((PVOID volatile *) &tempTail->next, (PVOID) node, 0) != 0); // keep catching up and try to update tail until we can set our node as the next tail (i.e there are no ohter elements being inserted after it)
                // at this point, the node should be the tail. 
                // catch up queue 
                InterlockedCompareExchangePointer((PVOID volatile *) &_linkedList.tail, (PVOID) node, (PVOID) tempTail); // In this scenario nothing happened after we acquired temp tail 
                (_coreData + coreNumber)->hazardPointer1 = nullptr; // temp tail is no longer hazardous
            };

            Queue<T>::Item* dequeue() override {
                Queue<T>::Item* item;
                size_t coreNumber = reinterpret_cast<size_t>(TlsGetValue(coreNumberTlsIndex));
                Queue<T>::Node volatile* tempHead = nullptr;
                Queue<T>::Node volatile* next = nullptr;
                
                do {
                    do {
                        tempHead = _linkedList.head; // try to get head of queue
                        (_coreData + coreNumber)->hazardPointer1 = tempHead; // this is hazardous, dont want other cores to reallocate it now 
                    } while (tempHead != _linkedList.head); // same as above, grab a stable heaad position 
                    do {
                        next = tempHead->next;
                        (_coreData + coreNumber)->hazardPointer2 = tempHead; // this is hazardous, dont want other cores to reallocate it now 
                    } while (next != tempHead->next); // Same as above, iterate until we acquire a stable actual first element in the queue
                    if (next == nullptr) {
                        // In this case the first actual element is null, so the queue also has nothing 
                        (_coreData + coreNumber)->hazardPointer1 = nullptr;
                        (_coreData + coreNumber)->hazardPointer2 = nullptr; 
                        return nullptr;
                    };
                    // there is an element in the queue 
                    // if tail == head, replace tail with next pointer (catch up global value now)
                    InterlockedCompareExchangePointer((PVOID volatile *) &_linkedList.tail, (PVOID) next, (PVOID) tempHead);
                    item = next->item;
                } while (InterlockedCompareExchangePointer((PVOID volatile *) &_linkedList.head, (PVOID) next, (PVOID) tempHead) != tempHead); // keep doing this until we are in a position where the linked lists's head can be replaced with the element after head
                // No more hazard pointers on this core 
                (_coreData + coreNumber)->hazardPointer1 = nullptr;
                (_coreData + coreNumber)->hazardPointer2 = nullptr; 
                item->node = tempHead; // list basically moved up so this node can be used now 
                return item;
            };

            void release(Queue<T>::Item* item) noexcept {
                // Get the node pointer that can be used for this item 
                Queue<T>::Node* node = acquire(item);
                if (node != nullptr) {
                    // this means this node pointer has no conflicts with anyhing, can be safely removed.
                    free(node);
                };
            };

            bool isEmpty() const override {
                return _linkedList.tail == nullptr;
            };

            bool isFull() const override {
                return false;
            };

        private:
            using LinkedListQueue = struct LinkedListQueue {
                Node volatile* head; //dummy node, used as an "end" pointer
                Node volatile* tail; //actual tail node, can lag behind and enqueue/dequeue will catch it up 
            };

            using CoreData = struct CoreData { // per-core data 
                Node volatile* hazardPointer1; // these nodes could be used by the core currently while they are not in the queue 
                Node volatile* hazardPointer2;
                Node volatile* pooledNode1; // these are safe to use, reusable nodes
                Node volatile* pooledNode2;
            };

            const size_t _coreCount;
            CoreData* _coreData;

            LinkedListQueue _linkedList;

            // This function tries to acquire 
            // space for this item. If it's a new item,
            // new memory will be allocated. 

            // If this was part of the queue at some point, memory will 
            // be attempted to be reused. 
            Queue<T>::Node* acquire(Queue<T>::Item* item) noexcept {
                Queue<T>::Node volatile* tempNode = item->node;
                bool swapped = false;
                do {
                    swapped = false;
                    for (size_t i = 0; i < _coreCount; ++i) {
                        if (tempNode == NULL) {
                            break; // as soon as the node becomes null it means there is no suitable node to reuse
                        };
                        if (tempNode == (_coreData + i)->hazardPointer1) {
                            // Memory currently used by the item conflicts with some core.
                            // Swap its memory with the core's pool
                            tempNode = static_cast<Node *>(InterlockedExchangePointer( (PVOID volatile *) &((_coreData + i)->pooledNode1), (PVOID) tempNode));
                            swapped = true;
                        } else if (tempNode == (_coreData + i)->hazardPointer2) {
                            tempNode = static_cast<Node *>(InterlockedExchangePointer( (PVOID volatile *) &((_coreData + i)->pooledNode2), (PVOID) tempNode));
                            swapped = true;
                        };
                    };
                } while (swapped);
                // Once we've gone through all processors without any swaps, there's no more conflicts anymore. 
                return (Queue<T>::Node *) tempNode;
            };
    };

};

#endif