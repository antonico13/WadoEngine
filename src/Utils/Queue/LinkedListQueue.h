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

                void enqueue(Queue<T>::Item* data) override {
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

                Queue<T>::Item* dequeue() noexcept override {
                    if (head == nullptr) {
                        return nullptr;
                    };
                    Queue<T>::Item* data = head->elementData;
                    Node* temp = head;
                    head = head->nextNode;
                    free(temp); // TODO: reclaim nodes here 
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
                        Queue<T>::Item* elementData;
                        Node* nextNode;
                };

                Node* head;
                Node* tail;
                size_t size;
        }; 

};

#endif