#ifndef WADO_ECS_H
#define WADO_ECS_H

#include "MainClonePtr.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>

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

            template <class T>
            const T& getComponent() const noexcept;

            template <class T>
            std::optional<const T&> getComponent() const noexcept;
            
            template <class T>
            bool hasComponent() const noexcept;

            EntityID getID() const noexcept;
        private:
            Entity(EntityID entityID, Database* database);
            const EntityID _entityID;
            // store raw pointer to DB for operations like 
            // adding or setting components 
            Database* _database; 
    };

    class Database {
        public:
            Database();
            ~Database();

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
            Memory::WdClonePtr<Entity> createEntityClonePtr();
            // Using entity objects, the lifetime is managed by the caller of
            // this function. This only requires one pointer dereference for
            // database operations. 
            Entity createEntityObj();
            // Highest performance option. Lifetime is still managed manually
            // by the caller, but all operations require a database object 
            // on which to call add, set component, etc. without any 
            // pointer dereferences 
            EntityID createEntityID();

            // adds a component to an entity based on its ID. The component is
            // "empty" by default, since the underlying type is erased 
            // in the container of the database. This component will be 
            // by default a contiguous array of bytes the size of 
            // the component all set to 0. 
            // Warning: only works with valid IDs produced by the database, 
            // undefined behaviour if the ID supplied is not valid. 
            template <class T> 
            void addComponent(EntityID entityID) noexcept;

            // sets an entity's component values. This uses the copy constructor of
            // the class.
            template <class T> 
            void setComponent(EntityID entityID, T componentData) noexcept;

            // sets an entity's component values. However, be careful as this
            // uses the move constructor, and the value passed in becomes invalid 
            // in the caller's scope.
            template <class T>
            void setComponent(EntityID entityID, T& componentData) noexcept;

            // removes a component to an entity based on its ID.
            template <class T> 
            void removeComponent(EntityID entityID) noexcept;

            // Component getters, these also have similarly multiple 
            // version for performance and safety issues as with entity
            // creation. All componet get return values are constant 
            // references. Updating component values should only be 
            // done through setters or systems. 

            // Unsafe component getter. This will never check if 
            // the entity has a component and thus will never throw 
            // an exception. Higher performance, but only use 
            // if it is provable the entity does have the component 
            // when called. 
            template <class T>
            const T& getComponent(EntityID entityID) const noexcept;

            // This getter also does not throw an exception, however 
            // it will first check whether the entity has the requested component,
            // and if not return an empty optional. 
            template <class T>
            std::optional<const T&> getComponent(EntityID entityID) const noexcept;
            
            template <class T>
            bool hasComponent(EntityID entityID) const noexcept;

        private:

            // The lower 32 bits of an entity ID 
            // are used for the actual ID. The higher 32 bits will be used 
            // for various flags and features such as entity relationships,
            // tags, and more.
            uint64_t COMPONENT_ID = 0;
            uint64_t ENTITY_ID = 1 << 31;
            static const uint64_t ENTITY_INCREMENT = 1;
            static const uint64_t COMPONENT_INCREMENT = 1;
            static const uint64_t MAX_ENTITY_ID = 1 << 32;
            static const uint64_t MAX_COMPONENT_ID = 1 << 31;
            static const uint64_t ENTITY_ID_MASK = 0xFFFFFFFF;

            // Gets the ID for a component. Each specialization of 
            // this function will have a static variable with the component ID,
            // if calling for the first time a new ID is generated, otherwise the 
            // component ID for this function is returned. 
            template <class T>
            ComponentID getComponentID();

            std::vector<EntityID> _reusableEntityIDs;

            // Generates a new entity ID. First, this function will
            // check if there is a reusable entity ID in the reusable list.
            // Otherwise, it will increment the current entity ID value to 
            // get a fresh ID. This function will throw an exception if 
            // the running entity ID accumulator has reched its 
            // maximum value (2^32). 
            inline EntityID generateNewEntityID();
            // Similarly to the above, will throw an error if the component ID
            // becomes greater than 2^31.
            inline ComponentID generateNewComponentID();

            inline EntityID generateNewIDAndAddToEmptyTableRegistry();

            using TableType = std::set<ComponentID, std::less<ComponentID>>; // The type of a table.
            // Just a vector of all its componets, in order. 

            struct Column {
                void* data; // Raw data pointer, let these be managed by the ECS instead of using 
                // memory/clone pointers
                size_t elementStride; // element stride in bytes  
                size_t elementCount;
            };

            struct Table {
                Table(TableType type);
                const TableType _type;
                std::vector<Column> _columns;
                size_t _rowCount;
                // Traversing the add/remove component graphs gives a
                // new table that has overlapping components with
                // the current table +/- the key component ID.
                using TableEdges = std::unordered_map<ComponentID, size_t>;

                TableEdges _addComponentGraph;
                TableEdges _removeComponentGraph;
            };  

            struct TableIndex {
                // TODO: this adds another vector
                // dereference basically, need to see if it's slower
                // than storing pointer here directly. 
                // Alternatively, malloc all tables and have a 
                // vector of table pointers for cleanup,
                // and this can be done the "ref" way 
                // with no contention on the table vector.
                size_t tableIndex;
                size_t entityColumnIndex;
            };

            std::vector<Table> _tables; // Keep a record of tables so they can be
            // cleaned up later. 

            // vector of main pointer for entities. The lifetime of 
            // these entities is managed by the database, so all
            // the main pointers need to be kept track of. 
            std::vector<Memory::WdMainPtr<Entity>> _entityMainPtrs;
            
            using TableRegistry = std::unordered_map<EntityID, TableIndex>;
            TableRegistry _tableRegistry;

            // Used for fast inverse look-ups, for example: getting all tables
            // that have a specific component
            using ComponentRegistry = std::unordered_map<ComponentID, std::unordered_set<Table&>>;
            ComponentRegistry _componentRegistry;

            inline void addEntityToTableRegistry(EntityID entityID, size_t tableIndex) noexcept;
            inline size_t findTableSuccessor(const TableType& fullType) noexcept;
            inline size_t getNextTableOrAddEdges(size_t tableIndex, const ComponentID componentID) noexcept;
    };
};

#endif