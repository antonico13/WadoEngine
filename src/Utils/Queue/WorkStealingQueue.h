#ifndef WD_WORK_STEALING_QUEUE_H
#define WD_WORK_STEALING_QUEUE_H

#include "Atomics.h"
#include "CircularArray.h"

#include <optional>

// Based on: Dynamic Circular Work-Stealing Deque [Lev, Chase 2005] 

namespace Wado::Queue {

    // LIFO array queue 
    template <typename T, size_t N>
    class WorkStealingQueue {
        public:
            WorkStealingQueue() noexcept { };
            ~WorkStealingQueue() noexcept { };

            bool enqueue(T item) noexcept {
                // Get local version of top
                // Only one core can every modify bottom so this is fine 
                size_t t = top;
                size_t size = bottom - t;
                // This is the size when top was read 
                if (size >= array.size()) {
                    // The queue is currently full
                    return false;
                } else {
                    array.insert(bottom, item);
                    ++bottom;
                    return true;
                };
            };

            std::optional<T> dequeue() noexcept {
                size_t b = bottom;
                --b;
                bottom = b; // ???
                size_t t = top;
                size_t size = b - t;

                if (size < 0) {
                    // This means that something else got the singular bottom element
                    // while we were updating and the queue should now be empty 
                    bottom = t;
                    return std::nullopt;
                };

                T item = array.get(b);

                if (size > 0) {
                    return item;
                };

                // Need to make top and bottom the same thing now since the size is 0 
                bottom = t + 1;
                // Something else popped while we were trying to obtain the element, so 
                // it's no longer valid 
                if (Atomics::CompareAndSwap(&top, t + 1, t) != t) {
                    return std::nullopt;
                };
                return item;
            };

            std::optional<T> steal() noexcept {
                // Get local versions of these as both could be modified 
                // by the other core at this point 
                size_t t;
                size_t b;
                T item;

                // Need to increase the top element to ensure we can steal something
                // but also to return in case the queue ever becomes empty while we try to work 
                do {
                    // Acquire values
                    t = top;
                    b = bottom;
                    if ((b - t) <= 0) {
                        return std::nullopt;
                    };
                    // Need to get item before CAS, because we are using a circular array 
                    item = array.get(t);
                } while (Atomics::CompareAndSwap(&top, t + 1, t) != t);

                // At this point, we managed to move top up and the queue was never empty
                return item;
            };

            size_t size() {
                return (bottom - top);
            };
            
            bool isEmpty() {
                return bottom == top;
            };

            bool isFull() {
                return (bottom - top) == array.size();
            };
            
            
        private:
            CircularArray<T, N> array;

            volatile size_t bottom = 0; 
            volatile size_t top = 0;
    };
};
#endif
