#include <gtest/gtest.h>
#include "ArrayQueue.h"
#include "LinkedListQueue.h"

#define QUEUE_SIZE 20

using TestStruct = struct TestStruct {
    void *data;
    int x;
    int y;
    float z;
    size_t size;
};

TEST(ArrayQueueTest, QueueIsInitiallyEmpty) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default";
};

TEST(ArrayQueueTest, AddingToQueueMakesItNonEmpty) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    ASSERT_FALSE(newQueue.isEmpty()) << "Expected queue to not be empty anymore after adding element";
};

TEST(ArrayQueueTest, AddingToQueueMakesItNonFull) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    ASSERT_FALSE(newQueue.isFull()) << "Expected queue to not be empty anymore after adding element";
};

TEST(ArrayQueueTest, AddingQueueSizeElementsMakesItFull) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    for (int i = 0; i < QUEUE_SIZE; ++i) {
        newQueue.enqueue(&newStruct); 
    };
    ASSERT_TRUE(newQueue.isFull()) << "Expected queue to be full after adding QUEUE_SIZE elements";
};

TEST(ArrayQueueTest, CanDequeueElement) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    ASSERT_EQ(&newStruct, newQueue.dequeue()) << "Expected the added pointer to be returned";
};

TEST(ArrayQueueTest, EmptyAfterDequeue) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    newQueue.dequeue();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected queue to be empty after dequeue";
};

TEST(ArrayQueueTest, DequeueIsPerformedInLIFOOrder) {
    using namespace Wado::Queue;
    ArrayQueue<TestStruct> newQueue = ArrayQueue<TestStruct>(QUEUE_SIZE);
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    TestStruct newStruct2;
    newQueue.enqueue(&newStruct);
    newQueue.enqueue(&newStruct2);
    ASSERT_EQ(&newStruct2, newQueue.dequeue()) << "Expected the second pointer to be dequeued first";
    ASSERT_EQ(&newStruct, newQueue.dequeue()) << "Expected the first pointer to be dequeued second";
};





TEST(LinkedQueueTest, QueueIsInitiallyEmpty) {
    using namespace Wado::Queue;
    LinkedListQueue<TestStruct> newQueue = LinkedListQueue<TestStruct>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default";
};

TEST(LinkedQueueTest, AddingToQueueMakesItNonEmpty) {
    using namespace Wado::Queue;
    LinkedListQueue<TestStruct> newQueue = LinkedListQueue<TestStruct>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    ASSERT_FALSE(newQueue.isEmpty()) << "Expected queue to not be empty anymore after adding element";
};

TEST(LinkedQueueTest, AddingToQueueMakesItNonFull) {
    using namespace Wado::Queue;
    LinkedListQueue<TestStruct> newQueue = LinkedListQueue<TestStruct>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    ASSERT_FALSE(newQueue.isFull()) << "Expected queue to not be empty anymore after adding element";
};

TEST(LinkedQueueTest, CanDequeueElement) {
    using namespace Wado::Queue;
    LinkedListQueue<TestStruct> newQueue = LinkedListQueue<TestStruct>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    ASSERT_EQ(&newStruct, newQueue.dequeue()) << "Expected the added pointer to be returned";
};

TEST(LinkedQueueTest, EmptyAfterDequeue) {
    using namespace Wado::Queue;
    LinkedListQueue<TestStruct> newQueue = LinkedListQueue<TestStruct>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    newQueue.enqueue(&newStruct);
    newQueue.dequeue();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected queue to be empty after dequeue";
};

TEST(LinkedQueueTest, DequeueIsPerformedInFIFOOrder) {
    using namespace Wado::Queue;
    LinkedListQueue<TestStruct> newQueue = LinkedListQueue<TestStruct>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct;
    TestStruct newStruct2;
    newQueue.enqueue(&newStruct);
    newQueue.enqueue(&newStruct2);
    ASSERT_EQ(&newStruct, newQueue.dequeue()) << "Expected the second pointer to be dequeued first";
    ASSERT_EQ(&newStruct2, newQueue.dequeue()) << "Expected the first pointer to be dequeued second";
};