#include <gtest/gtest.h>
#include "ECS.h"
#include <iostream>

using Position = struct Position {
            float x;
            float y;
};

using Map = std::map<int, int>;
using Vec = std::vector<int>;

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
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Position>(testID);
    bool found = db.hasComponent<Position>(testID);
    EXPECT_TRUE(found) << "Expected entity to have component, but it doesn't";
};

TEST(ComponentTest, CanAddMultipleComponents) {
    using namespace Wado::ECS;
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

    // Tables should exist now.
    EntityID testID2 = db.createEntityID();
    db.addComponent<int>(testID2);
    db.addComponent<float>(testID2);
    db.addComponent<char>(testID2);

    db.removeComponent<float>(testID2);
};

TEST(ComponentTest, CanSetComponentMove) {
    using namespace Wado::ECS;
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

TEST(ComponentTest, ComponentCopiesDoNotOverlap) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    db.addComponent<Vec>(testID);
    db.addComponent<Vec>(testID2);
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have component, but it doesn't";
    ASSERT_TRUE(db.hasComponent<Vec>(testID2)) << "Expected entity to have component, but it doesn't";
    Vec vec({1, 2, 3});
    db.setComponentCopy<Vec>(testID, vec);
    Vec vec2({5, 6, 7});
    db.setComponentCopy<Vec>(testID2, vec2);
    const Vec& dbVec = db.getComponent<Vec>(testID);
    EXPECT_EQ(dbVec[0], vec[0]) << "Expected first component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[1], vec[1]) << "Expected second component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[2], vec[2]) << "Expected third component of position to be equal, but it isn't";
    EXPECT_EQ(vec.size(), 3) << "Expected original vector to keep all components";
    const Vec& dbVec2 = db.getComponent<Vec>(testID2);
    EXPECT_EQ(dbVec2[0], vec2[0]) << "Expected first component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec2[1], vec2[1]) << "Expected second component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec2[2], vec2[2]) << "Expected third component of position to be equal, but it isn't";
    EXPECT_EQ(vec2.size(), 3) << "Expected original vector to keep all components";
};

TEST(ComponentTest, CanGetComponentSafelyWhenItDoesntExist) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    EXPECT_EQ(db.getComponentSafe<Vec>(testID), std::nullopt) << "Expected entity to return a null optional if component isn't present";
};

TEST(ComponentTest, CanGetComponentSafelyWhenItDoesExist) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponent<Vec>(testID);
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have component, but it doesn't";
    Vec vec({1, 2, 3});
    db.setComponentCopy<Vec>(testID, vec);
    Vec dbVec = db.getComponentSafe<Vec>(testID).value();
    EXPECT_EQ(dbVec[0], vec[0]) << "Expected first component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[1], vec[1]) << "Expected second component of position to be equal, but it isn't";
    EXPECT_EQ(dbVec[2], vec[2]) << "Expected third component of position to be equal, but it isn't";
    EXPECT_EQ(vec.size(), 3) << "Expected original vector to keep all components";
};

TEST(ComponentTest, DoesNotHaveUnusedComponent) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    ASSERT_FALSE(db.hasComponent<Map>(testID)) << "Expected entity to not have a component that's never been used";
};

TEST(DeferredTest, CanAddSingleComponentDeferred) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.flushDeferred(testID);
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity to have component after single deferred flush";
};

TEST(DeferredTest, CanAddMultipleComponentsDeferred) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.addComponentDeferred<Vec>(testID);
    db.flushDeferred(testID);
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity to have map component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have vec component after single deferred flush";
};

TEST(DeferredTest, CanRemoveComponentsDeferred) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.addComponentDeferred<Vec>(testID);
    db.flushDeferred(testID);
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity to have map component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have vec component after single deferred flush";
    db.removeComponentDeferred<Map>(testID);
    db.flushDeferred(testID);
    ASSERT_FALSE(db.hasComponent<Map>(testID)) << "Expected entity not to have map component";
};

TEST(DeferredTest, RemovingComponentsDeferredAddsNewTable) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.addComponentDeferred<Vec>(testID);
    db.flushDeferred(testID);
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity to have map component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have vec component after single deferred flush";
    db.removeComponentDeferred<Vec>(testID);
    db.flushDeferred(testID);
    ASSERT_FALSE(db.hasComponent<Vec>(testID)) << "Expected entity not to have map component";
};

TEST(DeferredTest, RemovingManyComponentsDeferredAddsNewEdges) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.addComponentDeferred<Vec>(testID);
    db.addComponentDeferred<int>(testID);
    db.addComponentDeferred<float>(testID);
    db.flushDeferred(testID);
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity to have map component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have vec component after single deferred flush";
    db.removeComponentDeferred<Vec>(testID);
    db.removeComponentDeferred<float>(testID);
    db.flushDeferred(testID);
    ASSERT_FALSE(db.hasComponent<Vec>(testID)) << "Expected entity not to have map component";
    ASSERT_FALSE(db.hasComponent<float>(testID)) << "Expected entity not to have map component";

    EntityID testID2 = db.createEntityID();
    db.addComponentDeferred<Map>(testID2);
    db.addComponentDeferred<int>(testID2);
    db.flushDeferred(testID2);
};

TEST(DeferredTest, DeferredCommandsOverwrite) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.addComponentDeferred<Vec>(testID);
    db.addComponentDeferred<int>(testID);
    db.removeComponentDeferred<float>(testID);
    db.addComponentDeferred<float>(testID);
    db.flushDeferred(testID);
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity to have map component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity to have vec component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<float>(testID)) << "Expected entity to have float component after single deferred flush with overwrite";
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity to have int component after single deferred flush";
};

TEST(DeferredTest, CanFlushAllEntities) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    db.addComponentDeferred<Map>(testID);
    db.addComponentDeferred<Vec>(testID);
    db.addComponentDeferred<Vec>(testID2);
    db.addComponentDeferred<float>(testID2);
    db.flushDeferredAll();
    ASSERT_TRUE(db.hasComponent<Map>(testID)) << "Expected entity 1 to have map component after all deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID)) << "Expected entity 1 to have vec component after all deferred flush";
    ASSERT_TRUE(db.hasComponent<float>(testID2)) << "Expected entity 2 to have float component after all deferred flush";
    ASSERT_TRUE(db.hasComponent<Vec>(testID2)) << "Expected entity 2 to have int component after all deferred flush";
    
    db.addComponentDeferred<float>(testID);
    db.addComponentDeferred<Map>(testID2);
    db.flushDeferredAll();

    ASSERT_TRUE(db.hasComponent<Map>(testID2)) << "Expected entity 2 to have map component after all deferred flush";
    ASSERT_TRUE(db.hasComponent<float>(testID)) << "Expected entity 1 to have float component after all deferred flush";
};

TEST(DeferredTest, CanSetComponentsDeferred) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<float>(testID);
    db.addComponentDeferred<int>(testID);
    db.flushDeferredAll();
    ASSERT_TRUE(db.hasComponent<float>(testID)) << "Expected entity 1 to have float component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity 1 to have int component after single deferred flush";

    float x = 5.0;
    int y = 3;
    db.setComponentCopyDeferred<float>(testID, &x);
    db.setComponentCopyDeferred<int>(testID, &y);

    db.flushDeferredAll();

    ASSERT_EQ(db.getComponent<float>(testID), 5.0) << "Expected deferred copy to copy local float pointer data to entity";
    ASSERT_EQ(db.getComponent<int>(testID), 3) << "Expected deferred copy to copy local int pointer data to entity";
};

TEST(DeferredTest, CanSetAndAddComponentsDeferred) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID = db.createEntityID();
    db.addComponentDeferred<float>(testID);
    db.addComponentDeferred<int>(testID);
    float x = 5.0;
    int y = 3;
    db.setComponentCopyDeferred<float>(testID, &x);
    db.setComponentCopyDeferred<int>(testID, &y);
    db.flushDeferredAll();
    ASSERT_TRUE(db.hasComponent<float>(testID)) << "Expected entity 1 to have float component after single deferred flush";
    ASSERT_TRUE(db.hasComponent<int>(testID)) << "Expected entity 1 to have int component after single deferred flush";
    ASSERT_EQ(db.getComponent<float>(testID), 5.0) << "Expected deferred copy to copy local float pointer data to entity";
    ASSERT_EQ(db.getComponent<int>(testID), 3) << "Expected deferred copy to copy local int pointer data to entity";
};

// TEST(StressTest, OneMillionEntitiesStressTest) {
//     using namespace Wado::ECS;
//     Database db = Database();
//     int x = 5;
//     const int maxIDs = 1000000;
//     std::vector<EntityID> ids(maxIDs);
//     for (int i = 0; i < maxIDs; i++) {
//         EntityID tempID = db.createEntityID();
//         db.addComponentDeferred<Position>(tempID);
//         db.addComponentDeferred<int>(tempID);
//         //db.setComponentCopyDeferred<int>(tempID, &x);
//         db.addComponentDeferred<float>(tempID);
//         db.addComponentDeferred<Vec>(tempID);
//         db.addComponentDeferred<Map>(tempID);
//         db.setComponentCopyDeferred<int>(tempID, &x);
//         ids[i] = tempID;
//     };
//     db.flushDeferredAll();
//     // for (int i = 0; i < maxIDs - 1; i++) {
//     //     db.setComponentCopyDeferred<int>(ids[i], &x);
//     // };

//     for (int i = 0; i < maxIDs - 1; i++) {
//         ASSERT_EQ(db.getComponent<int>(ids[i]), x) << "Entity int value set incorrectly";
//     };
// };

// TEST(MemoryTest, CanCleanup) {
//     using namespace Wado::ECS;
//     Database db = Database();
//     int x = 5;
//     const int maxIDs = 1000;
//     std::vector<EntityID> ids(maxIDs);
//     for (int i = 0; i < maxIDs; i++) {
//         EntityID tempID = db.createEntityID();
//         db.addComponentDeferred<int>(tempID);
//         db.setComponentCopyDeferred<int>(tempID, &x);
//         ids[i] = tempID;
//     };
//     db.flushDeferredAll();
//     for (int i = maxIDs / 2; i < maxIDs; i++) {
//         db.destroyEntity(ids[i]);
//     };

//     db.cleanupMemory();
// };

TEST(MemoryTest, CanHandleDeleteListCorrectly) {
    using namespace Wado::ECS;
    Database db = Database();
    int x = 5;
    const int maxIDs = 1000;
    std::vector<EntityID> ids(maxIDs);
    for (int i = 0; i < maxIDs; i++) {
        EntityID tempID = db.createEntityID();
        db.addComponentDeferred<int>(tempID);
        db.setComponentCopyDeferred<int>(tempID, &x);
        ids[i] = tempID;
    };
    db.flushDeferredAll();

    for (int i = 0; i < 300; i += 3) {
        db.destroyEntity(ids[i]);
    }

    for (int i = 500; i < 550; i++) {
        db.destroyEntity(ids[i]);
    };

    //db.cleanupMemory();
};

using Player = struct Player{};
// TODO: for tags, make sure no mallocs are ever called 
TEST(TagTest, SupportsEmptyStructTags) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID tempID = db.createEntityID();
    db.addComponent<Player>(tempID);
    ASSERT_TRUE(db.hasComponent<Player>(tempID)) << "Expected entity to have player tag";
};

using Likes = struct Likes { };
using ParentOf = struct ParentOf { };
using ChildOf = struct ChildOf { };

TEST(RelationshipTest, CanAddRelationships) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID1 = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    db.addRelationship<Likes>(testID1, testID2);
    ASSERT_TRUE(db.hasRelationship<Likes>(testID1, testID2)) << "Expected entity 1 to have a like relationship with entitiy 2";
    ASSERT_FALSE(db.hasRelationship<Likes>(testID2, testID1)) << "Expetced entity 2 not to have a relationship with entity 1";
};

TEST(RelationshipTest, CanRemoveRelationships) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID1 = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    db.addRelationship<Likes>(testID1, testID2);
    db.addRelationship<ChildOf>(testID1, testID2);
    ASSERT_TRUE(db.hasRelationship<Likes>(testID1, testID2)) << "Expected entity 1 to have a like relationship with entitiy 2";
    db.removeRelationship<Likes>(testID1, testID2);
    ASSERT_FALSE(db.hasRelationship<Likes>(testID1, testID2)) << "Expetced entity 1 not to have a relationship with entity 2";
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID1, testID2)) << "Remove relationship doesn't affect other relationships";
};

TEST(RelationshipTest, CanGetMultipleSubentitiesWithSameRelationship) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID1 = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    EntityID testID3 = db.createEntityID();
    db.addRelationship<ChildOf>(testID1, testID2);
    db.addRelationship<ChildOf>(testID1, testID3);
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID1, testID2)) << "Expect entity 1 to have both parent relationships";
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID1, testID3)) << "Expect entity 1 to have both parent relationships";
    std::unordered_set<EntityID> parentIDs = db.getRelationshipTargets<ChildOf>(testID1);
    ASSERT_EQ(parentIDs, std::unordered_set<EntityID>({testID2, testID3})) << "Expect both parents to be retrieved";
};

TEST(RelationshipTest, SubentitySetReducesWhenRemovingRelationship) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID1 = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    EntityID testID3 = db.createEntityID();
    db.addRelationship<ChildOf>(testID1, testID2);
    db.addRelationship<ChildOf>(testID1, testID3);
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID1, testID2)) << "Expect entity 1 to have both parent relationships";
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID1, testID3)) << "Expect entity 1 to have both parent relationships";
    std::unordered_set<EntityID> parentIDs = db.getRelationshipTargets<ChildOf>(testID1);
    ASSERT_EQ(parentIDs, std::unordered_set<EntityID>({testID2, testID3})) << "Expect both parents to be retrieved";
    db.removeRelationship<ChildOf>(testID1, testID2);
    std::unordered_set<EntityID> newParentIDs = db.getRelationshipTargets<ChildOf>(testID1);
    ASSERT_EQ(newParentIDs.size(), 1);
    ASSERT_EQ(*newParentIDs.begin(), testID3);
};

TEST(RelationShipTest, CanGetAllTargetIDs) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID1 = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    EntityID testID3 = db.createEntityID();
    EntityID testID4 = db.createEntityID();
    db.addRelationship<ChildOf>(testID1, testID2);
    db.addRelationship<ChildOf>(testID4, testID3);
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID1, testID2)) << "Expect entity 1 to be a child to 2";
    ASSERT_TRUE(db.hasRelationship<ChildOf>(testID4, testID3)) << "Expect entity 4 to be a child to 3";
    std::unordered_set<EntityID> parentIDs = db.getAllRelationshipTargets<ChildOf>();
    ASSERT_EQ(parentIDs, std::unordered_set<EntityID>({testID2, testID3})) << "Expected both test entity 2 and 3 to be in the global parent set";
};

TEST(QueryTest, CanCreateAndIterateSimpleQueries) {
    using namespace Wado::ECS;
    Database db = Database();
    EntityID testID1 = db.createEntityID();
    EntityID testID2 = db.createEntityID();
    EntityID testID3 = db.createEntityID();
    EntityID testID4 = db.createEntityID();
    db.addComponent<Position>(testID1);
    Position pos1{1.0, 2.0};
    db.setComponentMove<Position>(testID1, pos1);
    db.addComponent<float>(testID1);
    db.addComponent<float>(testID2);
    db.addComponent<Position>(testID3);
    Position pos2{3.0, 2.0};
    db.setComponentMove<Position>(testID3, pos2);
    db.addComponent<float>(testID4);
    db.addComponent<Position>(testID4);
    Position pos3{4.0, 2.0};
    db.setComponentMove<Position>(testID4, pos3);

    Database::QueryBuilder& builder = db.makeQueryBuilder(); //.withComponent<Position>().build();
    
    Database::QueryBuilder& builder2 = builder.withComponent<Position>();

    std::vector<Position> positions;

    /*for (Query::FullIterator it = query.begin(); it != query.end(); ++it) {
        //Position& p = it.operator*<Position>();
        positions.push_back({1.0, 2.0});
        //p.x += 1.0;
        //p.y += 1.0;
    };*/
/*
    const Position& newPos1 = db.getComponent<Position>(testID1);

    ASSERT_EQ(newPos1.x, 2.0) << "Expected query to increase values";
    ASSERT_EQ(newPos1.x, 3.0) << "Expected query to increase values"; */
};