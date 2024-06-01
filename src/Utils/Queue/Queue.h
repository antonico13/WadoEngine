#ifndef WD_QUEUE_H
#define WD_QUEUE_H

namespace Wado::Queue {

    template <typename T>
    class Queue {
        public:
            virtual void enqueue(T* data) = 0;
            virtual T* dequeue() = 0;
            virtual bool isEmpty() const = 0;
            virtual bool isFull() const = 0;
    }; 

};

#endif
