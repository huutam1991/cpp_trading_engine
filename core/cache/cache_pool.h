#pragma once

#include <cstddef>
#include <list>

#include <utils/spin_lock.h>

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

    // Acquire a cache item
    static T* acquire()
    {
        std::lock_guard<SpinLock> guard(get_spin_lock());

        std::list<T*>& available_items = get_available_items_list();

        if (available_items.empty())
        {
            throw std::runtime_error("No available items in cache pool: [" + T::name() + "]");
        }

        T* item = available_items.front();
        available_items.pop_front();
        return item;
    }

    // Release a cache item back to the pool
    void release(T* item)
    {
        std::lock_guard<SpinLock> guard(get_spin_lock());

        std::list<T*>& available_items = get_available_items_list();    
        if (item != nullptr)
        {
            available_items.push_back(item);
        }
        else
        {
            throw std::runtime_error("Attempt to release a null item back to the cache pool: [" + T::name() + "]");
        }
    }
};