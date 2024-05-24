#include "ECS.h"

#include <stdexcept>

namespace Wado::ECS {
    Entity::Entity(EntityID entityID, Database* database) : 
        _entityID(entityID), 
        _database(database) { };

    Database::Table::Table(TableType type) : _type(type) {

    };

    
    Database::Database() {
        // Create the empty table and place it in the 
        // table vector, and initalize the empty table 
        // variable, empty table will always be the 0th
        // element of this collection
        _tables.emplace_back(std::vector<uint64_t>());
    };

    Database::~Database() {
        // TODO: cleanup functions here 
    };

    EntityID Database::generateNewEntityID() {
        EntityID newId;
        if (!_reusableEntityIDs.empty()) {
            newId = _reusableEntityIDs.back();
            _reusableEntityIDs.pop_back();
        } else {
            if (ENTITY_ID == MAX_ENTITY_ID) {
                throw std::runtime_error("Reached the maximum number of live entities, cannot create any new ones.");
            }
            newId = (ENTITY_ID++);
        };
        return newId;
    };

    EntityID Database::generateNewIDAndAddToEmptyTableRegistry() {
        EntityID newID = generateNewEntityID();
        // The 0-th table will always be the default empty
        // table. 
        addEntityToTableRegistry(newID, 0); // first table, the "empty" table
        return newID;
    };


    void Database::addEntityToTableRegistry(EntityID entityID, size_t tableIndex) {
        _tableRegistry.emplace(entityID & ENTITY_ID_MASK, tableIndex, _tables[tableIndex]._rowCount);
        _tables[tableIndex]._rowCount++;
        // TODO: need to resize entity columns here if needed and increase capacity, count, etc  
    };

    Memory::WdClonePtr<Entity> Database::createEntityClonePtr() {
        return _entityMainPtrs.emplace_back(new Entity(generateNewIDAndAddToEmptyTableRegistry(), this)).getClonePtr();
    };

    Entity Database::createEntityObj() {
        return Entity(generateNewIDAndAddToEmptyTableRegistry(), this);
    };

    EntityID Database::createEntityID() {
        return generateNewIDAndAddToEmptyTableRegistry();
    };

    ComponentID Database::generateNewComponentID() {
        if (COMPONENT_ID == MAX_COMPONENT_ID) {
                throw std::runtime_error("Reached the maximum number of components, cannot create any new ones.");
        }
        return COMPONENT_ID++;
    };

    template <class T>
    ComponentID Database::getComponentID() {
        static ComponentID componentID = generateNewComponentID();
        return componentID;
    };

    size_t Database::getNextTableOrAddEdges(size_t tableIndex, const ComponentID componentID) {
        //Table::TableEdges::iterator nextTable = table._addComponentGraph.find(componentID);
        // TODO: need to add table to component registry too. 

    };


    size_t Database::findTableSuccessor(const TableType& fullType) {
        size_t currentTableIndex = 0;
        for (const ComponentID& componentID : fullType) {
            Table::TableEdges::iterator nextTable = _tables[currentTableIndex]._addComponentGraph.find(componentID);
            
            // Next table doesn't exist, need to create. 
            // TODO: this is slow, once a table is found not to exist 
            // all of its successors will also not exist. 
            if (nextTable == _tables[currentTableIndex]._addComponentGraph.end()) {
                // Create type of the new table
                TableType newType(_tables[currentTableIndex]._type);
                newType.insert(componentID);
                _tables.emplace_back(newType);
                // Size of table - 1 is the index of the newly inserted table, need
                // to add this to the graph now.
                _tables[currentTableIndex]._addComponentGraph[componentID] = _tables.size() - 1;
                // need to add the backwards edge too 
                _tables.back()._removeComponentGraph[componentID] = currentTableIndex;

                currentTableIndex = _tables.size() - 1;
            } else {
                currentTableIndex = nextTable->second;
            };
        };
        return currentTableIndex;
    };


    template <class T>
    void Database::addComponent(EntityID entityID) {
        // When adding a component, check 
        // current entity's table table graph.
        // if edge exists to new table we need 
        // to move all the overlapping data 
        // (can I SIMD this? It's just memcopies to
        // all the columns I think. )
        // If it doesn't exist need to create new table
        // and update. Also need to add new table to 
        // componet's registry, and update
        // the index for the table index of this entity.
        // This all seems pretty expensive. 
        // This should be the immediate version, need to add 
        // a deferred version as welll.

        // When if table edge doesn't exist, need to 
        // traverse the entire tree with every component 
        // until we've found the edges. 

    };

};