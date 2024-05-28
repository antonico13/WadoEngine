#include "MainClonePtr.h"

#include <stdexcept>

namespace Wado::Memory {
    template <class T>
    WdClonePtr<T>::WdClonePtr() : _ptr(nullptr), _validity(nullptr) {};

    template <class T>
    WdClonePtr<T>::operator bool() const noexcept {
        return _ptr != nullptr && _validity != nullptr;
    };

    template <class T>
    T& WdClonePtr<T>::operator*() {
        if (!this) {
            throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
        }
        if (!_validity->alive) {
            throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
        }
        return *_ptr;
    };

    template <class T>
    T* WdClonePtr<T>::operator->() {
        if (!this) {
            throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
        }
        if (!_validity->alive) {
            throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
        }
        return _ptr;
    };

    template <class T>
    const T& WdClonePtr<T>::operator*() const {
        if (!this) {
            throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
        }
        if (!_validity->alive) {
            throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
        }
        return *_ptr;
    };

    template <class T>
    const T* WdClonePtr<T>::operator->() const {
        if (!this) {
            throw std::runtime_error("Attempted to access clone pointer resource that is uninitialized.");
        }
        if (!_validity->alive) {
            throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
        }
        return _ptr; 
    };


    template <class T>
    WdClonePtr<T>& WdClonePtr<T>::operator=(const WdClonePtr<T>& other) {
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

    template <class T>
    WdClonePtr<T>& WdClonePtr<T>::operator=(WdClonePtr<T>&& other) { 
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

    template <class T>
    WdClonePtr<T>::WdClonePtr(const WdClonePtr& other) {
        *this = other;
    };

    template <class T> 
    WdClonePtr<T>::WdClonePtr(WdClonePtr&&  other) {
        *this = other;
    };

    template <class T>
    WdClonePtr<T>::WdClonePtr(T* const ptr, PtrValidity* validity) : _validity(validity), _ptr(ptr) { };
    
    template <class T>
    WdClonePtr<T>::~WdClonePtr() { 
        // TOOD: add an assert here 
        _validity->clones--;
        if (!_validity->alive && _validity->clones == 0) {
            // can safely free the validity struct here
            delete _validity;
        }
    }; 

    template <class T>
    WdMainPtr<T>::WdMainPtr(T* const ptr) : _ptr(ptr) { 
        _validity = new PtrValidity;
        // TODO: check if it failed here 
    };

    template <class T>
    WdMainPtr<T>::~WdMainPtr() {
        _validity->alive = false;
        if (_validity->clones == 0) {
            // Has to be safe 
            delete _validity;
        }
        delete _ptr;
    };

    template <class T>
    WdClonePtr<T> WdMainPtr<T>::getClonePtr() {
        WdClonePtr<T> newPtr(_ptr, _validity);
        _validity->clones++;
        return newPtr;
    };
}