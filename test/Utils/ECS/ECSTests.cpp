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

TEST(ComponentTest, CanAddMultipleComponents) {
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
    db.addComponent<int>(testID);
    found = db.hasComponent<int>(testID);
    EXPECT_TRUE(found) << "Expected entity to have second component, but it doesn't";
};

TEST(ComponentTest, CanSetComponentValue) {
    using namespace Wado::ECS;
    using Position = struct Position {
            float x;
            float y;
    };
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Position>(testID);
    bool found = db.hasComponent<Position>(testID);
    ASSERT_TRUE(found) << "Expected entity to have component, but it doesn't";
    Position pos = {1.0, 2.0};
    db.setComponentCopy<Position>(testID, pos);
    const Position& dbPos = db.getComponent<Position>(testID);
    EXPECT_EQ(dbPos.x, pos.x) << "Expected x component of position to be equal, but it isn't";
    EXPECT_EQ(dbPos.y, pos.y) << "Expected y component of position to be equal, but it isn't";    
};

TEST(ComponentTest, ComponentValueSurvivesTableMove) {
    using namespace Wado::ECS;
    using Position = struct Position {
            float x;
            float y;
    };
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Position>(testID);
    bool found = db.hasComponent<Position>(testID);
    ASSERT_TRUE(found) << "Expected entity to have component, but it doesn't";
    Position pos = {1.0, 2.0};
    db.setComponentCopy<Position>(testID, pos);
    const Position& dbPos = db.getComponent<Position>(testID);
    ASSERT_EQ(dbPos.x, pos.x) << "Expected x component of position to be equal, but it isn't";
    ASSERT_EQ(dbPos.y, pos.y) << "Expected y component of position to be equal, but it isn't";  
    db.addComponent<int>(testID);
    int x = 5;
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity to have int id";
    ASSERT_TRUE(db.hasComponent<Position>(testID)) << "Expected entity to still have position";
    const Position& dbPosAfter = db.getComponent<Position>(testID);
    ASSERT_EQ(dbPosAfter.x, pos.x) << "Expected x component of position to be equal, but it isn't";
    ASSERT_EQ(dbPosAfter.y, pos.y) << "Expected y component of position to be equal, but it isn't";  
};
