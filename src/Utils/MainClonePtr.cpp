#include "MainClonePtr.h"

#include <stdexcept>

namespace Wado::Memory {
    template <class T>
    T& WdClonePtr<T>::operator*() {
        // TODO: assert validity not null here 
        if (!_validity->alive) {
            throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
        }
        return *_ptr;
    };

    template <class T>
    T* WdClonePtr<T>::operator->() {
        // TODO: assert validity not null here 
        if (!_validity->alive) {
            throw std::runtime_error("Attempted to access clone pointer resource after main pointer was deleted.");
        }
        return _ptr;
    };

    template <class T>
    WdClonePtr<T>::WdClonePtr(T* const ptr, PtrValidity* validity) : _validity(validity), _ptr(ptr) { };
    
    template <class T>
    WdClonePtr<T>::WdClonePtr(WdClonePtr<T>& other) : _validity(other._validity), _ptr(other._ptr) { };

    template <class T>
    WdClonePtr<T>::WdClonePtr(WdClonePtr<T>&& other) : _ptr(other._ptr), _validity(other._validity) { };

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
        delete _ptr;
    };

    template <class T>
    WdClonePtr<T> WdMainPtr<T>::getClonePtr() {
        WdClonePtr<T> newPtr(_ptr, _validity);
        _validity->clones++;
        return newPtr;
    };
}