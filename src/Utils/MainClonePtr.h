#ifndef WADO_MAIN_CLONE_PTR_H
#define WADO_MAIN_CLONE_PTR_H

#include <stdint.h>

namespace Wado::Memory {
    
    using PtrValidity = struct PtrValidity {
        bool alive = true;
        uint32_t clones = 0;
    };

    template <class T>
    class WdMainPtr;

    // TODO: add default constructor for unsafe ptr 
    // TODO: these are non-thread safe at the moment    
    template <class T>
    class WdClonePtr final { // Final, don't want people to inherit this class and access the ptr after it's been invalidated 
        public:
            friend class WdMainPtr<T>;

            T& operator* ();
            T* operator-> ();
            ~WdClonePtr();

            WdClonePtr(); // Empty clone pointer can be initialized without a main pointer, always invalid but useful for default values

            explicit operator bool() const noexcept; // Returns whether pointer is valid or not 

            // copy assignment operator
            WdClonePtr& operator=(const WdClonePtr& other);
            // move assignemnt operator
            WdClonePtr& operator=(WdClonePtr&& other);

            // copy constructor
            WdClonePtr(const WdClonePtr& other);
            // move constructor 
            WdClonePtr(WdClonePtr&&  other);
            // move assignment operator 
            //WdClonePtr(WdClonePtr&& other); // for move constructors 
            //WdClonePtr operator=(WdClonePtr other); 
            //WdClonePtr operator=(WdClonePtr& other); 
            //WdClonePtr operator=(const WdClonePtr& other); 
            //WdClonePtr(WdClonePtr& other);
        private:
            WdClonePtr(T* const ptr, PtrValidity* _validity);
            //WdClonePtr& operator=(WdClonePtr& other); // or assign without move  
            //WdClonePtr  operator=(WdClonePtr other); 

            PtrValidity* _validity;
            T* const _ptr;
    };

    template <class T>
    class WdMainPtr {
        public:
            WdMainPtr(T* const ptr);
            ~WdMainPtr();

            WdClonePtr<T> getClonePtr();
        private:
            T* const _ptr;
            PtrValidity* _validity;
    };
}

#endif