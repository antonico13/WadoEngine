#include "ECS.h"

#include <stdlib.h> 
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <optional>

namespace Wado::ECS {
    Entity::Entity(EntityID entityID, Database* database) : 
        _entityID(entityID), 
        _database(database) { };

    Database::Table::Table(TableType type, const ComponentSizes& sizes, const size_t defaultColumnSize) : _type(type) {
        for (const ComponentID componentID : type) {
            void* columnData = malloc(sizes.at(componentID) * defaultColumnSize);
            
            if (columnData == nullptr) {
                throw std::runtime_error("Ran out of memory for storing column data");
            };
            _columns.emplace(componentID, columnData, sizes.at(componentID), defaultColumnSize);
        };
    };

    // Empty constructor
    Database::Table::Table() : _type({}) {

    };

    
    Database::Database(size_t defaultColumnSize) : _defaultColumnSize(defaultColumnSize) {
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
        addEntityToTableRegistry(newID, 0, _tables.front()._rowCount); // first table, the "empty" table
        return newID;
    };


    void Database::addEntityToTableRegistry(EntityID entityID, size_t tableIndex, size_t position) {
        _tableRegistry.emplace(entityID & ENTITY_ID_MASK, tableIndex, position);
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
        _componentSizes.try_emplace(componentID, sizeof(T));
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
                _tables.emplace_back(newType, *this);
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

    void Database::moveToTable(EntityID entityID, size_t sourceTableIndex, size_t destTableIndex) {
        size_t currentEntityRowIndex = _tableRegistry[entityID].entityColumnIndex;
        Table& sourceTable = _tables[sourceTableIndex];
        Table& destTable = _tables[destTableIndex];

        // the row for this entity is voided from the original table
        sourceTable.deleteList.push_back(currentEntityRowIndex);

        // get next row index for the entity 
        size_t freeRowIndex;
        if (destTable.deleteList.empty()) {
            freeRowIndex = destTable._rowCount++;
            //destTable._rowCount++;
        } else {
            freeRowIndex = destTable.deleteList.back();
            destTable.deleteList.pop_back();
        };

        // Now, update table registry
        addEntityToTableRegistry(entityID, destTableIndex, freeRowIndex);
        
        // If we are adding a new row and its index is greater than
        // the column capacity, we need to realloc all columns. 
        // TODO: this can also be vectorized with SIMD I think. 
        if (freeRowIndex >= destTable._columns.begin()->second.capacity) {
            // everything needs to be resized in this case. 
            // By default, just double capacity.
            for (Columns::iterator it = destTable._columns.begin(); it != destTable._columns.end(); it++) {
                it->second.data = realloc(it->second.data, it->second.capacity * 2);
                
                if (it->second.data == nullptr) {
                    throw std::runtime_error("Ran out of memory, cannot increase row count for table.");
                };

                it->second.capacity = it->second.capacity * 2;
            };
        };

        // Now perform data copies for overlapping components, 
        // with guaranteed space, first find overlapping components. 
        TableType overlappingColumns;
        // TODO: does this work if the set doesn't have capacity reserved for the overlapping elements? 
        std::set_intersection(sourceTable._type.begin(), sourceTable._type.end(), destTable._type.begin(), destTable._type.end(), overlappingColumns.begin());
        // TODO: can I SIMD/vectorize this in any way?
        
        // Copy column content
        for (const ComponentID& componentID : overlappingColumns) {
            size_t copySize = sourceTable._columns[componentID].elementStride;
            // Always deal with byte increments 
            void* copyFrom = static_cast<char*>(sourceTable._columns[componentID].data) + copySize * currentEntityRowIndex;
            void* copyTo = static_cast<char*>(destTable._columns[componentID].data) + copySize * freeRowIndex;
            std::memcpy(copyTo, copyFrom, copySize);
        };  
    };


    // This is immediate mode. 
    // For multiple components in quick succession,
    // use the deferred mode. 
    template <class T>
    void Database::addComponent(EntityID entityID) {
        size_t currentTableIndex = _tableRegistry[entityID].tableIndex;
        size_t nextTableIndex = getNextTableOrAddEdges(currentTableIndex, getComponentID<T>());

        moveToTable(entityID, currentTableIndex, nextTableIndex);
    };

    template <class T> 
    void Database::removeComponent(EntityID entityID) {
        // By construction, every remove edge will 
        // already exist in immediate mode and point 
        // to the unique table due to adding components first. 
        size_t currentTableIndex = _tableRegistry[entityID].tableIndex;
        size_t nextTableIndex = _tables[currentTableIndex]._removeComponentGraph[getComponentID<T>()];

        moveToTable(entityID, currentTableIndex, nextTableIndex);
    };

    template <class T>
    bool Database::hasComponent(EntityID entityID) const {
        //size_t tableIndex = _tableRegistry[entityID].tableIndex;
        size_t tableIndex = _tableRegistry[entityID].tableIndex;
        std::set<size_t> typeSet& = _componentRegistry.at(tableIndex);
        return typeSet.find(tableIndex) != typeSet.end();
    };

    template <class T>
    const T& Database::getComponent(EntityID entityID) const {
        ComponentID componentID = getComponentID<T>();
        size_t tableIndex = _tableRegistry[entityID].tableIndex;
        size_t entityColumnIndex = _tableRegistry[entityID].entityColumnIndex;
        const Column& column = _tables[tableIndex]._columns[componentID];
        return *static_cast<T*>(column.data + column.elementStride * entityColumnIndex);
    };

    template <class T>
    std::optional<const T&> Database::getComponentSafe(EntityID entityID) const {
        if (!hasComponent<T>(entityID)) {
            return std::nullopt;
        } else {
            return std::optional<T>(getComponent<T>(entityID));
        };
    };

    template <class T> 
    void setComponentCopy(EntityID entityID, T componentData) {
        ComponentID componentID = getComponentID<T>();
        size_t tableIndex = _tableRegistry[entityID].tableIndex;
        size_t entityColumnIndex = _tableRegistry[entityID].entityColumnIndex;
        const Column& column = _tables[tableIndex]._columns[componentID];
        // This version uses the copy constructor. 
        *static_cast<T*>(column.data + column.elementStride * entityColumnIndex) = componentData;
    };

    template <class T>
    void setComponentMove(EntityID entityID, T& componentData) {
        ComponentID componentID = getComponentID<T>();
        size_t tableIndex = _tableRegistry[entityID].tableIndex;
        size_t entityColumnIndex = _tableRegistry[entityID].entityColumnIndex;
        const Column& column = _tables[tableIndex]._columns[componentID];
        // This version uses the move constructor
        *static_cast<T*>(column.data + column.elementStride * entityColumnIndex) = std::move(componentData);
    };


    void Database::flushDeferred(EntityID entityID) {

    };

    void Database::flushDeferredAll(EntityID entityID) {

    };

    template <class T>
    void Database::addComponentDeferred(EntityID entityID) {
        _deferredPayloads[entityID]._addPayload.insert(getComponentID<T>());
    };

    template <class T> 
    void Database::removeComponentDeferred(EntityID entityID) {
        _deferredPayloads[entityID]._removePayload.insert(getComponentID<T>());
    };

    template <class T> 
    void Database::setComponentCopyDeferred(EntityID entityID, T* componentData) {
        _deferredPayloads[entityID]._setPayloadCopy.insert(getComponentID<T>(), static_cast<void *>(componentData));
    };

    template <class T>
    void Database::setComponentMoveDeferred(EntityID entityID, T* componentData) {
        _deferredPayloads[entityID]._setPayloadMove.insert(getComponentID<T>(), static_cast<void *>(componentData));
    };

};