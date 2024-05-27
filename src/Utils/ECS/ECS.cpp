#ifndef WADO_ECS_CPP
#define WADO_ECS_CPP
#include "ECS.h"

#include <stdlib.h> 
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <optional>
#include <iterator>

#include <stdio.h>

namespace Wado::ECS {
    Entity::Entity(EntityID entityID, Database* database) : 
        _entityID(entityID), 
        _database(database) { };

    Database::Table::Table(TableType type, const ComponentSizes& sizes, const size_t defaultColumnSize) : _type(type), _maxComponentID(*(type.rbegin())), _capacity(defaultColumnSize), _maxOccupiedRow(0) {
        for (const ComponentID componentID : type) {
            char* columnData = static_cast<char*>(malloc(sizes.at(componentID) * defaultColumnSize));
            
            if (columnData == nullptr) {
                throw std::runtime_error("Ran out of memory for storing column data");
            };
            // TODO: won't let me emplace with arguments directly...?
            _columns.emplace(componentID, Column(columnData, sizes.at(componentID)));
        };
    };

    // Empty constructor
    Database::Table::Table() : _type({}), _maxComponentID(0), _capacity(DEFAULT_COLUMN_SIZE), _maxOccupiedRow(0) {

    };

    // Static ID init 
    ComponentID Database::COMPONENT_ID = 1;

    Database::Database(size_t defaultColumnSize) : _defaultColumnSize(defaultColumnSize) {
        // Create the empty table and place it in the 
        // table vector, and initalize the empty table 
        // variable, empty table will always be the 0th
        // element of this collection
        _tables.emplace_back();
    };

    Database::~Database() {
        // Go over every table and free the data in every column of the table. 
        for (Table& table : _tables) {
            for (Columns::iterator it = table._columns.begin(); it != table._columns.end(); ++it) {
                free(it->second.data);
            };
        };
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
        addEntityToTableRegistry(newID, 0, _tables.front()._maxOccupiedRow); // first table, the "empty" table
        _tables.front()._maxOccupiedRow++;
        return newID;
    };


    void Database::addEntityToTableRegistry(EntityID entityID, size_t tableIndex, size_t position) noexcept {
        // TODO: fix this and use proper emplace 
        TableIndex newIndex{};
        newIndex.entityColumnIndex = position;
        newIndex.tableIndex = tableIndex;
        auto res = _tableRegistry.insert_or_assign((entityID & ENTITY_ID_MASK), newIndex);
        if (res.second == true) {
            //std::cout << "Inserted to registry " << std::endl;
        } else {
            //std::cout << "Updated registry " << std::endl;
        }
    };

    // Memory::WdClonePtr<Entity> Database::createEntityClonePtr() {
    //     return _entityMainPtrs.emplace_back(new Entity(generateNewIDAndAddToEmptyTableRegistry(), this)).getClonePtr();
    // };

    Entity Database::createEntityObj() {
        return Entity(generateNewIDAndAddToEmptyTableRegistry(), this);
    };

    EntityID Database::createEntityID() {
        return generateNewIDAndAddToEmptyTableRegistry();
    };

    // TODO: entity destroys should also be deferred 
    void Database::destroyEntity(EntityID entityID) noexcept {
        // TODO: repalce vector lookups with table refs 
        size_t tableIndex = _tableRegistry[entityID].tableIndex;
        //std::cout << "Delete table index: " << tableIndex << std::endl;
        size_t tableRowIndex = _tableRegistry[entityID].entityColumnIndex;
        // Add a block of size 1 to the list 
        _tables[tableIndex].addToFreeBlockList(Table::ColumnBlock(tableRowIndex, tableRowIndex, 1));

        //std::cout << "Free block list looks like: " << std::endl;
        for (const Table::ColumnBlock& freeBlock : _tables[tableIndex]._freeBlockList) {
            //std::cout << "[" << freeBlock.startRow << ", " << freeBlock.endRow << "]" << ", ";
        };
        //std::cout << std::endl;

        if (tableRowIndex == _tables[tableIndex]._maxOccupiedRow - 1) {
            //std::cout << "Deleting the largest unused block at the end. " << std::endl;
            Table::FreeBlockList::iterator endBlock = --(_tables[tableIndex]._freeBlockList.end());
            //std::cout << "Deleting from " << endBlock->startRow << " to " << endBlock->endRow << std::endl;
            _tables[tableIndex]._maxOccupiedRow = endBlock->startRow;
            _tables[tableIndex]._blockSizeMap.at(endBlock->size).erase(*endBlock);
            _tables[tableIndex]._freeBlockList.erase(endBlock);
            //std::cout << "Finished deleting" << std::endl;
        };

        //std::cout << "Free block list looks like: " << std::endl;
        for (const Table::ColumnBlock& freeBlock : _tables[tableIndex]._freeBlockList) {
            //std::cout << "[" << freeBlock.startRow << ", " << freeBlock.endRow << "]" << ", ";
        };
        //std::cout << std::endl;

        _tableRegistry.erase(entityID);
        _reusableEntityIDs.emplace_back(entityID);
    };

    ComponentID Database::generateNewComponentID() {
        if (COMPONENT_ID == MAX_COMPONENT_ID) {
                throw std::runtime_error("Reached the maximum number of components, cannot create any new ones.");
        }
        //std::cout << "Generated id: " << COMPONENT_ID << std::endl;
        return COMPONENT_ID++;
    };

    template <typename T>
    ComponentID Database::getComponentID() {
        static ComponentID componentID = generateNewComponentID();
        // TODO: this is a bit ugly, need to find a way to statically
        // initialize all of this 
        _componentSizes.try_emplace(componentID, sizeof(T));
        _componentRegistry.try_emplace(componentID, std::set<size_t>());
        return componentID;
    };

    size_t Database::createTableGraph(size_t startingTableIndex, const TableType& addType) {
        size_t currentTableIndex = startingTableIndex;
        TableType::const_iterator it;
        Table::TableEdges::const_iterator addGraphIt;
        Table::TableEdges::const_iterator addGraphEnd;;

        for (it = addType.begin(); it != addType.end(); ++it) {
            addGraphIt = _tables[currentTableIndex]._addComponentGraph.find(*it);
            addGraphEnd = _tables[currentTableIndex]._addComponentGraph.end();

            // TODO: find a way to do this without many branches 
            if (addGraphIt == addGraphEnd) {
                //std::cout << "Did not find existing branch from table index " << currentTableIndex << " to add component: " << (*it) << std::endl;
                break;
            };
            //std::cout << "Found existing branch from table index " << currentTableIndex << " to add component: " << (*it) << std::endl;
            currentTableIndex = addGraphIt->second;
        };

        if (it == addType.end()) {
            // All edges existed in the table graph, can return
            //std::cout << "All edges existed, did not create any new tables " << std::endl;
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
            _tables[currentTableIndex]._addComponentGraph.emplace(*it, _tables.size() - 1);
            _tables.back()._removeComponentGraph.emplace(*it, currentTableIndex);

            currentTableIndex = _tables.size() - 1; // TODO: This could potentially always be .back here after the first
            //std::cout << "Current max table index after creating from scratch: " << currentTableIndex << std::endl;
            // insert, maybe that would be faster.

            // Add new table to component registry of every component
            // it has 
            //std::cout << "New table has: " << std::endl;
            for (const ComponentID& componentID : newType) {
                //std::cout << "ComponentID: " << componentID << " ";
                _componentRegistry[componentID].insert(currentTableIndex); 
            };
            //std::cout << std::endl;

            ++it;
        };
        //std::cout << "Finished creating table " << std::endl;
        return currentTableIndex; // final table index 
    };

    size_t Database::findTableSuccessor(const size_t tableIndex, const TableType& addType) {
        size_t startingTableIndex = tableIndex;
        TableType startingAddType = addType;
        // If the first add component ID is greater than the 
        // max component id of the table we need to perform the 
        // graph search from the beginning
        if (addType.size() > 0 && _tables[tableIndex]._maxComponentID > *addType.begin()) {
            //std::cout << "Performing graph search from the start: " << std::endl;
            startingTableIndex = 0;
            startingAddType.insert(_tables[tableIndex]._type.begin(), _tables[tableIndex]._type.end());
        };

        // Now we can perform the graph search 
        return createTableGraph(startingTableIndex, addType);
    };

    size_t Database::findTablePredecessor(const size_t tableIndex, const TableType& removeTypes) {
        size_t currentTableIndex = tableIndex;
        Table::TableEdges::const_iterator removeGraphIt;
        Table::TableEdges::const_iterator removeGraphEnd;
        // Perform a backwards graph walk until we find a missing edge.
        // Add all the edges up to that point then resume walk. 
        TableType::reverse_iterator it;// = removeTypes.rbegin();
        for (it = removeTypes.rbegin(); it != removeTypes.rend(); ++it) {
            removeGraphIt = _tables[currentTableIndex]._removeComponentGraph.find(*it);
            removeGraphEnd = _tables[currentTableIndex]._removeComponentGraph.end();
            
            // Edge not found, need to build graph from the beginning 
            // and continue 
            if (removeGraphIt == removeGraphEnd) {
                //std::cout << "Did not find remove in table " << currentTableIndex << " for component: " << (*it) << std::endl;
                break;
            };
            //std::cout << "Found remove edge from table " << currentTableIndex << " to component ID: " << (*it) << std::endl;
            currentTableIndex = removeGraphIt->second;
        };

        if (it == removeTypes.rend()) {
            //std::cout << "Found all remove edges" << std::endl;
            return currentTableIndex;
        };

        if (it != removeTypes.rend()) {
            TableType fullType;
            std::set_difference(_tables[tableIndex]._type.begin(), _tables[tableIndex]._type.end(), removeTypes.begin(), removeTypes.end(), std::inserter(fullType, fullType.begin()));
            //std::cout << "Final type that needs to be created for remove: " << std::endl;
            for (const ComponentID& componentID : fullType) {
                //std::cout << "Component: " << componentID << " ";
            };
            //std::cout << std::endl;
            size_t finalTableIndex = createTableGraph(0, fullType);
            //std::cout << "Final table has index: " << finalTableIndex << std::endl;

            //std::cout << "Now, creating remaining connections: " << std::endl;
            TableType remainingTypes;
            remainingTypes.insert(removeTypes.begin(), std::next(it, 1).base());
            for (const ComponentID& componentID : remainingTypes) {
                //std::cout << "Component: " << componentID << " ";
            };
            //std::cout << std::endl;
            size_t missingTableIndex = createTableGraph(finalTableIndex, remainingTypes);
            //std::cout << "Missing link table index is: " << missingTableIndex << std::endl;
            _tables[currentTableIndex]._removeComponentGraph.emplace(*it, missingTableIndex);
            _tables[missingTableIndex]._addComponentGraph.emplace(*it, currentTableIndex);
            
            currentTableIndex = finalTableIndex;
        };

        return currentTableIndex;
    };


    void Database::moveToTable(EntityID entityID, size_t sourceTableIndex, size_t destTableIndex) {
        size_t currentEntityRowIndex = _tableRegistry.at(entityID).entityColumnIndex;
        Table& sourceTable = _tables[sourceTableIndex];
        Table& destTable = _tables[destTableIndex];
        
        // Add a block of size 1 to the list 
        //std::cout << "About to add to free block list " << std::endl;
        sourceTable.addToFreeBlockList(Table::ColumnBlock(currentEntityRowIndex, currentEntityRowIndex, 1));
        //std::cout << "Finished adding to free block list " << std::endl;

        if (currentEntityRowIndex == sourceTable._maxOccupiedRow - 1) {
            //std::cout << "Deleting the largest unused block at the end. " << std::endl;
            // TODO: should I use rbegin here?
            Table::FreeBlockList::iterator endBlock = --(sourceTable._freeBlockList.end());
            //std::cout << "Deleting from " << endBlock->startRow << " to " << endBlock->endRow << std::endl;
            sourceTable._maxOccupiedRow = endBlock->startRow;
            sourceTable._blockSizeMap.at(endBlock->size).erase(*endBlock);
            sourceTable._freeBlockList.erase(endBlock);
            //std::cout << "Finished deleting" << std::endl;
        };

        // get next row index for the entity 
        size_t freeRowIndex;
        if (destTable._freeBlockList.empty()) {
            freeRowIndex = destTable._maxOccupiedRow++;
            //std::cout << "Free row index: " << freeRowIndex << std::endl;
            // TODO: fix this 
        } else {
            // Always pick a block with the smallest size first
            // to combat fragmentation. Pick the lowest index block
            // from the smallest size table 
            Table::FreeBlockList::iterator blockIt = destTable._blockSizeMap.begin()->second.begin();
            Table::ColumnBlock newBlock(blockIt->startRow + 1, blockIt->endRow, blockIt->size - 1);
            freeRowIndex = blockIt->startRow;
            // Cleanup 
            destTable._freeBlockList.erase(*(blockIt));
            destTable._blockSizeMap.begin()->second.erase(blockIt);
            // TODO: this might be very slow, could modify the original
            // block in the list in place instead?
            destTable.addToFreeBlockList(newBlock);
        };

        // Now, update table registry
        addEntityToTableRegistry(entityID, destTableIndex, freeRowIndex);
        
        // If we are adding a new row and its index is greater than
        // the column capacity, we need to realloc all columns. 
        // TODO: this can also be vectorized with SIMD I think.
        // In this case we know for sure there is no fragmentation  
        if (freeRowIndex + 1 > destTable._capacity) {
            //std::cout << "Bigger than dest capacity during move " << std::endl;
            // everything needs to be resized in this case. 
            // By default, just double capacity.
            destTable._capacity = destTable._capacity * 2;
            for (Columns::iterator it = destTable._columns.begin(); it != destTable._columns.end(); it++) {
                it->second.data = static_cast<char *>(realloc(it->second.data, destTable._capacity * it->second.elementStride));
                
                if (it->second.data == nullptr) {
                    throw std::runtime_error("Ran out of memory, cannot increase row count for table.");
                };
            };
        };

        // Now perform data copies for overlapping components, 
        // with guaranteed space, first find overlapping components. 
        TableType overlappingColumns;
        // TODO: does this work if the set doesn't have capacity reserved for the overlapping elements? 
        std::set_intersection(sourceTable._type.begin(), sourceTable._type.end(), destTable._type.begin(), destTable._type.end(), std::inserter(overlappingColumns, overlappingColumns.begin()));
        // TODO: can I SIMD/vectorize this in any way?
        
        // Copy column content
        for (const ComponentID& componentID : overlappingColumns) {
            //std::cout << "Overlapping component ID: " << componentID << std::endl;
            size_t copySize = sourceTable._columns[componentID].elementStride;
            // Always deal with byte increments 
            void* copyFrom = static_cast<char*>(sourceTable._columns[componentID].data) + copySize * currentEntityRowIndex;
            void* copyTo = static_cast<char*>(destTable._columns[componentID].data) + copySize * freeRowIndex;
            std::memcpy(copyTo, copyFrom, copySize);
        };  

        //std::cout << "Finished moving entity to new table " << std::endl;
    };


    // This is immediate mode. 
    // For multiple components in quick succession,
    // use the deferred mode. 
    template <typename T>
    void Database::addComponent(EntityID entityID) {
        size_t currentTableIndex = _tableRegistry.at(entityID).tableIndex;
        //std::cout << "Type for current table: " << std::endl;
        //for (const ComponentID& componentID : _tables[currentTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        //}
        //std::cout << std::endl;
        // TODO: not sure if I should use the deferred version here 
        size_t nextTableIndex = findTableSuccessor(currentTableIndex, {getComponentID<T>()});
        //std::cout << "Next table index and type: " << nextTableIndex << std::endl;
        //for (const ComponentID& componentID : _tables[nextTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        //};
        //std::cout << std::endl;
        moveToTable(entityID, currentTableIndex, nextTableIndex);
        //std::cout << "Finished adding component " << std::endl;
    };

    template <class T> 
    void Database::removeComponent(EntityID entityID) noexcept {
        size_t currentTableIndex = _tableRegistry.at(entityID).tableIndex;
        //std::cout << "Current table index and type at remove: " << currentTableIndex << std::endl;
        for (const ComponentID& componentID : _tables[currentTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        };
        //std::cout << std::endl;
        size_t nextTableIndex = findTablePredecessor(currentTableIndex, {getComponentID<T>()});
        //std::cout << "Next table index and type at remove: " << nextTableIndex << std::endl;
        for (const ComponentID& componentID : _tables[nextTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        };
        //std::cout << std::endl;
        moveToTable(entityID, currentTableIndex, nextTableIndex);
    };

    template <class T>
    bool Database::hasComponent(EntityID entityID) noexcept {
        //size_t tableIndex = _tableRegistry[entityID].tableIndex;
        size_t tableIndex = _tableRegistry.at(entityID).tableIndex;
        //std::cout << "Entity's table index is: " << tableIndex << std::endl;
        const std::set<size_t>& typeSet = _componentRegistry.at(getComponentID<T>());
        //std::cout << "How many tables have component T " << typeSet.size() << std::endl;
        return typeSet.find(tableIndex) != typeSet.end();
    };

    template <class T>
    const T& Database::getComponent(EntityID entityID) noexcept {
        ComponentID componentID = getComponentID<T>();
        size_t tableIndex = _tableRegistry.at(entityID).tableIndex;
        size_t entityColumnIndex = _tableRegistry.at(entityID).entityColumnIndex;
        const Column& column = _tables[tableIndex]._columns.at(componentID);
        return *static_cast<T*>(static_cast<void*>(column.data + column.elementStride * entityColumnIndex));
    };

    template <class T>
    std::optional<T> Database::getComponentSafe(EntityID entityID) noexcept {
        if (!hasComponent<T>(entityID)) {
            return std::nullopt;
        } else {
            return std::optional<T>(getComponent<T>(entityID));
        };
    };
    // TODO: is it the most optimal to use New here?
    // Does this get freed properly? 
    template <typename T> 
    void Database::setComponentCopy(EntityID entityID, T& componentData) noexcept {
        ComponentID componentID = getComponentID<T>();
        size_t tableIndex = _tableRegistry.at(entityID).tableIndex;
        size_t entityColumnIndex = _tableRegistry.at(entityID).entityColumnIndex;
        Column& column = _tables[tableIndex]._columns.at(componentID);
        // This version uses the copy constructor. 
        void *address = static_cast<void *>(column.data + column.elementStride * entityColumnIndex);
        //std::cout << "Entity index: " << entityColumnIndex << std::endl;
        //std::cout << "Column capacity: " << _tables[tableIndex]._capacity << " and max occupied row: " << _tables[tableIndex]._maxOccupiedRow << ", and element size: " << column.elementStride << std::endl;
        //std::cout << "Address: " << address << std::endl;
        new(address) T(componentData);
        //*static_cast<T*>(static_cast<void *>(column.data + column.elementStride * entityColumnIndex)) = T(componentData); 
        /*std::vector<int>& obj_vec = reinterpret_cast<std::vector<int>&>(obj);
        std::vector<int>& data_vec = reinterpret_cast<std::vector<int>&>(componentData);
        //std::cout << data_vec.size() << std::endl;
        //std::cout << obj_vec.size() << std::endl;
        obj_vec.assign(data_vec.begin(), data_vec.end()); */
        // = componentData;
    };

    template <typename T>
    void Database::setComponentMove(EntityID entityID, T& componentData) noexcept {
        ComponentID componentID = getComponentID<T>();
        size_t tableIndex = _tableRegistry.at(entityID).tableIndex;
        size_t entityColumnIndex = _tableRegistry.at(entityID).entityColumnIndex;
        const Column& column = _tables[tableIndex]._columns.at(componentID);
        // This version uses the move constructor
        void *address = static_cast<void *>(column.data + column.elementStride * entityColumnIndex);
        new(address) T(std::move(componentData));  
        /*//std::cout << "Made it to here " << std::endl;
        //std::cout << "Size of T: " << sizeof(T) << std::endl;
        *static_cast<T*>(static_cast<void *>(column.data + column.elementStride * entityColumnIndex)) = std::move(componentData); */
    };

    void Database::flushDeferredNoDelete(EntityID entityID) {
        const DeferredPayload& entityPayload = _deferredPayloads[entityID];
        //std::cout << "Got deferred payload " << std::endl;
        // First, add all components
        TableType addType;
        TableType removeType;
        for (ComponentPayloadMap::const_iterator it = entityPayload._componentPayloadMap.begin(); it != entityPayload._componentPayloadMap.end(); ++it) {
            if (it->second == ComponentMode::Add) {
                addType.insert(it->first);
                //std::cout << "Inserted add component " << std::endl;
            } else {
                removeType.insert(it->first);
                //std::cout << "Inserted remove component " << std::endl;
            };
        };
        
        // TODO: this might be faster by just moving through the sorted 
        // list one by one and changing table until the final one is found 

        size_t currentTableIndex = _tableRegistry.at(entityID).tableIndex;
        //std::cout << "Current table index is: " << currentTableIndex << std::endl;
        
        size_t nextTableIndex = findTableSuccessor(currentTableIndex, addType);
        //std::cout << "Next table index after adding types is: " << nextTableIndex << std::endl;

        nextTableIndex = findTablePredecessor(nextTableIndex, removeType);
        //std::cout << "Next table index after removing types is: " << nextTableIndex << std::endl;
        
        moveToTable(entityID, currentTableIndex, nextTableIndex);

        // Now, apply all the set deferred operations 
        for (SetComponentMap::const_iterator it = entityPayload._setMap.begin(); it != entityPayload._setMap.end(); ++it) {
            if (it->second.mode == SetMode::Copy) {
                size_t tableIndex = _tableRegistry[entityID].tableIndex;
                size_t entityColumnIndex = _tableRegistry[entityID].entityColumnIndex;
                const Column& column = _tables[tableIndex]._columns[it->first];
                // This version uses the copy constructor. 
                //std::cout << "Performing memcopy for component: " << it->first << std::endl;
                memcpy(static_cast<char*>(column.data) + column.elementStride * entityColumnIndex, it->second.data, column.elementStride);
            } else {
                // TODO;
            };
        };
    };

    void Database::flushDeferred(EntityID entityID) {
        flushDeferredNoDelete(entityID);
        _deferredPayloads.erase(entityID);
    };

    void Database::flushDeferredAll() {
        for (std::map<EntityID, DeferredPayload>::const_iterator it = _deferredPayloads.begin(); it != _deferredPayloads.end(); it++) {
            // TODO: maybe should pass both here so I don't do another lookup in the function?
            flushDeferredNoDelete(it->first);
        };
        _deferredPayloads.clear();
    };

    template <typename T>
    void Database::addComponentDeferred(EntityID entityID) noexcept {
        _deferredPayloads[entityID]._componentPayloadMap.insert_or_assign(getComponentID<T>(), ComponentMode::Add);
        //std::cout << "Inserted add component with ID: " << getComponentID<T>() << std::endl;
    };

    template <class T> 
    void Database::removeComponentDeferred(EntityID entityID) noexcept {
        _deferredPayloads[entityID]._componentPayloadMap.insert_or_assign(getComponentID<T>(), ComponentMode::Remove);
        //std::cout << "Inserted remove component with ID: " << getComponentID<T>() << std::endl;
    };

    template <class T> 
    void Database::setComponentCopyDeferred(EntityID entityID, T* componentData) noexcept {
        // TODO: fix this 
        SetComponentPayload setPayload{};
        setPayload.data = static_cast<void *>(componentData);
        setPayload.mode = SetMode::Copy;
        _deferredPayloads[entityID]._setMap.insert_or_assign(getComponentID<T>(), setPayload);
    };

    template <class T>
    void Database::setComponentMoveDeferred(EntityID entityID, T* componentData) noexcept {
        _deferredPayloads[entityID]._setMap.insert_or_assign(getComponentID<T>(), static_cast<void *>(componentData), SetMode::Move);
    };

    void Database::cleanupMemory() {
        // Go over every table and every column of their table
        // resize columns to size of maxOccupiedRow
        // remove every free index bigger than or equal to max occupied 
        // row from their delete sets 

        // do this for every table except empty begin table..? 
        // TODO: need to make a map only with tables where capacity >= 
        // maxRow, and only those are viable for cleanup
        for (std::vector<Table>::iterator it = _tables.begin(); it != _tables.end(); ++it) {
            // TODO: maybe should also fix all fragmentation issues 
            // here. 
            // TODO: for fixing fragmentation, need to do
            // memcpy without overlapping memory ranges. 
            //std::cout << "Old capacity: " << it->_capacity << std::endl;
            //std::cout << "New capacity: " << it->_maxOccupiedRow << std::endl;
            for (Columns::iterator columnIt = it->_columns.begin(); columnIt != it->_columns.end(); ++columnIt) {
                columnIt->second.data = static_cast<char*>(realloc(columnIt->second.data, (it->_maxOccupiedRow + 1) * columnIt->second.elementStride));
                
                if (columnIt->second.data == nullptr) {
                    throw std::runtime_error("Could not reallocatee column data");
                };
            };

            // Now, remove from the delete list all rows >= maximum row.
            Table::DeleteList::iterator delListIt = it->deleteList.begin();
            while (delListIt != it->deleteList.end() && *(delListIt) >= it->_maxOccupiedRow) {
                ++delListIt;
            };

            if (delListIt == it->deleteList.end()) {
                // can clear the entire list
                it->deleteList.clear();
            } else {
                // delListIt points to the first element that's less than
                // max occupied row, need to delete everything from it 
                // to the beginning 
                it->deleteList.erase(it->deleteList.begin(), delListIt);
            };
        };
    };


    template <typename T>
    void Database::addRelationship(EntityID mainID, EntityID targetID) {
        ComponentID relationshipTypeID = getComponentID<T>();
        // generated component IDs go from 1..2^31 - 1, entity IDs go from 2^31 to 2^32 - 1
        // to get the relationship ID the entity ID is shifted left by 32 and ored with the 
        // component ID 
        ComponentID relationshipID = relationshipTypeID | (targetID << 32);
        // TODO this is slow 
        _componentSizes.insert_or_assign(relationshipID, 0);

        // Mostly the same as normal immediate component add 
        size_t currentTableIndex = _tableRegistry.at(mainID).tableIndex;
        //std::cout << "Type for current table: " << std::endl;
        //for (const ComponentID& componentID : _tables[currentTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        //}
        //std::cout << std::endl;
        // TODO: not sure if I should use the deferred version here 
        size_t nextTableIndex = findTableSuccessor(currentTableIndex, {relationshipID});
        //std::cout << "Next table index and type: " << nextTableIndex << std::endl;
        //for (const ComponentID& componentID : _tables[nextTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        //};
        //std::cout << std::endl;
        moveToTable(mainID, currentTableIndex, nextTableIndex);
        //std::cout << "Finished adding component " << std::endl;
        // TODO: see if all fo these can be static initialized somehow 
        if (_relationshipRegistry.find(relationshipTypeID) == _relationshipRegistry.end()) {
            _relationshipRegistry.emplace(relationshipTypeID, {targetID, {nextTableIndex}});
        } else {
            // Nasty 
            if (_relationshipRegistry.at(relationshipTypeID).find(targetID) != _relationshipRegistry.at(relationshipTypeID).end()) {
                _relationshipRegistry.at(relationshipTypeID).at(targetID).insert(nextTableIndex);
            } else {
                _relationshipRegistry.at(relationshipTypeID).emplace(targetID, {nextTableIndex});
            };
        };

        // Now add the inverse one 
        if (_inverseTargetRegistry.find(targetID) == _inverseTargetRegistry.end()) {
            _inverseTargetRegistry.emplace(targetID, {relationshipTypeID, {nextTableIndex}});
        } else {
            if (_inverseTargetRegistry.at(targetID).find(relationshipTypeID) != _inverseTargetRegistry.at(targetID).end()) {
                _inverseTargetRegistry.at(targetID).at(relationshipTypeID).insert(nextTableIndex);
            } else {
                _inverseTargetRegistry.at(targetID).emplace(relationshipTypeID, {nextTableIndex});
            };
        };
        
        // Finally, for the component registry
        if (_relationshipCountRegistry.find(relationshipTypeID) != _relationshipCountRegistry.end()) {
            _relationshipCountRegistry.at(relationshipTypeID).insert(nextTableIndex);
        } else {
            _relationshipCountRegistry.emplace(relationshipTypeID, {nextTableIndex});
        };
    };

    template <typename T>
    void Database::removeRelationship(EntityID mainID, EntityID targetID) noexcept {
        ComponentID relationshipTypeID = getComponentID<T>();
        // generated component IDs go from 1..2^31 - 1, entity IDs go from 2^31 to 2^32 - 1
        // to get the relationship ID the entity ID is shifted left by 32 and ored with the 
        // component ID 
        ComponentID relationshipID = relationshipTypeID | (targetID << 32);

        size_t currentTableIndex = _tableRegistry.at(mainID).tableIndex;
        //std::cout << "Current table index and type at remove: " << currentTableIndex << std::endl;
        //for (const ComponentID& componentID : _tables[currentTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        //};
        //std::cout << std::endl;
        size_t nextTableIndex = findTablePredecessor(currentTableIndex, {relationshipID});
        //std::cout << "Next table index and type at remove: " << nextTableIndex << std::endl;
        //for (const ComponentID& componentID : _tables[nextTableIndex]._type) {
            //std::cout << "Component: " << componentID << " ";
        //};
        //std::cout << std::endl;
        moveToTable(mainID, currentTableIndex, nextTableIndex);
    };


    template <typename T>
    std::set<EntityID> Database::getRelationshipTargets(EntityID entityID) {

    };

    template <typename T>
    std::set<EntityID> Database::getAllRelationshipTargets() {

    };

    template <typename T>
    std::set<EntityID> Database::getRelationshipHolders(EntityID targetID) {

    };

    template <typename T>
    std::set<EntityID> Database::getAllRelationshipHolders() {

    };
};

#endif