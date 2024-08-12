#include <gtest/gtest.h>
#include "CircularArray.h"

using namespace Wado;

TEST(CircularArrayTest, SizeIsReturnedCorrectly) {
    CircularArray<int, 20> array;
    ASSERT_EQ(array.size(), 20) << "Expected array to have correct size";
};

TEST(CircularArrayTest, CanInsertElements) {
    CircularArray<int, 20> array;
    array.insert(10, 2);
    ASSERT_EQ(array.get(10), 2) << "Expected element to have been inserted at correct position";
};

TEST(CircularArrayTest, IndexIsCalcuatedCorrectlyForInsertions) {
    CircularArray<int, 20> array;
    array.insert(10, 2);
    ASSERT_EQ(array.get(10), 2) << "Expected element to have been inserted at correct position";
    array.insert(30, 3);
    ASSERT_EQ(array.get(10), 3) << "Expected element to have been inserted at correct modulo position";
};


TEST(CircularArrayTest, IndexIsCalcuatedCorrectlyForGet) {
    CircularArray<int, 20> array;
    array.insert(10, 2);
    ASSERT_EQ(array.get(10), 2) << "Expected element to have been inserted at correct position";
    ASSERT_EQ(array.get(30), 2) << "Expected element to be the same for the same modulo index";
};

