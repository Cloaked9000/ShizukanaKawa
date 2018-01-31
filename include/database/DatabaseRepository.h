//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_DATABASEREPOSITORY_H
#define SFTPMEDIASTREAMER_DATABASEREPOSITORY_H


#include <memory>
#include <Types.h>
#include <map>
#include <mutex>

template<typename T>
class DatabaseRepository
{
public:
    virtual ~DatabaseRepository()=default;

    /*!
     * Flushes the cache to the database
     */
    void flush()
    {
        std::lock_guard<std::mutex> guard(lock);
        for(auto &a : cache)
        {
            database_update(a.second);
        }
    }

    /*!
     * Creates a new entry in the database. Goes through
     * the caching layer first.
     *
     * @param entry The entry to create. Its id parameter will be filled in.
     * @return The ID of the newly created object
     */
    uint64_t create(T *entry)
    {
        try
        {
            entry->id = database_create(entry);
            store_cache(std::make_shared<T>(*entry));
        }
        catch(std::exception &e)
        {
            throw std::runtime_error("Failed to create a new '" + DEMANGLE(T) + "' entry within the database. Exception: " + e.what());
        }
        return entry->id;
    }

    /*!
     * Loads an entry with a given ID from the database, Goes through
     * the caching layer first.
     *
     * @param id The ID of the entry to load.
     * @return The ID of the newly created object
     */
    std::shared_ptr<T> load(uint64_t id)
    {
        //The database might throw an exception if it can't find it. Catch it.
        std::shared_ptr<T> loaded;
        try
        {
            std::lock_guard<std::mutex> guard(lock);

            //Check the cache first to see if it's already loaded
            auto iter = cache.find(id);
            if(iter != cache.end())
                return iter->second;

            //It's not cached, load it from the database implementation
            loaded = database_load(id);
        }
        catch(std::exception &e)
        {
            throw std::runtime_error("Failed to load '" + DEMANGLE(T) + "' with ID " + std::to_string(id) + " from the database. Exception: " + e.what());
        }

        //Cache the loaded entry then exit
        store_cache(loaded);
        return loaded;
    }

    /*!
     * Erases an object from both the database and cache
     *
     * @param id The ID of the entry to erase
     */
    void erase(uint64_t id)
    {
        std::lock_guard<std::mutex> guard(lock);

        try
        {
            //Delete from cache if it's cached
            auto iter = cache.find(id);
            if(iter != cache.end())
                cache.erase(iter);

            //Delete from database
            database_erase(id);
        }
        catch(std::exception &e)
        {
            throw std::runtime_error("Failed to delete '" + DEMANGLE(T) + "' with ID " + std::to_string(id) + " from the database. Exception: " + e.what());
        }
    }

    /*!
     * Checks if an entry with a given ID exists.
     *
     * Works internally by calling load(id) and then catching
     * any exceptions. If an exception is thrown, then the entry
     * couldn't be loaded, and so doesn't exist. If no exception was
     * thrown then the entry must exist.
     *
     * Because 'load' caches the entry,
     * this makes checking for existence of an entry before loading it efficient,
     * as only one database query is actually made.
     *
     * @param id The ID of the entry to check the existance of
     * @return True if it exists, false otherwise
     */
    bool exists(uint64_t id)
    {
        try
        {
            load(id);
        }
        catch(...)
        {
            return false;
        }
        return true;
    }

protected:

    // Cache layer functions

    /*!
     * Store an object in the cache
     *
     * @param obj The object to store
     */
    void store_cache(std::shared_ptr<T> obj)
    {
        if(!obj)
            return;

        std::lock_guard<std::mutex> guard(lock);

        //Clean cache
        clean_cache();

        //Store the object in the cache
        auto ret = cache.emplace(obj->id, std::move(obj));
        if(!ret.second)
        {
            throw std::logic_error("DatabaseRepository::store_cache(): Can't cache entities with duplicate IDs");
        }
    }

    /*!
     * Retrieves an object from the cache
     *
     * @throws An std::range_error on failure, if the ID doesn't exist.
     * @param id The ID of the object to fetch
     * @return The object
     */
    T retrieve_cache(uint64_t id)
    {
        std::lock_guard<std::mutex> guard(lock);
        return cache.at(id);
    }

    /*!
     * Tries to retrieve a cache item
     *
     * @param id The ID of the object to fetch
     * @param entry A pointer to a valid object of type T, where the object will be copied to
     * @return True if the object could be fetched, false otherwise.
     */
    bool try_retrieve_cache(uint64_t id, T *entry)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto iter = cache.find(id);
        entry = iter;
        return iter != cache.end();
    }

    // To be implemented by the repository implementation

    /*!
     * Creates a new entry and saves it to the database
     *
     * @throws An std::logic_error on failure
     * @returns The ID of the newly created object
     */
    virtual uint64_t database_create(T *entry) =0;

    /*!
     * Loads an existing entry from the database
     *
     * @throws An std::logic_error on failure.
     * @param entry_id The ID of the entry to load
     */
    virtual std::shared_ptr<T> database_load(uint64_t entry_id) =0;

    /*!
     * Updates the entry if it's already
     * an existing entry in the database
     *
     * @throws An std::logic_error on failure
     */
    virtual void database_update(std::shared_ptr<T> entry) =0;

    /*!
     * Removes an entry from the database
     *
     * @param entry_id The ID of the entry
     */
    virtual void database_erase(uint64_t entry_id) =0;


private:

    /*!
     * Removes all entries which are no longer needed
     * from the cache
     */
    void clean_cache()
    {
        //Free things which aren't in use
        for(auto iter = cache.begin(); iter != cache.end(); )
        {
            if(iter->second.use_count() == 1) //If we're the only ones using it
            {
                try
                {
                    //Commit to database then erase
                    database_update(iter->second);
                }
                catch(const std::exception &e)
                {
                    throw std::runtime_error("Failed to commit '" + DEMANGLE(T) + "' with ID " + std::to_string(iter->second->id) + " to the database. Exception: " + e.what());
                }

                iter = cache.erase(iter);
                continue;
            }
            ++iter;
        }
    }

    //State
    std::map<uint64_t, std::shared_ptr<T>> cache;
    std::mutex lock;
};

#endif //SFTPMEDIASTREAMER_DATABASEREPOSITORY_H
