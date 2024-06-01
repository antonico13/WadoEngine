#ifndef WD_LINKED_QUEUE_H
#define WD_LINKED_QUEUE_H

#include "Queue.h"

#include <stdexcept>

namespace Wado::Queue {
    // FIFO queue
    template <typename T>
    class LinkedListQueue : public Queue<T> {
            public:
                LinkedListQueue() noexcept {
                    head = (tail = nullptr);
                    size = 0;
                };

                ~LinkedListQueue() noexcept {
                    Node* prev = nullptr;
                    while (head != nullptr) {
                        prev = head;
                        head = prev->nextNode;
                        free(prev);
                    };
                };

                void enqueue(T* data) override {
                    Node* newNode = static_cast<Node*>(malloc(sizeof(Node)));
                    if (newNode == nullptr) {
                        throw std::runtime_error("Could not allocate new node for linked list queue");
                    };

                    newNode->elementData = data;
                    newNode->nextNode = nullptr;
                    if (head == nullptr) {
                        head = newNode;
                        tail = newNode;
                    } else {
                        tail->nextNode = newNode;
                        tail = newNode;
                    };
                    ++size;
                };

                // Assuming non empy 
                T* dequeue() noexcept override {
                    T* data = head->elementData;
                    head = head->nextNode;
                    --size;
                    return data;
                };

                bool isEmpty() const noexcept override {
                    return size == 0;
                };

                bool isFull() const noexcept override {
                    return false;
                };
            private:
                class Node {
                    public:
                        T* elementData;
                        Node* nextNode;
                };

                Node* head;
                Node* tail;
                size_t size;
        }; 

};

#endif