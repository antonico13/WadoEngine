#ifndef WADO_ECS_H
#define WADO_ECS_H

//#include "MainClonePtr.h"

#include <iostream>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>
#include <algorithm>
#include <typeindex>

namespace Wado::ECS {

    using EntityID = uint64_t;
    using ComponentID = uint64_t;

    struct Entity {
            friend class Database;
            // Destroys entity. The entity's ID will be added 
            // to a pool of reusable IDs after this operation. 
            void destroy(); 

            // Getters, setters and verifiers act the same way as
            // the main database functions. Check the Database class
            // for more in-depth comments. 
            template <class T> 
            void addComponent() noexcept;

            template <class T> 
            void setComponent(T componentData) noexcept;

            template <class T> 
            void setComponent(T& componentData) noexcept;

            template <class T> 
            void removeComponent() noexcept;

            // TODO: how to make these const??
            template <class T>
            const T& getComponent() noexcept;

            template <class T>
            std::optional<T> getComponentSafe() noexcept;
            
            template <class T>
            bool hasComponent() noexcept;

            EntityID getID() const noexcept;
        private:
            Entity(EntityID entityID, Database* database) : _entityID(entityID), _database(database) { };
            const EntityID _entityID;
            // store raw pointer to DB for operations like 
            // adding or setting components 
            Database* _database; 
    };

    class Database {
        private:

            using ComponentInfo = struct ComponentInfo {
                ComponentInfo(const ComponentID _componentID, const size_t _elementSize) noexcept : componentID(_componentID), elementSize(_elementSize) {};
                const ComponentID componentID;
                const size_t elementSize;
            };

            using ComponentInfoMap = std::unordered_map<std::type_index, ComponentInfo>;
            static ComponentInfoMap _componentInfoMap;

            using ComponentSizes = std::unordered_map<ComponentID, size_t>;
            static ComponentSizes _componentSizes;

            // The lower 32 bits of an entity ID 
            // are used for the actual ID. The higher 32 bits will be used 
            // for various flags and features such as entity relationships,
            // tags, and more.
            static ComponentID COMPONENT_ID;
            EntityID ENTITY_ID = (uint64_t)1 << 31;
            static const uint64_t ENTITY_INCREMENT = 1;
            static const uint64_t COMPONENT_INCREMENT = 1;
            static const uint64_t MAX_ENTITY_ID = (uint64_t)1 << 32;
            static const uint64_t MAX_COMPONENT_ID = (uint64_t)1 << 31;
            static const uint64_t ENTITY_ID_MASK = ((uint64_t)1 << 32) - 1;

            const size_t _defaultColumnSize;

            // TODO: this is heap managed by default,
            // but reusable IDs could live on the stack maybe?
            std::vector<EntityID> _reusableEntityIDs;
            
            // Table type needs to be an ordered set to facilitate adding components 
            using TableType = std::set<ComponentID>; 

            using Column = struct Column {
                Column() noexcept : _data(nullptr), _elementSize(0) {};
                Column(char* data, size_t elementSize) noexcept : _data(data), _elementSize(elementSize) {};
                // TODO: when freeing this, the object destructor should somehow be called/stored 
                // Raw data pointer, let these be managed by the ECS instead of using 
                // memory/clone pointers
                char* _data; 
                const size_t _elementSize; // element stride in bytes  
            };

            using Columns = std::unordered_map<ComponentID, Column>;

            using Table = struct Table {
                Table(const TableType& type, const size_t defaultColumnSize) : _type(type), _maxComponentID(*(type.rbegin())), _capacity(defaultColumnSize), _maxOccupiedRow(0) {
                    for (const ComponentID& componentID : type) {
                        size_t elementSize = Database::getComponentSize(componentID);
                        // TODO: instead of malloc, use block allocator here 
                        char* columnData = static_cast<char*>(malloc(elementSize * defaultColumnSize));
                        
                        if (columnData == nullptr) {
                            throw std::runtime_error("Ran out of memory for storing column data");
                        };
                        _columns.emplace(std::piecewise_construct, std::forward_as_tuple(componentID), std::forward_as_tuple(columnData, elementSize));
                    };
                };

                // Empty table constructor, should only be used 
                // for the first table 
                Table() noexcept : _type({}), _maxComponentID(0), _capacity(DEFAULT_COLUMN_SIZE), _maxOccupiedRow(0) { };

                const TableType _type;
                const ComponentID _maxComponentID;
                Columns _columns;

                using ColumnBlock = struct ColumnBlock {
                    ColumnBlock(size_t startRow, size_t endRow, size_t size) noexcept : _startRow(startRow), _endRow(endRow), _size(size) {};
                    const size_t _startRow;
                    const size_t _endRow;
                    const size_t _size;
                };

                struct FreeBlockListComp {
                    bool operator() (const ColumnBlock& b1, const ColumnBlock& b2) const {
                        return b1._startRow < b2._startRow;
                    };
                };

                using FreeBlockList = std::set<ColumnBlock, FreeBlockListComp>;
                using FreeBlockSizeMap = std::unordered_map<size_t, FreeBlockList>;

                FreeBlockList _freeBlockList;
                FreeBlockSizeMap _blockSizeMap;
                // TODO: change so these can be batched as well 
                inline void addToFreeBlockList(const ColumnBlock& block) noexcept {
                    if (_freeBlockList.empty()) {
                        _blockSizeMap.emplace(block._size, FreeBlockList{block});
                        _freeBlockList.insert(block);
                        return;
                    };

                    // TODO: Should this be a read iterator only?
                    FreeBlockList::const_iterator upperBound = _freeBlockList.upper_bound(block);

                    size_t startRow = block._startRow;
                    size_t endRow = block._endRow;
                    
                    if (upperBound != _freeBlockList.begin()) {
                        FreeBlockList::const_iterator prevBound = std::prev(upperBound, 1);
                        if (prevBound->_endRow + 1 == block._startRow) {
                            startRow = prevBound->_startRow;
                            _blockSizeMap.at(prevBound->_size).erase((*prevBound));
                            _freeBlockList.erase(prevBound);
                        };
                    };

                    if (upperBound != _freeBlockList.end()) {
                        if (upperBound->_startRow == block._endRow + 1) {
                            endRow = upperBound->_endRow;
                            _blockSizeMap.at(upperBound->_size).erase((*upperBound));
                            _freeBlockList.erase(upperBound);
                        };
                    };

                    ColumnBlock newBlock(startRow, endRow, endRow - startRow + 1);
                    
                    _freeBlockList.emplace(startRow, endRow, endRow - startRow + 1);

                    if (_blockSizeMap.count(newBlock._size) == 0) {
                        // TODO: does this work?
                        _blockSizeMap.emplace(std::piecewise_construct, std::forward_as_tuple(newBlock._size), std::forward_as_tuple());
                        _blockSizeMap.at(newBlock._size).emplace(startRow, endRow, endRow - startRow + 1);
                        //_blockSizeMap.emplace(std::piecewise_construct, std::forward_as_tuple(newBlock._size), std::forward_as_tuple(newBlock));
                    } else {
                        _blockSizeMap.at(newBlock._size).emplace(startRow, endRow, endRow - startRow + 1);
                    };
                };

                size_t _maxOccupiedRow;
                size_t _capacity;
                // Traversing the add/remove component graphs gives a
                // new table that has overlapping components with
                // the current table +/- the key component ID.
                using TableEdges = std::unordered_map<ComponentID, size_t>;

                TableEdges _addComponentGraph;
                TableEdges _removeComponentGraph;
            };  

            using TableIndex = struct TableIndex {
                TableIndex(Table& table, const size_t tableIndex, const size_t entityRowIndex) noexcept : _table(table), _tableIndex(tableIndex), _entityRowIndex(entityRowIndex) { };
                
                // TODO: 128 bits repeated for every row of this table...
                Table& _table; 
                const size_t _tableIndex;
                const size_t _entityRowIndex;
            };

            std::vector<Table> _tables; // Keep a record of tables so they can be
            // cleaned up later. 

            // vector of main pointer for entities. The lifetime of 
            // these entities is managed by the database, so all
            // the main pointers need to be kept track of. 
            //std::vector<Memory::WdMainPtr<Entity>> _entityMainPtrs;
            
            using TableRegistry = std::unordered_map<EntityID, TableIndex>;
            TableRegistry _tableRegistry;

            using TableSet = std::unordered_set<size_t>;

            // Used for fast inverse look-ups, for example: getting all tables
            // that have a specific component
            using ComponentRegistry = std::unordered_map<ComponentID, TableSet>;
            ComponentRegistry _componentRegistry;

            // Keep a record of every table thart has a specific relation ship type.
            // (i.e: Likes, childOf)
            using RelationshipTypeRegistry = std::unordered_map<ComponentID, TableSet>;
            RelationshipTypeRegistry _relationshipCountRegistry;

            // Relationship indexing
            // With this registry, it can be verified whether an entity is the target
            // of a relationship type, and which tables have the component (relationship type, Target entity).
            using TargetRegistry = std::unordered_map<EntityID, TableSet>;
            using RelationshipRegistry = std::unordered_map<ComponentID, TargetRegistry>;
            RelationshipRegistry _relationshipRegistry;

            // Support inverse lookup too.
            // For an entity, query which relationship types it is a target of, and then 
            // which tables for each relationship type
            using InverseRelationshipRegistry = std::unordered_map<ComponentID, TableSet>;
            using InverseTargetRegistry = std::unordered_map<EntityID, InverseRelationshipRegistry>;
            InverseTargetRegistry _inverseTargetRegistry;

            // Deferred commands data types
            // Deferred commands use raw pointers only, since
            // it's impossible as far as I know to use 
            // generic references to hold data like this
            // TODO: could solve this implicitly with function pointers 
            // to the specialized versions 
            
            enum SetMode {
                Copy,
                Move
            };

            enum ComponentMode {
                Add,
                Remove,
            };

            using SetComponentPayload = struct SetComponentPayload {
                void* data;
                SetMode mode;
            };
            using SetComponentMap = std::unordered_map<ComponentID, SetComponentPayload>;
            using ComponentPayloadMap = std::unordered_map<ComponentID, ComponentMode>;

            using DeferredPayload = struct DeferredPayload {
                // TODO: could this be faster if instead 
                // I stored per component whether it's an Add/Remove?
                SetComponentMap _setMap; 
                ComponentPayloadMap _componentPayloadMap;
            };

            std::unordered_map<EntityID, DeferredPayload> _deferredPayloads;


        public:
            
            class Query {
                public:
                    friend class Database;

                    ~Query() {
                        // nothing for now?
                    };

                    struct FullIterator {
                        public:
                            FullIterator(size_t tableIndex, size_t rowIndex, Query& query) : 
                                _tableIndex(tableIndex), 
                                _rowIndex(rowIndex),
                                _query(query) { };

                            template <typename T>
                            T& operator*() {
                                // TODO: make all of these static, dereference is too slow
                                ComponentID columnID = Database::getComponentIDUnsafe<T>();
                                Database::Column* columnData = _query._tables[_tableIndex]._columnData.at(columnID);
                                return *static_cast<T*>(static_cast<void *>(columnData->_data + columnData->_elementSize * _rowIndex));
                            };

                            template <typename T>
                            T* operator->() { 
                                // TODO: make all of these static, dereference is too slow
                                ComponentID columnID = Database::getComponentIDUnsafe<T>();
                                Database::Column* columnData = _query._tables[_tableIndex]._columnData.at(columnID);
                                return static_cast<T*>(static_cast<void *>(columnData->data + columnData->elementStride * _rowIndex));
                            };

                            // Prefix increment
                            FullIterator& operator++() {
                               //std::cout << "increment" << std::endl;
                                _rowIndex++;
                                if (_rowIndex == _query._tables[_tableIndex]._elementCount) {
                                    _rowIndex = 0;
                                    _tableIndex++;
                                };
                                //std::cout << "Row index: " << _rowIndex << std::endl;
                                //std::cout << "Table index: " << _tableIndex << std::endl;
                                return *this;
                            };

                            // Postfix increment
                            FullIterator operator++(int) { 
                                FullIterator current = *this; 
                                ++(*this); 
                                return current; 
                            };

                            friend bool operator== (const FullIterator& obj, const FullIterator& other) { 
                                if (obj._query != other._query) {
                                    return false;
                                };

                                if (obj._tableIndex != other._tableIndex) {
                                    return false;
                                };

                                return obj._rowIndex == other._rowIndex;
                            };

                            friend bool operator!= (const FullIterator& obj, const FullIterator& other) {
                                if (obj._query != other._query) {
                                    //std::cout << "Not equal to end" << std::endl;
                                    return true;
                                };

                                if (obj._tableIndex != other._tableIndex) {
                                    //std::cout << "Not equal to end" << std::endl;
                                    return true;
                                };
                                //std::cout << "Checking if row is equal" << std::endl;
                                return obj._rowIndex != other._rowIndex;
                            }; 


                        private:
                            size_t _tableIndex;
                            size_t _rowIndex;
                            Query& _query;
                    };

                    friend bool operator== (const Query& obj, const Query& other) { 
                        return obj._queryID == other._queryID;
                    };

                    friend bool operator!= (const Query& obj, const Query& other) { 
                        return obj._queryID != other._queryID;
                    };

                    FullIterator begin() { 
                        return FullIterator(0, 0, *this); 
                    };

                    FullIterator end() { 
                        return FullIterator(_tables.size(), 0, *this); 
                    };

                private:

                    using QueryTable = struct QueryTable {
                        QueryTable(const std::unordered_map<ComponentID, Database::Column*>& columnData, const std::vector<EntityID>& entityIDs, const size_t elementCount, const size_t databaseTableIndex) : 
                            _columnData(columnData),
                            _entityIDs(entityIDs),
                            _elementCount(elementCount),
                            _dbTableIndex(databaseTableIndex) { };
                        // TODO: should this maybe be a sparse set?
                        // we know at construction time how many component IDs
                        // we have, and the max value.
                        // but it might be way too big actually 
                        const std::unordered_map<ComponentID, Database::Column*> _columnData;
                        const std::vector<EntityID> _entityIDs;
                        const size_t _elementCount;
                        const size_t _dbTableIndex;
                    };

                    Query(const std::vector<QueryTable>& tables, const size_t queryID, Database* database) : 
                        _queryID(queryID),
                        _tables(tables),
                        _db(database)  {

                    };

                    // db pointer for destruct and component IDs
                    Database* _db;
                    // TODO: should this be const?
                    std::vector<QueryTable> _tables;
                    const size_t _queryID;
            };
            enum QueryBuildMode {
                Transient,
                Cached,
            };

            class QueryBuilder {
                public:
                    friend class Database;
                    enum ConditionOperator {
                        None,
                        Not,
                        // Or and And are unused for now 
                        Or,
                        And,
                    };

                    // Works for any component, tag and even 
                    // a relationship pair if made from the 
                    // database. 

                    // TODO: add relationship pair constructor to DB
                    template <typename T>
                    QueryBuilder& componentCondition(const ConditionOperator op = ConditionOperator::None) {
                        if (op == ConditionOperator::None) {
                            _requiredComponents.insert(Database::getComponentIDUnsafe<T>());
                        } else {
                            _bannedComponents.insert(Database::getComponentIDUnsafe<T>());
                        };
                        return *this;
                    };
                    
                    // Determines the actual data that will be 
                    // in the iterator of the final query, implicitly
                    // defines a component condition 
                    template <typename T>
                    QueryBuilder& withComponent() {
                        ComponentID id = Database::getComponentIDUnsafe<T>();
                        _requiredComponents.insert(id);
                        _getComponents.insert(id);
                        return *this;
                    };

                    // Equivalent to checking if a relationship 
                    // actually exists 
                    template <typename T>
                    QueryBuilder& relationshipCondition(const EntityID targetEntity = 0, const ConditionOperator op = ConditionOperator::None) {
                        // TODO: magic number... 
                        if (targetEntity == 0) {
                            if (op == ConditionOperator::None) {
                                _requiredRelationshipsHolder.insert(Database::getComponentIDUnsafe<T>());
                            } else {
                                _bannedRelationshipsHolder.insert(Database::getComponentIDUnsafe<T>());
                            };
                        } else {
                            ComponentID relationshipID = _db.makeRelationshipPair<T>(targetEntity);
                            if (op == ConditionOperator::None) {
                                _requiredRelationshipsTargets.insert(relationshipID);
                            } else {
                                _bannedRelationshipsTargets.insert(relationshipID);
                            };
                        };
                        return *this;
                    };

                    
                    // Entity ID of the relationship target will be 
                    // included in the iterator 
                    // implicitly defines a relationship condition 
                    template <typename T>
                    QueryBuilder& withRelationshipTarget(const EntityID targetEntity) {
                        ComponentID relationshipID = _db.makeRelationshipPair<T>(targetEntity);
                        _requiredRelationshipsTargets.insert(relationshipID);
                        _getRelationships.insert(relationshipID);
                        return *this;
                    };

                    // condition that specifies the entity you're 
                    // querying is or is not a target of the relationship
                    template <typename T>
                    QueryBuilder& relationshipTargetCondition(const ConditionOperator op = ConditionOperator::None) {
                        ComponentID relationshipID = Database::getComponentIDUnsafe<T>();
                        if (op == ConditionOperator::None) {
                            _requiredRelationshipsAsTarget.insert(relationshipID);
                        } else {
                            _bannedRelationshipsAsTarget.insert(relationshipID);
                        };
                        return *this;
                    };

                    Query build() {
                        return _db->buildQuery(*this);
                    };

                    ~QueryBuilder() {};

                private: 
                    QueryBuilder(const Database::QueryBuildMode buildMode, Database* database) : _buildMode(buildMode), _db(database) {};
                    const Database::QueryBuildMode _buildMode;
                    Database* _db;
                    
                    // Pure components and tags 
                    std::unordered_set<ComponentID> _requiredComponents;
                    // These only include relationships + targets
                    std::unordered_set<ComponentID> _requiredRelationshipsTargets;
                    // Relationships for which the entity must be a holder of
                    // with any target 
                    std::unordered_set<ComponentID> _requiredRelationshipsHolder;
                    // Relationships for which the entity must be a target
                    std::unordered_set<ComponentID> _requiredRelationshipsAsTarget;
                    // Components that must not be in an entity's row
                    std::unordered_set<ComponentID> _bannedComponents;
                    // Relationships + targets that must not be in an 
                    // entity's row 
                    std::unordered_set<ComponentID> _bannedRelationshipsTargets;
                    // Relationships for which the entity must not be a 
                    // holder of with any target 
                    std::unordered_set<ComponentID> _bannedRelationshipsHolder;
                    // Relationships for which the entity must not be a target
                    std::unordered_set<ComponentID> _bannedRelationshipsAsTarget;

                    // Components we want to get the values of 
                    std::unordered_set<ComponentID> _getComponents;
                    // Relationships we want to get the target of 
                    std::unordered_set<ComponentID> _getRelationships;
            };

            friend class QueryBuilder;
            friend class Query;

            static const size_t DEFAULT_COLUMN_SIZE = 8;
            Database(size_t defaultColumnSize = DEFAULT_COLUMN_SIZE) : _defaultColumnSize(defaultColumnSize) {
                // Create the empty table and place it in the 
                // table vector, and initalize the empty table 
                // variable, empty table will always be the 0th
                // element of this collection
                _tables.emplace_back();
            };

            ~Database() {
                // Go over every table and free the data in every column of the table. 
                for (Table& table : _tables) {
                    for (Columns::iterator it = table._columns.begin(); it != table._columns.end(); ++it) {
                        free(it->second._data);
                    };
                };
            };

            // Multiple ways of creating entities

            // These functions will throw errors if and only if 
            // the pool of available IDs has been fully consumed. E.g: no reusable
            // IDs, and 2^31 entities created. 

            // Creating a pointer entity means the lifetime is managed 
            // by the database. This means entities don't need to be manually
            // destructed but incurs more of a performance cost as each 
            // operation requires 3 dereferences. First, to check 
            // if the main pointer is alive, then dereferencing the 
            // actual entity pointer, then using the database pointer for 
            // operations such as set/add component
            //Memory::WdClonePtr<Entity> createEntityClonePtr();
            // Using entity objects, the lifetime is managed by the caller of
            // this function. This only requires one pointer dereference for
            // database operations. 
            
            Entity createEntityObj() {
                return Entity(generateNewIDAndAddToEmptyTableRegistry(), this);
            };

            // Highest performance option. Lifetime is still managed manually
            // by the caller, but all operations require a database object 
            // on which to call add, set component, etc. without any 
            // pointer dereferences 
            EntityID createEntityID() {
                return generateNewIDAndAddToEmptyTableRegistry();
            };

            // Passing invalid entity IDs leads to undefined behaviour
            // This is a deferred function by default. To actually clear
            // the memory used by this entity, cleanupMemory needs to
            // be called. 

            // TODO: all entity destroys should be deferred 
            void destroyEntity(EntityID entityID) noexcept {
                // Add a block of size 1 to the list 
                TableIndex& tableIndex = _tableRegistry.at(entityID);
                tableIndex._table.addToFreeBlockList(Table::ColumnBlock(tableIndex._entityRowIndex, tableIndex._entityRowIndex, 1));

                //std::cout << "Free block list looks like: " << std::endl;
                //for (const Table::ColumnBlock& freeBlock : tableIndex._table._freeBlockList) {
                    //std::cout << "[" << freeBlock.startRow << ", " << freeBlock.endRow << "]" << ", ";
               // };
                //std::cout << std::endl;

                if (tableIndex._entityRowIndex == tableIndex._table._maxOccupiedRow -  1) {
                    //std::cout << "Deleting the largest unused block at the endl;. " << std::e ndl;
                    Table::FreeBlockList::iterator endBlock = --(tableIndex._table._freeBlockList.end());
                    //std::cout << "Deleting from " << endBlock->startRow << " to " << endBlock->endRow << std::endl;
                    tableIndex._table._maxOccupiedRow = endBlock->_startRow;
                    tableIndex._table._blockSizeMap.at(endBlock->_size).erase(*endBlock);
                    tableIndex._table._freeBlockList.erase(endBlock);
                    //std::cout << "Finished deleting" << std::endl;
                };

                //std::cout << "Free block list looks like: " << std::endl;
                //for (const Table::ColumnBlock& freeBlock : tableIndex._table._freeBlockList) {
                    //std::cout << "[" << freeBlock.startRow << ", " << freeBlock.endRow << "]" << ", ";
                //};
                //std::cout << std::endl;

                _tableRegistry.erase(entityID);
                _reusableEntityIDs.emplace_back(entityID);
            };

            // adds a component to an entity based on its ID. The component is
            // "empty" by default, since the underlying type is erased 
            // in the container of the database. This component will be 
            // by default a contiguous array of bytes the size of 
            // the component all set to 0. 
            // Warning: only works with valid IDs produced by the database, 
            // undefined behaviour if the ID supplied is not valid. 
            template <typename T> 
            void addComponent(EntityID entityID) {
                size_t currentTableIndex = _tableRegistry.at(entityID)._tableIndex;
                //std::cout << "Type for current table: " << std::endl;
                //for (const ComponentID& componentID : _tables[currentTableIndex]._type) {
                    //std::cout << "Component: " << componentID << " ";
                //}
                //std::cout << std::endl;
                // TODO: not sure if I should use the deferred version here 
               //std::cout << "Requested ID: " << getComponentIDOrInsert<T>() << std::endl;
                size_t nextTableIndex = findTableSuccessor(currentTableIndex, {getComponentIDOrInsert<T>()});
                //std::cout << "Next table index and type: " << nextTableIndex << std::endl;
                //for (const ComponentID& componentID : _tables[nextTableIndex]._type) {
                    //std::cout << "Component: " << componentID << " ";
                //};
                //std::cout << std::endl;
                moveToTable(entityID, currentTableIndex, nextTableIndex);
                //std::cout << "Finished adding component " << std::endl;
            };

            template <typename T>
            ComponentID makeRelationshipPair(EntityID targetID) {
                ComponentID relationshipTypeID = getComponentIDOrInsert<T>();
                // generated component IDs go from 1..2^31 - 1, entity IDs go from 2^31 to 2^32 - 1
                // to get the relationship ID the entity ID is shifted left by 32 and ored with the 
                // component ID 
                ComponentID relationshipID = relationshipTypeID | (targetID << 32);
                return relationshipID;
            };

            template <typename T>
            void addRelationship(EntityID mainID, EntityID targetID) {
                ComponentID relationshipTypeID = getComponentIDOrInsert<T>();
                // generated component IDs go from 1..2^31 - 1, entity IDs go from 2^31 to 2^32 - 1
                // to get the relationship ID the entity ID is shifted left by 32 and ored with the 
                // component ID 
                ComponentID relationshipID = relationshipTypeID | (targetID << 32);
                // TODO this is slow 
                _componentSizes.insert_or_assign(relationshipID, 0);

                // Mostly the same as normal immediate component add 
                size_t currentTableIndex = _tableRegistry.at(mainID)._tableIndex;
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
                // TODO: see if all fo these can be static initialized some how 
                if (_relationshipRegistry.find(relationshipTypeID) == _relationshipRegistry.end()) {
                    TargetRegistry registry = TargetRegistry();
                    registry.emplace(targetID, std::unordered_set<size_t>({nextTableIndex}));
                    _relationshipRegistry.emplace(relationshipTypeID, registry);
                } else {
                    // Na sty 
                    if (_relationshipRegistry.at(relationshipTypeID).find(targetID) != _relationshipRegistry.at(relationshipTypeID).end()) {
                        _relationshipRegistry.at(relationshipTypeID).at(targetID).insert(nextTableIndex);
                    } else {
                        _relationshipRegistry.at(relationshipTypeID).emplace(targetID, std::unordered_set<size_t>({nextTableIndex}) );
                    };
                };

                // Now add the inverse  one 
                if (_inverseTargetRegistry.find(targetID) == _inverseTargetRegistry.end()) {
                    InverseRelationshipRegistry inverseRegistry = InverseRelationshipRegistry();
                    inverseRegistry.emplace(relationshipTypeID, std::unordered_set<size_t>({nextTableIndex}));
                    _inverseTargetRegistry.emplace(targetID, inverseRegistry);
                } else {
                    if (_inverseTargetRegistry.at(targetID).find(relationshipTypeID) != _inverseTargetRegistry.at(targetID).end()) {
                        _inverseTargetRegistry.at(targetID).at(relationshipTypeID).insert(nextTableIndex);
                    } else {
                        _inverseTargetRegistry.at(targetID).emplace(relationshipTypeID, std::unordered_set<size_t>({nextTableIndex}));
                    };
                };
                
                // Finally, for the component regi stry
                if (_relationshipCountRegistry.find(relationshipTypeID) != _relationshipCountRegistry.end()) {
                    _relationshipCountRegistry.at(relationshipTypeID).insert(nextTableIndex);
                } else {
                    _relationshipCountRegistry.emplace(relationshipTypeID, std::unordered_set<size_t>({nextTableIndex}));
                };
            };

            template <typename T>
            void removeRelationship(EntityID mainID, EntityID targetID) noexcept {
                ComponentID relationshipTypeID = getComponentIDUnsafe<T>();
                // generated component IDs go from 1..2^31 - 1, entity IDs go from 2^31 to 2^32 - 1
                // to get the relationship ID the entity ID is shifted left by 32 and ored with the 
                // component ID 
                ComponentID relationshipID = relationshipTypeID | (targetID << 32);

                size_t currentTableIndex = _tableRegistry.at(mainID)._tableIndex;
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
            bool hasRelationship(EntityID mainId, EntityID targetID) noexcept {
                // TODO: put this in a function, using it quite often 
                ComponentID relationshipTypeID = getComponentIDUnsafe<T>();
                // generated component IDs go from 1..2^31 - 1, entity IDs go from 2^31 to 2^32 - 1
                // to get the relationship ID the entity ID is shifted left by 32 and ored with the 
                // component ID 
                ComponentID relationshipID = relationshipTypeID | (targetID <<  32);
                if (_componentRegistry.find(relationshipID) != _componentRegistry.end( )) {
                    return _componentRegistry.at(relationshipID).find(_tableRegistry.at(mainId)._tableIndex) != _componentRegistry.at(relationshipID).end();
                };
                return false;
            };
            
            // Gets all the targets the specified entity has
            // for the given relationship 
            template <typename T>
            std::unordered_set<EntityID> getRelationshipTargets(EntityID entityID) {
                const ComponentID relationshipTypeID = getComponentIDUnsafe<T>();
                const Table& table = _tables[_tableRegistry.at(entityID)._tableIndex];
                std::unordered_set<EntityID> targets;
                for (const ComponentID& componentID : table._type) {
                    // Has the full ID
                    if (componentID & relationshipTypeID == relationshipTypeID) {
                        // TODO: replace all these magic numbers 
                        targets.insert((componentID >> 32));
                    };
                };
                return targets;
            };

            // Gets all targets for the given relationship 
            template <typename T>
            std::unordered_set<EntityID> getAllRelationshipTargets() {
                const ComponentID relationshipTypeID = getComponentIDUnsafe<T>();
                std::unordered_set<EntityID> targetIDs;
                for (TargetRegistry::const_iterator it = _relationshipRegistry.at(relationshipTypeID).begin(); it != _relationshipRegistry.at(relationshipTypeID).end(); ++it) {
                    targetIDs.insert(it->first);
                }
                return targetIDs;
            };

            // Gets all the "main" entities given 
            // a relationship and a target 
            template <typename T>
            std::unordered_set<EntityID> getRelationshipHolders(EntityID targetID) {
                // TOOD
            };

            // Gets all the "main" entities for
            // a given relationship 
            template <typename T>
            std::unordered_set<EntityID> getAllRelationshipHolders() {
                // TODO
            };
            // TODO: all sets should also add the component 
            template <typename T>
            void setRelationshipCopy(EntityID mainID, EntityID targetID, T& relationshipData);

            template <typename T>
            void setRelationshipMove(EntityID mainID, EntityID targetID, T& relationshipData);
            // TODO: add deferred relationship commands 

            // sets an entity's component values. This uses the copy constructor of
            // the class.
            // Warning: set and remove component never check if the component
            // already exists. Remove component will silently exit,
            // while set component have undefined behaviour. 

            // TODO: is it the most optimal to use New here?
            // Does this get freed properly? 
            template <typename T> 
            void setComponentCopy(EntityID entityID, T& componentData) noexcept {
                ComponentID componentID = getComponentIDUnsafe<T>();
                //std::cout << "Setting component for copy " << std::endl;
                size_t tableIndex = _tableRegistry.at(entityID)._tableIndex;
                size_t entityColumnIndex = _tableRegistry.at(entityID)._entityRowIndex;
                Column& column = _tables[tableIndex]._columns.at(componentID);
                // This version uses the copy constructor. 
                void *address = static_cast<void *>(column._data + column._elementSize * entityColumnIndex);
                //std::cout << "Entity index: " << entityColumnIndex << std::endl;
                //std::cout << "Column capacity: " << _tables[tableIndex]._capacity << " and max occupied row: " << _tables[tableIndex]._maxOccupiedRow << ", and element size: " << column.elementStride << std::endl;
                //std::cout << "Address: " << address << std::endl;
                new(address) T(componentData);
                //*static_cast<T*>(static_cast<void *>(column.data + column.elementStride * entityColumnIndex)) = T(componentData); 
                /*std::vector<int>& obj_vec = reinterpret_cast<std::vector<int>&>(obj);
                std::vector<int>& data_vec = reinterpret_cast<std::vector<int>&>(componentData);
                //std::cout << data_vec.size() << std::endl;
                //std::cout << obj_vec.size() << std::e ndl;
                obj_vec.assign(data_vec.begin(), data_vec.end()); */
                // = componentData;
            };

            // sets an entity's component values. However, be careful as this
            // uses the move constructor, and the value passed in becomes invalid 
            // in the caller's scope.
            template <typename T>
            void setComponentMove(EntityID entityID, T& componentData) noexcept {
                ComponentID componentID = getComponentIDUnsafe<T>();
                size_t tableIndex = _tableRegistry.at(entityID)._tableIndex;
                size_t entityColumnIndex = _tableRegistry.at(entityID)._entityRowIndex;
                const Column& column = _tables[tableIndex]._columns.at(componentID);
                // This version uses the move constructor
                void *address = static_cast<void *>(column._data + column._elementSize * entityColumnIndex);
                new(address) T(std::move(componentData));  
                /*//std::cout << "Made it to here " << std::endl;
                //std::cout << "Size of T: " << sizeof(T) << std::endl;
                *static_cast<T*>(static_cast<void *>(column.data + column.elementStride * entityColumnIndex)) = std::move(componentData); */
            };

            // removes a component to an entity based on its ID.
            // Undefined behaviour if the entity doesn't have
            // the component. 
            template <class T> 
            void removeComponent(EntityID entityID) noexcept {
                size_t currentTableIndex = _tableRegistry.at(entityID)._tableIndex;
                //std::cout << "Current table index and type at remove: " << currentTableIndex << std::endl;
                for (const ComponentID& componentID : _tables[currentTableIndex]._type) {
                    //std::cout << "Component: " << componentID << " ";
                };
                //std::cout << std::endl;
                size_t nextTableIndex = findTablePredecessor(currentTableIndex, {getComponentIDUnsafe<T>()});
                //std::cout << "Next table index and type at remove: " << nextTableIndex << std::endl;
                for (const ComponentID& componentID : _tables[nextTableIndex]._type) {
                    //std::cout << "Component: " << componentID << " ";
                };
                //std::cout << std::endl;
                moveToTable(entityID, currentTableIndex, nextTableIndex);
            };

            // Component getters, these also have similarly multiple 
            // version for performance and safety issues as with entity
            // creation. All componet get return values are constant 
            // references. Updating component values should only be 
            // done through setters or systems. 
            
            // TODO: how to make getters safe??

            // Unsafe component getter. This will never check if 
            // the entity has a component and thus will never throw 
            // an exception. Higher performance, but only use 
            // if it is provable the entity does have the component 
            // when called. 
            template <class T>
            const T& getComponent(EntityID entityID) noexcept {
                ComponentID componentID = getComponentIDUnsafe<T>();
                //std::cout << "Getting component data" << std::endl;
                size_t tableIndex = _tableRegistry.at(entityID)._tableIndex;
                size_t entityColumnIndex = _tableRegistry.at(entityID)._entityRowIndex;
                const Column& column = _tables[tableIndex]._columns.at(componentID);
                return *static_cast<T*>(static_cast<void*>(column._data + column._elementSize * entityColumnIndex));
            };

            // This getter also does not throw an exception, however 
            // it will first check whether the entity has the requested component,
            // and if not return an empty optional. 
            template <class T>
            std::optional<T> getComponentSafe(EntityID entityID) noexcept {
                if (!hasComponent<T>(entityID)) {
                    std::cout << "Did not have component " << std::endl;
                    return std::nullopt;
                } else {
                    std::cout << "Had component " << std::endl;
                    return std::optional<T>(getComponent<T>(entityID));
                };
            };

            // Pre: component must have been created before 
            static size_t getComponentSize(ComponentID componentID) {
                return _componentSizes.at(componentID);
            };

            template <typename T>
            static size_t getComponentSize() {
                return _componentInfoMap.at(std::type_index(typeid(T)));
            };
            
            template <class T>
            bool hasComponent(EntityID entityID) noexcept {
                //size_t tableIndex = _tableRegistry.at(entityID)_.tableIndex;
                size_t tableIndex = _tableRegistry.at(entityID)._tableIndex;
                // want to add to component registry if it doesn't exist so can use [] here
                return _componentRegistry[getComponentIDOrInsert<T>()].count(tableIndex) != 0;
            };

            // Deferred versions of all operations. 
            // The effects of these operations will only be visible
            // on the database once flushDeferred or flushDeferredAll 
            // is called. 

            void flushDeferred(EntityID entityID) {
                flushDeferredNoDelete(entityID);
                _deferredPayloads.erase(entityID);
            };

            void flushDeferredAll() {
                for (std::unordered_map<EntityID, DeferredPayload>::const_iterator it = _deferredPayloads.begin(); it != _deferredPayloads.end(); it++) {
                    // TODO: maybe should pass both here so I don't do another lookup in the function?
                    flushDeferredNoDelete(it->first);
                };
                _deferredPayloads.clear();
            };

            template <typename T>
            void addComponentDeferred(EntityID entityID) noexcept {
                _deferredPayloads[entityID]._componentPayloadMap.insert_or_assign(getComponentIDOrInsert<T>(), ComponentMode::Add);
                //std::cout << "Inserted add component with ID: " << getComponentID<T>() << std::endl;
            };

            template <class T> 
            void removeComponentDeferred(EntityID entityID) noexcept {
                _deferredPayloads[entityID]._componentPayloadMap.insert_or_assign(getComponentIDOrInsert<T>(), ComponentMode::Remove);
                //std::cout << "Inserted remove component with ID: " << getComponentID<T>() << std::endl;
            };

            template <class T> 
            void setComponentCopyDeferred(EntityID entityID, T* componentData) noexcept {
                // TODO: fix this 
                SetComponentPayload setPayload{};
                setPayload.data = static_cast<void *>(componentData);
                setPayload.mode = SetMode::Copy;
                _deferredPayloads[entityID]._setMap.insert_or_assign(getComponentIDUnsafe<T>(), setPayload);
            };

            template <class T>
            void setComponentMoveDeferred(EntityID entityID, T* componentData) noexcept {
                _deferredPayloads[entityID]._setMap.insert_or_assign(getComponentIDUnsafe<T>(), static_cast<void *>(componentData), SetMode::Move);
            };

            // Deletes all erased entities, resizes 
            // columns to only fit the minimal data required.
            void cleanupMemory() {
                // Go over every table and every column of their table
                // resize columns to size of maxOccupiedRow
                // remove every free index bigger than or equal to max occupied 
                // row from their delete sets 

                // do this for every table except empty begin table..? 
                // TODO: need to make a map only with tables where capacity >= 
                // maxRow, and only those are viable for cle anup
                for (std::vector<Table>::iterator it = _tables.begin(); it != _tables.end(); ++it) {
                    // TODO: maybe should also fix all fragmentation issues 
                    // here. 
                    // TODO: for fixing fragmentation, need to do
                    // memcpy without overlapping memory ranges. 
                    //std::cout << "Old capacity: " << it->_capacity << std::endl;
                    //std::cout << "New capacity: " << it->_maxOccupiedRow << std::e ndl;
                    for (Columns::iterator columnIt = it->_columns.begin(); columnIt != it->_columns.end(); ++columnIt) {
                        columnIt->second._data = static_cast<char*>(realloc(columnIt->second._data, (it->_maxOccupiedRow + 1) * columnIt->second._elementSize));
                        
                        if (columnIt->second._data == nullptr) {
                            throw std::runtime_error("Could not reallocatee column data");
                        };
                    };
                };
            };

            QueryBuilder makeQueryBuilder() {
                // TODO: use move constructor here 
                return QueryBuilder(QueryBuildMode::Transient, this);
            };

            // Gets the ID for a component or inserts it and its size
            // in the static component info map if it doesn't exist
            template <typename T>
            static ComponentID getComponentIDOrInsert() {
                ComponentID componentID;
                std::type_index typeIndex = std::type_index(typeid(T));
                if (_componentInfoMap.count(typeIndex) == 0) {
                   //std::cout << "Couldn't find component ID" << std::endl;
                    componentID = generateNewComponentID();
                    _componentInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(typeIndex), std::forward_as_tuple(componentID, sizeof(T)));
                    _componentSizes.emplace(std::piecewise_construct, std::forward_as_tuple(componentID), std::forward_as_tuple(sizeof(T)));
                } else {
                    componentID = _componentInfoMap.at(typeIndex).componentID;
                };
                return componentID;
            };  
            // Gets the ID of a component unsafely. Only use if
            // you know the component already exists. (e.g: in functions that 
            // come after having called addComponent<T>)
            template <typename T>
            static ComponentID getComponentIDUnsafe() {
                return _componentInfoMap.at(std::type_index(typeid(T))).componentID;
            };

        private:
            // Generates a new entity ID. First, this function will
            // check if there is a reusable entity ID in the reusable list.
            // Otherwise, it will increment the current entity ID value to 
            // get a fresh ID. This function will throw an exception if 
            // the running entity ID accumulator has reched its 
            // maximum value (2^32). 
            EntityID generateNewEntityID() {
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

            // Similarly to the above, will throw an error if the component ID
            // becomes greater than 2^31.
            static ComponentID generateNewComponentID() {
                if (COMPONENT_ID == MAX_COMPONENT_ID) {
                        throw std::runtime_error("Reached the maximum number of components, cannot create any new ones.");
                }
                //std::cout << "Generated id: " << COMPONENT_ID << std::endl;
                return COMPONENT_ID++;
            };

            EntityID generateNewIDAndAddToEmptyTableRegistry() {
                EntityID newID = generateNewEntityID();
                // The 0-th table will always be the default empty
                // table. 
                // TODO: this doesnt work with the memory management system 
                addEntityToTableRegistry(newID, 0, _tables.front()._maxOccupiedRow); // first table, the "empty" table
                _tables.front()._maxOccupiedRow++;
                return newID;
            };

            void addEntityToTableRegistry(EntityID entityID, size_t tableIndex, size_t position) noexcept {
                // TODO: figure out how to make the TableIndex assignable 
                _tableRegistry.erase(entityID);
                _tableRegistry.emplace(std::piecewise_construct, std::forward_as_tuple(entityID & ENTITY_ID_MASK), std::forward_as_tuple(_tables[tableIndex], tableIndex, position));
                //_tableRegistry.insert_or_assign((entityID & ENTITY_ID_MASK), newIndex);
            };

            void moveToTable(EntityID entityID, size_t sourceTableIndex, size_t destTableIndex) {
                size_t currentEntityRowIndex = _tableRegistry.at(entityID)._entityRowIndex;
                Table& sourceTable = _tables[sourceTableIndex];
                Table& destTable = _tables[destTableIndex];
                
                // Add a block of size 1 to the list 
                //std::cout << "About to add to free block list " << std::endl;
                sourceTable.addToFreeBlockList(Table::ColumnBlock(currentEntityRowIndex, currentEntityRowIndex, 1));
                //std::cout << "Finished adding to free block list " << std::endl;

                if (currentEntityRowIndex == sourceTable._maxOccupiedRow -  1) {
                    //std::cout << "Deleting the largest unused block at the endl;. " << std::endl;
                    // TODO: should I use rbegin h ere?
                    Table::FreeBlockList::iterator endBlock = --(sourceTable._freeBlockList.end());
                    //std::cout << "Deleting from " << endBlock->startRow << " to " << endBlock->endRow << std::endl;
                    sourceTable._maxOccupiedRow = endBlock->_startRow;
                    sourceTable._blockSizeMap.at(endBlock->_size).erase(*endBlock);
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
                    Table::ColumnBlock newBlock(blockIt->_startRow + 1, blockIt->_endRow, blockIt->_size - 1);
                    freeRowIndex = blockIt->_startRow;
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
                    destTable._capacity = destTable._capacity  * 2;
                    for (Columns::iterator it = destTable._columns.begin(); it != destTable._columns.end(); it++) {
                        it->second._data = static_cast<char *>(realloc(it->second._data, destTable._capacity * it->second._elementSize));
                        
                        if (it->second._data == nullptr) {
                            throw std::runtime_error("Ran out of memory, cannot increase row count for table.");
                        };
                    };
                };

                // Now perform data copies for overlapping components, 
                // with guaranteed space, first find overlapping components. 
                TableType overlappingColumns;
                // TODO: does this work if the set doesn't have capacity reserved fo r the overlapping elemen ts? 
                std::set_intersection(sourceTable._type.begin(), sourceTable._type.end(), destTable._type.begin(), destTable._type.end(), std::inserter(overlappingColumns, overlappingColumns.begin()));
                // TODO: can I SIMD/vectorize this in any way?
                
                // Copy column content
                for (const ComponentID& componentID : overlappingColumns) {
                    //std::cout << "Overlapping component ID: " << componentID << std::endl;
                    size_t copySize = sourceTable._columns[componentID]._elementSize;
                    // Always deal with byte increments 
                    void* copyFrom = static_cast<char*>(sourceTable._columns[componentID]._data) + copySize * currentEntityRowIndex;
                    void* copyTo = static_cast<char*>(destTable._columns[componentID]._data) + copySize * freeRowIndex;
                    std::memcpy(copyTo, copyFrom, copySize);
                };  

                //std::cout << "Finished moving entity to new table " << std::endl;
            };

            size_t createTableGraph(size_t startingTableIndex, const TableType& addType) {
                size_t currentTableIndex = startingTableIndex;
                TableType::const_iterator it;
                Table::TableEdges::const_iterator addGraphIt;
                Table::TableEdges::const_iterator addGraphEnd;

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
                // in order he re. 
                while (it != addType.end()) {
                    // Type of this is the type of the entry table
                    // + every type until the it component  
                   //std::cout << "Making new table type" << std::endl;
                    TableType newType(_tables[currentTableIndex]._type);
                    newType.insert(addType.begin(), std::next(it, 1));
                   //std::cout << "Added missing types to new type" << std::endl;
                    // TODO: maybe shouldnt do default common size here, since 
                    // the intermediary table might not immediately need
                    // as many components 
                    for (const ComponentID& componentID : newType) {
                       //std::cout << "Component " << componentID << " ";
                    };
                   //std::cout << std::endl;
                    _tables.emplace_back(newType, _defaultColumnSize);
                   //std::cout << "Added new table" << std::endl;
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

            size_t findTableSuccessor(const size_t tableIndex, const TableType& addType) {
                size_t startingTableIndex = tableIndex;
                TableType startingAddType = addType;
                // If the first add component ID is greater than the 
                // max component id of the table we need to perform the 
                // graph search from the beginning
                if (addType.size() > 0 && _tables[tableIndex]._maxComponentID > *addType.begin()) {
                    //std::cout << "Performing graph search from the start: " << std::endl;
                    startingTableIndex  = 0;
                    startingAddType.insert(_tables[tableIndex]._type.begin(), _tables[tableIndex]._type.end());
                };

                // Now we can perform the graph search 
                return createTableGraph(startingTableIndex, addType);
            };

            size_t findTablePredecessor(const size_t tableIndex, const TableType& removeTypes) {
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

            void flushDeferredNoDelete(EntityID entityID) {
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

                size_t currentTableIndex = _tableRegistry.at(entityID)._tableIndex;
                //std::cout << "Current table index is: " << currentTableIndex << std::endl;
                
                size_t nextTableIndex = findTableSuccessor(currentTableIndex, addType);
                //std::cout << "Next table index after adding types is: " << nextTableIndex << std::endl;

                nextTableIndex = findTablePredecessor(nextTableIndex, removeType);
                //std::cout << "Next table index after removing types is: " << nextTableIndex << std::endl;
                
                moveToTable(entityID, currentTableIndex, nextTableIndex);

                // Now, apply all the set deferred operati ons 
                for (SetComponentMap::const_iterator it = entityPayload._setMap.begin(); it != entityPayload._setMap.end(); ++it) {
                    if (it->second.mode == SetMode::Copy) {
                        size_t tableIndex = _tableRegistry.at(entityID)._tableIndex;
                        size_t entityColumnIndex = _tableRegistry.at(entityID)._entityRowIndex;
                        const Column& column = _tables[tableIndex]._columns[it->first];
                        // This version uses the copy constructor. 
                        //std::cout << "Performing memcopy for component: " << it->first << std::endl;
                        memcpy(column._data + column._elementSize * entityColumnIndex, it->second.data, column._elementSize);
                    } else {
                        // TODO;
                    };
                };
            };

            Query buildQuery(const QueryBuilder& builder) {
                // TODO: add caching 
                // First, get set of all tables

                std::unordered_set<ComponentID>::iterator it = builder._requiredComponents.begin();
                std::unordered_set<size_t> currentTableIDs;
                std::unordered_set<size_t> prevTableIDs = _componentRegistry[*it];

                for (std::unordered_set<ComponentID>::iterator requiredComp = std::next(it, 1); requiredComp != builder._requiredComponents.end(); ++it) {
                    for (const size_t& tableID : prevTableIDs) {
                        if (_componentRegistry[*requiredComp].find(tableID) != _componentRegistry[*requiredComp].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                };

               //std::cout << "Filtered required components " << std::endl;

                // Now filter all required explict relationships
                for (const ComponentID& componentID : builder._requiredRelationshipsTargets) {
                    for (const size_t& tableID : prevTableIDs) {
                        if (_componentRegistry[componentID].find(tableID) != _componentRegistry[componentID].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                };

               //std::cout << "Filtered required relationships " << std::endl;

                // Only keep tables where the entity is a relationship holder.
                for (const ComponentID& componentID : builder._requiredRelationshipsHolder) {
                    for (const size_t& tableID : prevTableIDs) {
                        if (_relationshipCountRegistry[componentID].find(tableID) != _relationshipCountRegistry[componentID].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                };

               //std::cout << "Filtered required relationship holdings " << std::endl;

                // Only keep tables where the entity is a target..???
                /*for (const ComponentID& componentID : builder._requiredRelationshipsHolder) {
                    for (const size_t& tableID : prevTableID s) {
                        if (_relationshipCountRegistry[componentID].find(tableID) != _relationshipCountRegistry[componentID].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                }; */


                // filter all tables that have banned components 
                for (const ComponentID& componentID : builder._bannedComponents) {
                    for (const size_t& tableID : prevTableIDs) {
                        if (_componentRegistry[componentID].find(tableID) == _componentRegistry[componentID].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                };

               //std::cout << "Filtered banned components " << std::endl;

                // filter all tables that have banned relationships 
                for (const ComponentID& componentID : builder._bannedRelationshipsTargets) {
                    for (const size_t& tableID : prevTableIDs) {
                        if (_componentRegistry[componentID].find(tableID) == _componentRegistry[componentID].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                };

               //std::cout << "Filtered banned relationships " << std::endl;

                // Filter all tables with banned relationship holders 
                for (const ComponentID& componentID : builder._bannedRelationshipsHolder) {
                    for (const size_t& tableID : prevTableIDs) {
                        if (_relationshipCountRegistry[componentID].find(tableID) == _relationshipCountRegistry[componentID].end()) {
                            currentTableIDs.emplace(tableID);
                        };
                    };
                    prevTableIDs.swap(currentTableIDs);
                    currentTableIDs.clear();
                };

               //std::cout << "Filtered banned relationship holdings " << std::endl;

                // Now, need to get the column data pointers from all 
                // tables according tot he get component sets 

                std::vector<Query::QueryTable> queryTables;
                
                for (const size_t& tableIndex : prevTableIDs) {
                    Table& table = _tables[tableIndex];
                    std::unordered_map<ComponentID, Database::Column*> columnData;
                    
                    for (const ComponentID& componentID : builder._getComponents) {
                        columnData[componentID] = &table._columns.at(componentID);
                    };

                    for (const ComponentID& componentID : builder._getRelationships) {
                        columnData[componentID] = &table._columns.at(componentID);
                    };
                    // TODO: add entity IDs here and for fragmentation 
                    // TODO: memory should be flushed before doing this, no 
                    // fragmentation 
                    queryTables.emplace_back(columnData, std::vector<EntityID>(), table._maxOccupiedRow, tableIndex);
                };

                return Query(queryTables, 0, this);
            };
    };

    // Static ID init 
    ComponentID Database::COMPONENT_ID = 1;
    Database::ComponentInfoMap Database::_componentInfoMap = Database::ComponentInfoMap();
    Database::ComponentSizes Database::_componentSizes = Database::ComponentSizes();

};

#endif