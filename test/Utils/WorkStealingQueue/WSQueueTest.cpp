#include <gtest/gtest.h>
#include "WorkStealingQueue.h"

#include <thread>
#include <unordered_set>

using namespace Wado::Queue;

#define DEFAULT_SIZE 20

using TestStruct = struct TestStruct {
    void *data;
    int x;
    int y;
    float z;
    size_t size;
};

TEST(WorkStealingQueueTest, QueueIsInitiallyEmpty) {
    using namespace Wado::Queue;
    WorkStealingQueue<TestStruct, DEFAULT_SIZE> newQueue = WorkStealingQueue<TestStruct, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default";
};

TEST(WorkStealingQueueTest, AddingToQueueMakesItNonEmpty) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(5);
    ASSERT_FALSE(newQueue.isEmpty()) << "Expected queue to not be empty anymore after adding element";
};

TEST(WorkStealingQueueTest, AddingToQueueMakesItNonFull) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(5);
    ASSERT_FALSE(newQueue.isFull()) << "Expected queue to not be empty anymore after adding element";
};

TEST(WorkStealingQueueTest, AddingQueueSizeElementsMakesItFull) {
    using namespace Wado::Queue;
    WorkStealingQueue<TestStruct, DEFAULT_SIZE> newQueue = WorkStealingQueue<TestStruct, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct{nullptr, 2, 5, 3.0, 12};
    for (int i = 0; i < DEFAULT_SIZE; ++i) {
        newQueue.enqueue(newStruct); 
    };
    ASSERT_TRUE(newQueue.isFull()) << "Expected queue to be full after adding  elements";
};

TEST(WorkStealingQueueTest, AddingToFullQueueIsNotPossible) {
    using namespace Wado::Queue;
    WorkStealingQueue<TestStruct, DEFAULT_SIZE> newQueue = WorkStealingQueue<TestStruct, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct{nullptr, 2, 5, 3.0, 12};
    for (int i = 0; i < DEFAULT_SIZE; ++i) {
        newQueue.enqueue(newStruct); 
    };
    ASSERT_TRUE(newQueue.isFull()) << "Expected queue to be full after adding  elements";

    ASSERT_FALSE(newQueue.enqueue(newStruct)) << "Expected full queue to reject new elements";
};

TEST(WorkStealingQueueTest, CanDequeueElement) {
    using namespace Wado::Queue;
    WorkStealingQueue<float, DEFAULT_SIZE> newQueue = WorkStealingQueue<float, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(6.0);
    std::optional<float> elem = newQueue.dequeue();
    ASSERT_TRUE(elem.has_value()) << "Expected dequeued element to have value";
    ASSERT_EQ(6.0, newQueue.dequeue().value()) << "Expected the added pointer to be returned";
};

TEST(WorkStealingQueueTest, EmptyAfterDequeue) {
    using namespace Wado::Queue;
    WorkStealingQueue<TestStruct, DEFAULT_SIZE> newQueue = WorkStealingQueue<TestStruct, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    TestStruct newStruct{nullptr, 2, 5, 3.0, 12};
    newQueue.enqueue(newStruct);
    newQueue.dequeue();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected queue to be empty after dequeue";
};

TEST(WorkStealingQueueTest, DequeueIsPerformedInLIFOOrder) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(5);
    newQueue.enqueue(6);
    ASSERT_EQ(6, newQueue.dequeue().value()) << "Expected the second int to be dequeued first";
    ASSERT_EQ(5, newQueue.dequeue().value()) << "Expected the first int to be dequeued second";
};

TEST(WorkStealingQueueTest, CanStealFromQueue) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(5);
    newQueue.enqueue(6);
    ASSERT_TRUE(newQueue.steal().has_value()) << "Expected stolen item to exist";
};

TEST(WorkStealingQueueTest, StealsFromQueueInFIFOOrder) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(5);
    newQueue.enqueue(6);
    ASSERT_EQ(5, newQueue.steal().value()) << "Expected the first int to be stolen first";
    ASSERT_EQ(6, newQueue.steal().value()) << "Expected the second int to be stolen second";
};

TEST(WorkStealingQueueTest, StealingFromQueueDecreasesSize) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    newQueue.enqueue(5);
    newQueue.enqueue(6);
    newQueue.steal();
    EXPECT_EQ(1, newQueue.size()) << "Expected the first steal to decrease size";
    newQueue.steal();
    EXPECT_TRUE(newQueue.isEmpty()) << "Expected the second steal to clear the queue";
};

TEST(WorkStealingQueueTest, StealingFromEmptyQueueReturnsNothing) {
    using namespace Wado::Queue;
    WorkStealingQueue<int, DEFAULT_SIZE> newQueue = WorkStealingQueue<int, DEFAULT_SIZE>();
    ASSERT_TRUE(newQueue.isEmpty()) << "Expected new queue to be empty by default before adding anything";
    EXPECT_FALSE(newQueue.steal().has_value()) << "Expected steal to be empty";
    EXPECT_TRUE(newQueue.isEmpty()) << "Expected the steal to not modify size";
};


TEST(WorkStealingQueueTest, MultithreadedStealing) {
    std::unordered_set<int> testSet;

    for (int i = 0; i < DEFAULT_SIZE * 5; ++i) {
        testSet.insert(i);
    };
    WorkStealingQueue<int, DEFAULT_SIZE * 5> testQueue = WorkStealingQueue<int, DEFAULT_SIZE * 5>();

    std::unordered_set<int> firstSet;
    std::unordered_set<int> secondSet;

    auto f1 = [&firstSet, &testQueue]() {
        for (int i = 0; i < DEFAULT_SIZE * 5; ++i) {
            testQueue.enqueue(i);
        };

        while (!testQueue.isEmpty()) {
            // Processing bit 
            std::optional<int> elem = testQueue.dequeue();
            if (elem.has_value()) {
                firstSet.insert(elem.value());
            };
        };
    };

    auto f2 = [&secondSet, &testQueue]() {
        while (!testQueue.isEmpty()) {
            // Processing bit 
            std::optional<int> elem = testQueue.steal();
            if (elem.has_value()) {
                secondSet.insert(elem.value());
            };
        };
    };


    std::thread firstThread(f1);
    std::thread secondThread(f2);

    firstThread.join();
    secondThread.join();

    std::cout << "First set:" << std::endl;
    for (const int elem : firstSet) {
        std::cout << elem << " ";
    };
    std::cout << std::endl;

    std::cout << "Second set:" << std::endl;
    for (const int elem : secondSet) {
        std::cout << elem << " ";
    };
    std::cout << std::endl;

    firstSet.insert(secondSet.begin(), secondSet.end());

    ASSERT_EQ(firstSet, testSet) << "Expected all elements to remain";
};