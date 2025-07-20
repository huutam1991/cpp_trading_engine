#pragma once

#include <cstddef>
#include <list>
#include <string>
#include <cxxabi.h>

#include <utils/spin_lock.h>

template <typename T>
std::string demangled_name() {
    int status;
    char* realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string result = (status == 0 && realname) ? realname : typeid(T).name();
    std::free(realname);
    return result;
}

template <typename T, typename = void>
struct TypeName {
    static std::string name() {
        return demangled_name<T>();
    }
};

template <typename T>
struct TypeName<T, std::void_t<decltype(T::name())>> {
    static std::string name() {
        return T::name();
    }
};

template <class T, size_t Size>
class CachePool
{
    static std::list<T*>& get_available_items_list()
    {
        static std::list<T*> available_items;
        static T pool[Size];
        static bool initialized = [] 
        {
            for (size_t i = 0; i < Size; ++i)
            {
                available_items.push_back(&pool[i]);
            }
            return true;
        }();

        return available_items;
    }

    static SpinLock& get_spin_lock()
    {
        static SpinLock spin_lock;
        return spin_lock;
    }   

public:
    // Acquire a cache item
    static T* acquire()
    {
        std::lock_guard<SpinLock> guard(get_spin_lock());

        std::list<T*>& available_items = get_available_items_list();

        if (available_items.empty())
        {
            throw std::runtime_error("No available items in cache pool: [" + TypeName<T>::name() + "]");
        }

        T* item = available_items.front();
        available_items.pop_front();
        return item;
    }

    // Release a cache item back to the pool
    static void release(T* item)
    {
        std::lock_guard<SpinLock> guard(get_spin_lock());

        std::list<T*>& available_items = get_available_items_list();    
        if (item != nullptr)
        {
            available_items.push_back(item);
        }
        else
        {
            throw std::runtime_error("Attempt to release a null item back to the cache pool: [" + TypeName<T>::name() + "]");
        }
    }
};