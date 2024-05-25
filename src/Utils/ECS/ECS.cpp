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

    Database::Table::Table(TableType type, const ComponentSizes& sizes, const size_t defaultColumnSize) : _type(type), _maxComponentID(*(--type.end())) {
        for (const ComponentID componentID : type) {
            void* columnData = malloc(sizes.at(componentID) * defaultColumnSize);
            
            if (columnData == nullptr) {
                throw std::runtime_error("Ran out of memory for storing column data");
            };
            _columns.emplace(componentID, columnData, sizes.at(componentID), defaultColumnSize);
        };
    };

    // Empty constructor
    Database::Table::Table() : _type({}), _maxComponentID(0) {

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

    size_t Database::createTableGraph(size_t startingTableIndex, const TableType& addType) {
        size_t currentTableIndex = startingTableIndex;
        TableType::const_iterator it;
        Table::TableEdges::iterator addGraphIt;
        Table::TableEdges::iterator addGraphEnd;;

        for (it = addType.begin(); it != addType.end(); it++) {
            addGraphIt = _tables[currentTableIndex]._addComponentGraph.find(*it);
            addGraphEnd = _tables[currentTableIndex]._addComponentGraph.end();

            // TODO: find a way to do this without many branches 
            if (addGraphIt == addGraphEnd) {
                break;
            };

            currentTableIndex = addGraphIt->second;
        };

        if (it == addType.end()) {
            // All edges existed in the table graph, can return
            return currentTableIndex;
        };

        // At this point, we found an edge that doesn't exist, 
        // but because all add components are greater than the 
        // current table's max we can create & add all tables 
        // in order here. 
        while (it != addType.end()) {
            // Type of this is the type of the entry table
            // + every type until the it component  
            TableType newType(_tables[currentTableIndex]._type);
            newType.insert(addType.begin(), std::next(it, 1));
            // TODO: maybe shouldnt do default common size here, since 
            // the intermediary table might not immediately need
            // as many components 
            _tables.emplace_back(newType, _componentSizes, _defaultColumnSize);
            _tables[currentTableIndex]._addComponentGraph.insert(*it, _tables.size() - 1);
            _tables.back()._removeComponentGraph.insert(*it, currentTableIndex);

            currentTableIndex = _tables.size() - 1; // TODO: This could potentially always be .back here after the first
            // insert, maybe that would be faster.

            // Add new table to component registry. 
            _componentRegistry[*it].insert(currentTableIndex);
        };

        return currentTableIndex; // final table index 
    };

    size_t Database::findTableSuccessor(const size_t tableIndex, const TableType& addType) {
        size_t startingTableIndex = tableIndex;
        TableType startingAddType = addType;
        // If the first add component ID is greater than the 
        // max component id of the table we need to perform the 
        // graph search from the beginning
        if (_tables[tableIndex]._maxComponentID > *addType.begin()) {
            startingTableIndex = 0;
            startingAddType.insert(_tables[tableIndex]._type.begin(), _tables[tableIndex]._type.end());
        };

        // Now we can perform the graph search 
        return createTableGraph(startingTableIndex, startingAddType);
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
        // TODO: not sure if I should use the deferred version here 
        size_t nextTableIndex = findTableSuccessor(currentTableIndex, {getComponentID<T>()});

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
        DeferredPayload& entityPayload = _deferredPayloads[entityID];
        std::set<ComponentID> commonPayloads;
        // TODO: this might be very, very, very slow. 
        std::set_intersection(entityPayload._addPayload.begin(), entityPayload._addPayload.end(), entityPayload._removePayload.begin(), entityPayload._removePayload.end(), commonPayloads);
        
        TableType addType;
        std::set_difference(entityPayload._addPayload.begin(), entityPayload._addPayload.end(), commonPayloads.begin(), commonPayloads.end(), addType);

        TableType removeType;
        std::set_difference(entityPayload._removePayload.begin(), entityPayload._removePayload.end(), commonPayloads.begin(), commonPayloads.end(), removeType);

    };

    void Database::flushDeferredAll(EntityID entityID) {
        // TODO: maybe some payload sorting here could help...?
        // Not sure how to reason about it
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
        _deferredPayloads[entityID]._setMap.emplace(getComponentID<T>(), static_cast<void *>(componentData), SetMode::Copy);
    };

    template <class T>
    void Database::setComponentMoveDeferred(EntityID entityID, T* componentData) {
        _deferredPayloads[entityID]._setMap.emplace(getComponentID<T>(), static_cast<void *>(componentData), SetMode::Move);
    };

};