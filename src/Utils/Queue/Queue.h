#ifndef WD_QUEUE_H
#define WD_QUEUE_H

namespace Wado::Queue {
    template <typename T>
    class Queue {
        public:
            class Node;
            
            using Item = struct Item {
                Node volatile* node;
                T* data;
            };

            using Node = struct Node {
                Node volatile* next;
                Item* item;
            };

            virtual void enqueue(Item* data) = 0;
            virtual Item* dequeue() = 0;
            virtual bool isEmpty() const = 0;
            virtual bool isFull() const = 0;
    }; 

};

#endif
