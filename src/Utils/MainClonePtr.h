#ifndef WADO_MAIN_CLONE_PTR_H
#define WADO_MAIN_CLONE_PTR_H

#include <stdint.h>
#include <stdexcept>

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

            T& operator* () {
                if (!this) {
                    throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
                }
                if (!_validity->alive) {
                    throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
                }
                return *_ptr;
            };

            T* operator-> () {
                if (!this) {
                    throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
                }
                if (!_validity->alive) {
                    throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
                }
                return _ptr;
            };

            const T& operator* () const {
                if (!this) {
                    throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
                }
                if (!_validity->alive) {
                    throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
                }
                return *_ptr;
            };

            const T* operator->() const {
                if (!this) {
                    throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
                }
                if (!_validity->alive) {
                    throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
                }
                return _ptr;
            };

            ~WdClonePtr() { 
                // TOOD: add an assert here 
                _validity->clones--;
                if (!_validity->alive && _validity->clones == 0) {
                    // can safely free the validity struct here
                    delete _validity;
                }
            }; 

            WdClonePtr() : _ptr(nullptr), _validity(nullptr) {}; // Empty clone pointer can be initialized without a main pointer, always invalid but useful for default values

            explicit operator bool() const noexcept {
                return _ptr != nullptr && _validity != nullptr;
            }; // Returns whether pointer is valid or not 

            // copy assignment operator
            WdClonePtr& operator=(const WdClonePtr& other) {
                if (!other || !other._validity->alive) {
                    throw std::runtime_error("Attempted to copy clone pointer that is invalid.");
                }
                if (this == &other) {
                    return *this;
                }
                other._validity->clones++;
                this->_ptr = other._ptr;
                this->_validity = other._validity;
                return *this;
            };

            // move assignemnt operator
            WdClonePtr& operator=(WdClonePtr&& other) { 
                if (!other || !other._validity->alive) {
                    throw std::runtime_error("Attempted to copy clone pointer that is invalid.");
                }
                if (this == &other) {
                    return *this;
                }
                other._validity->clones++;
                this->_ptr = other._ptr;
                this->_validity = other._validity;
                return *this;
            };

            // copy constructor
            WdClonePtr(const WdClonePtr& other) {
                *this = other;
            };

            WdClonePtr(WdClonePtr& other) {
                *this = other;
            };

            // move constructor 
            WdClonePtr(WdClonePtr&& other) {
                *this = other;
            };
            // move assignment operator 
            //WdClonePtr(WdClonePtr&& other); // for move constructors 
            //WdClonePtr operator=(WdClonePtr other); 
            //WdClonePtr operator=(WdClonePtr& other); 
            //WdClonePtr operator=(const WdClonePtr& other); 
            //WdClonePtr(WdClonePtr& other);
        private:
            WdClonePtr(T* const ptr, PtrValidity* validity) : _validity(validity), _ptr(ptr) { };
            //WdClonePtr& operator=(WdClonePtr& other); // or assign without move  
            //WdClonePtr  operator=(WdClonePtr other); 

            PtrValidity* _validity;
            T* _ptr;
    };

    template <class T>
    class WdMainPtr {
        public:
            WdMainPtr(T* const ptr) : _ptr(ptr) { 
                _validity = static_cast<PtrValidity *>(malloc(sizeof(PtrValidity)));
                _validity->alive = true;
                _validity->clones = 0;
                // TODO: check if it failed here 
            };

            ~WdMainPtr() {
                _validity->alive = false;
                if (_validity->clones == 0) {
                    // Has to be safe 
                    delete _validity;
                }
                delete _ptr;
            };

            WdClonePtr<T> getClonePtr() {
                WdClonePtr<T> newPtr(_ptr, _validity);
                _validity->clones++;
                return newPtr;
            };

        private:
            T* const _ptr;
            PtrValidity* _validity;
    };
}

#endif