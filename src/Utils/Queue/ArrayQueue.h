#ifndef WD_ARRAY_QUEUE_H
#define WD_ARRAY_QUEUE_H

#include "Queue.h"

#include <stdexcept>

namespace Wado::Queue {

    // LIFO array queue 
    template <typename T>
    class ArrayQueue : public Queue<T> {
        public:
            ArrayQueue(const size_t count) : _count(count) {
                _data = static_cast<Queue<T>::Item**>(malloc(sizeof(Queue<T>::Item*) * _count));

                if (_data == nullptr) {
                    throw std::runtime_error("Could not allocate array memory for array queue");
                };

                _head = 0;
            };

            ~ArrayQueue() {
                free(_data);
            };

            // Pre: only called when queue is not full 
            void enqueue(Queue<T>::Item* elementData) noexcept override {
                *(_data + _head) = elementData;
                ++_head;
            };
            
            // Pre: can only be called when queue is not empty 
            Queue<T>::Item* dequeue() noexcept override {
                if (_head == 0) {
                    return nullptr;
                };
                --_head;
                return *(_data + _head);
            };

            bool isEmpty() const noexcept override {
                return _head == 0; 
            };

            bool isFull() const noexcept override {
                return _head == _count;
            };
        private:
            Queue<T>::Item** _data;
            size_t _head;
            const size_t _count;
    };
};
#endif
