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

TEST(ComponentTest, CanSetMultipleComponentValues) {
    using namespace Wado::ECS;
    using Position = struct Position {
            float x;
            float y;
    };
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Position>(testID);
    Position pos = {1.0, 2.0};
    db.setComponentCopy<Position>(testID, pos);
    db.addComponent<int>(testID);
    int x = 5;
    db.setComponentCopy<int>(testID, x);
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity to have int id";
    ASSERT_TRUE(db.hasComponent<Position>(testID)) << "Expected entity to still have position";
    const Position& dbPos = db.getComponent<Position>(testID);
    ASSERT_EQ(dbPos.x, pos.x) << "Expected x component of position to be equal, but it isn't";
    ASSERT_EQ(dbPos.y, pos.y) << "Expected y component of position to be equal, but it isn't";  
    const int& dbX = db.getComponent<int>(testID);
    ASSERT_EQ(dbX, x) << "Expected int component to be equal, but it isn't";
};

TEST(ComponentTest, CanRemoveComponents) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<int>(testID);
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity to have int id";
    db.removeComponent<int>(testID);
    ASSERT_FALSE(db.hasComponent<int>(testID)) << "Exepected entity not to have the int component after removing";   
};

TEST(ComponentTest, CanCreateNewTableCorrectly) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<int>(testID);
    db.addComponent<float>(testID);
    db.addComponent<char>(testID);
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity to have int id";
    ASSERT_TRUE(db.hasComponent<float>(testID)) << "Exepected entity to have float id";   
    ASSERT_TRUE(db.hasComponent<char>(testID)) << "Exepected entity not to have char id";   
    db.removeComponent<float>(testID);
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity to have int id";
    ASSERT_FALSE(db.hasComponent<float>(testID)) << "Exepected entity not to have float id";   
    ASSERT_TRUE(db.hasComponent<char>(testID)) << "Exepected entity not to have char id";   
};

TEST(ComponentTest, CanSetComponentMove) {
    using namespace Wado::ECS;
    using Position = struct Position {
            float x;
            float y;
    };
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Position>(testID);
    ASSERT_TRUE(db.hasComponent<Position>(testID)) << "Expected entity to have component, but it doesn't";
    Position pos = {1.0, 2.0};
    db.setComponentMove<Position>(testID, pos);
    const Position& dbPos = db.getComponent<Position>(testID);
    EXPECT_EQ(dbPos.x, 1.0) << "Expected x component of position to be equal, but it isn't";
    EXPECT_EQ(dbPos.y, 2.0) << "Expected y component of position to be equal, but it isn't";
};

TEST(ComponentTest, CanSetComponentMoveVector) {
    using namespace Wado::ECS;
    using Vec = std::vector<int>;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Vec>(testID);
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have component, but it doesn't";
    Vec vec({1, 2, 3});
    db.setComponentMove<Vec>(testID, vec);
    const Vec& dbVec = db.getComponent<Vec>(testID);
    EXPECT_EQ(dbVec[0], 1) << "Expected first component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[1], 2) << "Expected second component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[2], 3) << "Expected third component of position to be equal, but it isn't";
    EXPECT_EQ(vec.size(), 0) << "Expected original vector not to have any components";
};

TEST(ComponentTest, CanSetComponentCopyVector) {
    using namespace Wado::ECS;
    using Vec = std::vector<int>;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Vec>(testID);
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have component, but it doesn't";
    Vec vec({1, 2, 3});
    db.setComponentCopy<Vec>(testID, vec);
    const Vec& dbVec = db.getComponent<Vec>(testID);
    EXPECT_EQ(dbVec[0], vec[0]) << "Expected first component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[1], vec[1]) << "Expected second component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[2], vec[2]) << "Expected third component of position to be equal, but it isn't";
    EXPECT_EQ(vec.size(), 3) << "Expected original vector to keep all components";
};
