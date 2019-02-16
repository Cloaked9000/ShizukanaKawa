//
// Created by fred on 19/12/17.
//

#ifndef SFTPMEDIASTREAMER_SQLITE3WRAPPER_H
#define SFTPMEDIASTREAMER_SQLITE3WRAPPER_H
#include <string>
#include <sqlite3.h>
#include <vector>
#include <unordered_map>
#include <any>
#include <functional>
#include <Log.h>
#include "DBType.h"

class SQLite3DB
{
public:
    typedef std::unordered_map<std::string, std::vector<DBType>> query_t;
    class CompiledStatement
    {
    public:
        friend class SQLite3DB;
        CompiledStatement()
                : CompiledStatement(nullptr)
        {
        }

        CompiledStatement(CompiledStatement &&o) noexcept
        {
            //Move things over
            sqlite3_finalize(statement);
            statement = o.statement;
            column_count = o.column_count;
            column_iterators = std::move(o.column_iterators);
            column_types = std::move(o.column_types);

            //We don't want the other's destructor finalizing the statement
            o.statement = nullptr;
            o.column_count = 0;
            o.column_iterators.clear();
            o.column_types.clear();
        }

        CompiledStatement &operator=(CompiledStatement &&o) noexcept
        {
            //Move things over
            sqlite3_finalize(statement);
            statement = o.statement;
            column_count = o.column_count;
            column_iterators = std::move(o.column_iterators);
            column_types = std::move(o.column_types);

            //We don't want the other's destructor finalizing the statement
            o.statement = nullptr;
            o.column_count = 0;
            o.column_iterators.clear();
            o.column_types.clear();

            return *this;
        }

        ~CompiledStatement()
        {
            //Calling finalize on a nullptr is fine
            sqlite3_finalize(statement);
            statement = nullptr;
        };

        //Disable copying, as we need to be careful with the contained C objects
        CompiledStatement(const CompiledStatement&)=delete;
        CompiledStatement &operator=(const CompiledStatement &)=delete;
    private:
        explicit CompiledStatement(sqlite3_stmt *stmt)
                : statement(stmt) {}

        /*!
         * Gets the internal statement
         *
         * @return The internal statement
         */
        explicit operator sqlite3_stmt *()
        {
            return statement;
        }

        /*!
         * Binds a set of parameters to the query.
         * Reset is automatically called.
         *
         * @param parameters The parameters to bind
         */
        void bind_parameters(const std::vector<DBType> &parameters);

        /*!
         * Resets the query, so it's ready to have more parameters bound.
         */
        void reset();

        /*!
         * Steps through the current result set,
         * should be called repeatedly until the false is returned.
         *
         * @param database The current database
         * @param results Where to store the results. The *SAME* variable must be passed
         * with each step call in between resets.
         * @return True if there's more results, so step may be called again. False if no more results.
         */
        bool step(sqlite3 *database, query_t &results);

        //State
        sqlite3_stmt *statement;
        int column_count = 0;
        std::vector<std::vector<DBType>*> column_iterators;
        std::vector<int> column_types;
    };

    SQLite3DB();
    ~SQLite3DB();
    SQLite3DB(const SQLite3DB &)=delete;
    SQLite3DB(SQLite3DB &&)=delete;
    void operator=(SQLite3DB &&)=delete;
    void operator=(const SQLite3DB &)=delete;

    /*!
     * Constructs the object and opens a database
     *
     * @note If the DB is corrupt, then opening may fail if default_pragmas is not set to true
     * @throws An std::runtime_error on failure
     * @param default_pragmas True if sqlite's default pragmas should be used. False if we should use our own.
     * @param filepath The filepath to the database to create/open
     */
    SQLite3DB(const std::string &filepath, bool default_pragmas = false);

    /*!
     * Opens a database file from disk
     *
     * @note If the DB is corrupt, then opening may fail if default_pragmas is not set to true
     * @param filepath The filepath to the database to open
     * @param default_pragmas True if sqlite's default pragmas should be used. False if we should use our own.
     * @return True on success, false on failure
     */
    bool open(const std::string &filepath, bool default_pragmas = false);

    /*!
     * Closes the database
     */
    void close();

    /*!
     * Executes a query, using prepared statements.
     *
     * @throws an std::exception on failure.
     * @param query The query string. Variables should be replaced with '?', and passed in values
     * @param values The values of each of the SQL parameters
     * @return An std::unordered_map containing results, if any.
     */
    query_t query(const std::string &query, const std::vector<DBType> &values);

    /*!
     * Executes a statement which has been previously compiled.
     *
     * @throws an std::exception on failure.
     * @param query The pre-compiled query. Compiled using compile_statement.
     * @param values The values of each of the SQL parameters
     * @return An std::unordered_map containing results, if any.
     */
    query_t query(CompiledStatement &query, const std::vector<DBType> &values);

    /*!
     * Executes a statement which has been previously compiled.
     * Used for querying large amounts of data at once.
     *
     * @param query The query to execute
     * @param values The parameters to bind to the query
     * @param results A place for the function to store results. This will be re-filled prior to each callback call.
     * @param batch_size The number of results to return at once
     * @param callback A callback to repeatedly call when new data is available. It should return true if it wants
     * more entries, or false if it's had enough.
     */
    void query(CompiledStatement &query, const std::vector<DBType> &values, query_t &results, size_t batch_size, const std::function<bool()> &callback);

    /*!
     * Prepared insert statement query. Should be used to insert values.
     *
     * @throws an std::runtime_error on failure.
     * @param query The query string. Variables should be replaced with '?', and passed in values
     * @param values The values of each of the SQL parameters
     * @return The ID of the inserted row
     */
    uint64_t insert_query(const std::string &query, const std::vector<DBType> &values);

    /*!
     * Unsafe query, not safe from SQL injection. Should be used for
     * things like creating tables/modifying settings.
     *
     * @param query The query to run.
     */
    void unsafe_query(const std::string &query);


    /*!
     * Compiles a query and returns an object which can be
     * passed to future queries, if efficiency is key.
     *
     * @throws An std::exception on failure
     * @param statement The statement to query
     * @return The compiled statement object.
     */
    CompiledStatement compile_statement(const std::string &statement);

    /*!
     * Starts a new transaction,
     * should be used before a batch of inserts.
     */
    inline void lock()
    {
        unsafe_query("BEGIN");
    }

    /*!
     * Rolls back and ends the current transaction
     */
    inline void rollback()
    {
        unsafe_query("ROLLBACK");
    }

    /*!
     * Ends the current transaction
     * Should be used to end a transaction opened
     * with begin().
     */
    inline void unlock()
    {
        try
        {
            unsafe_query("END");
        }
        catch(const std::exception &e)
        {
            frlog << Log::warn << "Ending transaction threw: " << e.what() << Log::end;
        }
    }

    template<typename EntryType, typename QueryType, typename Functor, typename ColType, std::size_t col_count>
    bool for_each(QueryType &query_val, const std::vector<DBType> &values,
                  const std::array<ColType, col_count> &expected_column_names,
                  const Functor &functor)
    {
        query_t results;
        std::array<DBType, expected_column_names.size()> constructor_args;
        std::array<query_t::iterator, expected_column_names.size()> column_iterators;
        static_assert(!column_iterators.empty(), "There must be results");

        bool running = true;
        query(query_val, values, results, 100, [&]() -> bool {

            //Make sure we have cached iterators to each result column
            if(column_iterators[0] == results.end())
            {
                for(size_t col = 0; col < expected_column_names.size(); ++col)
                {
                    column_iterators[col] = results.find(expected_column_names[col]);
                    if(column_iterators[col] == results.end())
                    {
                        throw std::runtime_error("Couldn't find expected column '" + std::string(expected_column_names[col]) + "' in results set of for each");
                    }
                }
            }

            //Call our functor for each entry
            for(size_t a = 0; a < column_iterators[0]->second.size() && running; ++a)
            {
                for(size_t b = 0; b < constructor_args.size(); ++b)
                {
                    constructor_args[b] = std::move(column_iterators[b]->second[a]);
                }

                auto entry{std::make_from_tuple<EntryType>(std::move(constructor_args))};
                running = functor(std::make_shared<EntryType>(std::move(entry)));
            }

            return running;
        });

        return running;
    }

private:
    /*!
     * Sets pragmas to desired settings
     */
    void init_pragmas();

    /*!
     * Restores SQLite's default pragma values
     */
    void reset_pragmas();

    sqlite3 *database;
    std::string database_filepath;
};
#endif //SFTPMEDIASTREAMER_SQLITE3WRAPPER_H
