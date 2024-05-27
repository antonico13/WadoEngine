#include <gtest/gtest.h>
#include "ECS.h"
#include "ECS.cpp"
#include <iostream>


TEST(EntityIDTest, CorrectFirstID) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID id = db.createEntityID();
    ASSERT_EQ(id, (uint64_t)1 << 31) << "Entity ID does not start from the correct place";
};

TEST(EntityIDTest, EntityIDIncreases) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID id = db.createEntityID();
    EntityID id2 = db.createEntityID();
    EntityID expectedID = ((uint64_t)1 << 31) + 1;
    ASSERT_EQ(id2, expectedID) << "Entity ID does not increase correctly, expected " << expectedID << ", but got " << id2;
};

TEST(EntityIDTest, EntityIDsAreReused) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID id = db.createEntityID();
    EntityID id2 = db.createEntityID();
    EntityID expectedID = ((uint64_t)1 << 31) + 1;
    db.destroyEntity(id2);
    EntityID id3 = db.createEntityID();
    ASSERT_EQ(id3, expectedID) << "Entity ID does not get reused correctly, expected " << expectedID << ", but got " << id3;
    db.destroyEntity(id);
    EntityID expectedID2 = (uint64_t)1 << 31;
    EntityID id4 = db.createEntityID();
    ASSERT_EQ(id4, expectedID2) << "Entity ID does not get reused correctly, expected " << expectedID2 << ", but got " << id4;
};

TEST(ComponentTest, ComponentsGetAdded) {
    using namespace Wado::ECS;
    using Position = struct Position {
            float x;
            float y;
    };
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Position>(testID);
    bool found = db.hasComponent<Position>(testID);
    EXPECT_TRUE(found) << "Expected entity to have component, but it doesn't";
};

