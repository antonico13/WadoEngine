#ifndef WADO_ECS_H
#define WADO_ECS_H

#include "MainClonePtr.h"

#include <vector>
#include <map>

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
            const T& getComponent() noexcept;

            template <class T>
            std::optional<const T&> getComponent() noexcept;
            
            template <class T>
            bool hasComponent() noexcept;
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
            // IDs, and 2^32 entities created. 

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
            const T& getComponent(EntityID entityID) noexcept;

            // This getter also does not throw an exception, however 
            // it will first check whether the entity has the requested component,
            // and if not return an empty optional. 
            template <class T>
            std::optional<const T&> getComponent(EntityID entityID) noexcept;
            
            template <class T>
            bool hasComponent(EntityID entityID) noexcept;

        private:

            uint64_t COMPONENT_ID = 0;
            uint64_t ENTITY_ID = 1 << 32;
            static const uint64_t ENTITY_INCREMENT = 1 << 32;
            static const uint64_t COMPONENT_INCREMENT = 1;

            // Gets the ID for a component. Each specialization of 
            // this function will have a static variable with the component ID,
            // if calling for the first time a new ID is generated, otherwise the 
            // component ID for this function is returned. 
            template <class T>
            inline ComponentID getComponentID() noexcept;

            std::vector<EntityID> reusableEntityIDs;

            using TableType = const std::vector<uint64_t>; // The type of a table, this should hopefully allow for 
            // fast "has component" checks rather than looking up a map. Basically, do component ID mod 64. Each bit
            // in one of the uints is meant to represent whether a component is present or not. Since tables are technically 
            // immutable, once this vector is created it should never be modified. 

            struct Column {
                void* data; // Raw data pointer, let these be managed by the ECS instead of using 
                // memory/clone pointers
                size_t elementStride; // element stride in bytes  
                size_t elementCount;
            };

            struct Table {
                TableType type;
                std::vector<Column> columns;
            };  
    };
};

#endif