#include "ECS.h"

#include <stdlib.h> 
#include <stdexcept>
#include <cstring>

namespace Wado::ECS {
    Entity::Entity(EntityID entityID, Database* database) : 
        _entityID(entityID), 
        _database(database) { };

    Database::Table::Table(TableType type) : _type(type) {

    };

    // Empty constructor
    Database::Table::Table() : _type({}) {

    };

    
    Database::Database() {
        // Create the empty table and place it in the 
        // table vector, and initalize the empty table 
        // variable, empty table will always be the 0th
        // element of this collection
        _tables.emplace_back();
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
        addEntityToTableRegistry(newID, 0, _tables[0]._rowCount); // first table, the "empty" table
        return newID;
    };


    void Database::addEntityToTableRegistry(EntityID entityID, size_t tableIndex, size_t position) {
        _tableRegistry.emplace(entityID & ENTITY_ID_MASK, tableIndex, position);
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
        Table::TableEdges::iterator nextTable = _tables[tableIndex]._addComponentGraph.find(componentID);
        size_t nextTableID;
        if (nextTable == _tables[tableIndex]._addComponentGraph.end()) {
            // Successor doesn't exist, perform a full graph walk 
            // to find or create the path to the successor.

            // TODO: can optimize here. For example, if the requested
            // componentID is greater than the maximum component ID 
            // in the current table's full type, a full graph walk is not 
            // required, since the current type will represent an ascending
            // order. In that case, we can just create the new table directly 
            // and add it. This also avoids unneccesary set operations.

            // Can solve this by storing the max component ID on a table type
            // at creation time. 
            TableType newTableType(_tables[tableIndex]._type);
            newTableType.insert(componentID);
            nextTableID = findOrAddTable(newTableType);

            // Add graph edges now, component registry will 
            // have ben taken care of by the findOrAddTable function. 
            // This can also be optimized away with the maximum component ID optimization. 
            _tables[tableIndex]._addComponentGraph[componentID] = nextTableID;
            _tables[nextTableID]._removeComponentGraph[componentID] = tableIndex; 

        } else {
            nextTableID = nextTable->second;
        };

        return nextTableID;
    };


    size_t Database::findOrAddTable(const TableType& fullType) {
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
                // Add new table to component registry too.
                _componentRegistry[componentID].insert(currentTableIndex);

            } else {
                currentTableIndex = nextTable->second;
            };
        };
        return currentTableIndex;
    };

    // This is immediate mode. 
    // For multiple components in quick succession,
    // use the deferred mode. 
    template <class T>
    void Database::addComponent(EntityID entityID) {
        // TODO: replace all these array accesses with refs,
        // do the same in other functions too. 
        size_t currentTableIndex = _tableRegistry[entityID].tableIndex;
        size_t currentEntityColumnIndex = _tableRegistry[entityID].entityColumnIndex;
        size_t nextTableIndex = getNextTableOrAddEdges(currentTableIndex, getComponentID<T>());

        // the row for this entity is voided from the original table
        _tables[currentTableIndex].deleteList.push_back(currentEntityColumnIndex);

        size_t freeRowIndex;
        if (_tables[nextTableIndex].deleteList.empty()) {
            freeRowIndex = _tables[nextTableIndex]._rowCount;
            _tables[nextTableIndex]._rowCount++;
        } else {
            freeRowIndex = _tables[nextTableIndex].deleteList.back();
            _tables[nextTableIndex].deleteList.pop_back();
        };

        // Now, update table registry
        addEntityToTableRegistry(entityID, nextTableIndex, freeRowIndex);
        
        // If we are adding a new row and its index is greater than
        // the column capacity, we need to realloc all columns. 
        // TODO: this can also be vectorized with SIMD I think. 
        if (freeRowIndex >= _tables[nextTableIndex]._columns.begin()->second.capacity) {
            // everything needs to be resized in this case. 
            // By default, just double capacity.
            for (Columns::iterator it = _tables[nextTableIndex]._columns.begin(); it != _tables[nextTableIndex]._columns.end(); it++) {
                it->second.data = realloc(it->second.data, it->second.capacity * 2);
                
                if (it->second.data == nullptr) {
                    throw std::runtime_error("Ran out of memory, cannot increase row count for table.");
                };

                it->second.capacity = it->second.capacity * 2;

            };
        };

        // Now perform data copies for overlapping components, 
        // with guaranteed space. 
        // TODO: can I SIMD/vectorize this in any way?
        
        // Copy column content
        for (ComponentID& componentID : _tables[currentTableIndex]._type) {
            Column& copyFrom = _tables[currentTableIndex]._columns[componentID];
            Column& copyTo = _tables[nextTableIndex]._columns[componentID];
            std::memcpy(copyTo.data + copyTo.elementStride * freeRowIndex, copyFrom.data + copyFrom.elementStride * currentEntityColumnIndex, copyFrom.elementStride);
        };
    };

};