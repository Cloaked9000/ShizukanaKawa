//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_DATABASEREPOSITORY_H
#define SFTPMEDIASTREAMER_DATABASEREPOSITORY_H


#include <memory>
#include <Types.h>
#include <map>
#include <atomic>
#include <mutex>
#define CACHE_CLEAR_THRESHOLD 200

class SyntaxError : public std::logic_error
{
public:
    explicit SyntaxError(const std::string &e)
            : std::logic_error(e)
    {}
};


template<typename T>
class DatabaseRepository
{
public:
    virtual ~DatabaseRepository()= default;

    /*!
     * Flushes the cache to the database
     */
    void flush()
    {
        std::lock_guard<std::mutex> guard(lock);
        for(auto &a : cache)
        {
            if(a.second)
            {
                if(a.second->get_dirty())
                {
                    a.second->set_dirty(false); //Order is important, don't want to have changed commited, then more changes made, then dirty flag cleared
                    database_update(a.second);
                }
            }
        }
    }

    /*!
     * Creates a new entry in the database. Goes through
     * the caching layer first.
     *
     * @param entry The entry to create. Its id parameter will be filled in.
     * @return The ID of the newly created object
     */
    template<typename ...Args>
    inline uint64_t create(Args &&...args)
    {
        try
        {
            auto entry = std::make_shared<T>(std::forward<Args>(args)...);
            database_create(entry.get());
            entry->set_dirty(false);
            entry = store_cache(entry);
            return entry->get_id();
        }
        catch(std::exception &e)
        {
            throw std::runtime_error("Failed to create a new '" + DEMANGLE(T) + "' entry within the database. Exception: " + e.what());
        }
    }

    /*!
     * Loads an entry with a given ID from the database, Goes through
     * the caching layer first.
     *
     * @throws An std::exception on failure.
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
            {
                if(!iter->second)
                {
                    throw std::runtime_error("Entry was recently deleted");
                }

                return iter->second;
            }

            //It's not cached, load it from the database implementation
            loaded = database_load(id);
            loaded->set_dirty(false);
        }
        catch(std::exception &e)
        {
            throw std::runtime_error("Failed to load '" + DEMANGLE(T) + "' with ID " + std::to_string(id) + " from the database. Exception: " + e.what());
        }

        return store_cache(loaded);
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
            //Delete from database
            database_erase(id);

            //Delete from cache if it's cached
            auto iter = cache.find(id);
            if(iter != cache.end())
            {
                iter->second = nullptr;
                return;
            }
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
        catch(const std::out_of_range &e) //If it's an at() error, then it's a logic error. Don't absorb.
        {
            throw;
        }
        catch(const SyntaxError &) //If the query has an error in it, it's a logic error, so don't absorb.
        {
            throw;
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
     * @return Returns what you pass it if it's a new cache entry. Returns the previously-cached object if this ID is occupied.
     */
    std::shared_ptr<T> store_cache(std::shared_ptr<T> obj)
    {
        if(!obj)
        {
            return {};
        }

        std::lock_guard<std::mutex> guard(lock);

        //Clean cache
        clean_cache();

        //Store the object in the cache
        auto ret = cache.emplace(obj->get_id(), std::move(obj));
        return ret.first->second;
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
     * @throws An std::exception on failure.
     * @param entry_id The ID of the entry to load
     */
    virtual std::shared_ptr<T> database_load(uint64_t entry_id) =0;

    /*!
     * Updates the entry if it's already
     * an existing entry in the database
     *
     * @throws An std::exception on failure
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
        //Don't do anything unless there's lots of stuff in the cache
        if(cache.size() < CACHE_CLEAR_THRESHOLD)
        {
            return;
        }

        //Free things which aren't in use
        for(auto iter = cache.begin(); iter != cache.end(); )
        {
            if(!iter->second)
            {
                iter = cache.erase(iter);
                continue;
            }

            if(iter->second.use_count() == 1) //If we're the only ones using it
            {
                try
                {
                    //Commit to database then erase
                    if(iter->second->get_dirty())
                    {
                        iter->second->set_dirty(false);
                        database_update(iter->second);
                    }
                }
                catch(const std::exception &e)
                {
                    throw std::runtime_error("Failed to commit '" + DEMANGLE(T) + "' with ID " + std::to_string(iter->second->get_id()) + " to the database. Exception: " + e.what());
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

constexpr bool equal( char const* a, char const* b )
{
    return *a == *b && (*a == '\0' || equal(a + 1, b + 1));
}

#define db_define_dirty() \
public: \
inline void set_dirty(bool val) {dirty = val;} \
inline bool get_dirty() {return dirty;} \
private: \
std::atomic<bool> dirty = false;

#define db_entry_def(type, name) \
public: \
template<typename T> \
inline void set_##name(T val) \
{ \
    (name) = std::move(val); \
    if constexpr(!equal("id", #name) && !equal("dirty", #name)) dirty = true; \
} \
template<typename T> \
inline void inc_##name(T val) \
{ \
    (name) += val; \
    if constexpr(!equal("id", #name) && !equal("dirty", #name)) dirty = true; \
} \
inline const auto &get_##name() \
{ \
    return (name); \
} \
private: \
type name;

#endif //SFTPMEDIASTREAMER_DATABASEREPOSITORY_H
