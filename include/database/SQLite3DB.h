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
#include "DBType.h"

class SQLite3DB
{
public:
    typedef std::unordered_map<std::string, std::vector<DBType>> query_t;
    SQLite3DB();
    ~SQLite3DB();


    /*!
     * Opens a database file from disk
     *
     * @param filepath The filepath to the database to open
     * @return True on success, false on failure
     */
    bool open(const std::string &filepath);

    /*!
     * Closes the database
     */
    void close();

    /*!
     * Returns the last error encountered by the database.
     * Thread safe.
     *
     * @return The string containing the error message
     */
    std::string get_last_error();

    /*!
     * Prepared statements query.
     * Throws an std::runtime_error on failure.
     *
     * @param query The query string. Variables should be replaced with '?', and passed in values
     * @param values The values of each of the SQL parameters
     * @return An std::unordered_map containing results, if any.
     */
    query_t query(const std::string &query, const std::vector<DBType> &values);

    /*!
     * Prepared insert statement query. Should be used to insert values.
     * Throws an std::runtime_error on failure.
     *
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
     * Executes a query.
     * Used for querying large amounts of data at once.
     *
     * @param query The query to execute
     * @param parameters The parameters to bind to the query
     * @param batch_size The number of results to return at once
     * @param callback A callback to repeatedly call when new data is available. A variable containing
     * the resulting columns will be returned. This variable will be the same for each callback in this query, except old
     * row data will be cleared between rows. Column data will not be cleared, and so iterators will remain valid. The
     * callback should return true if it wants to continue receiving more data, or false if it has had enough.
     */
    void query(const std::string &query, const std::vector<DBType> &parameters, size_t batch_size, const std::function<bool(query_t&)> &callback);

private:

    struct QueryContext
    {
        QueryContext()
        : stmt(nullptr){}

        sqlite3_stmt *stmt;
        std::vector<int> column_types{};
        std::vector<std::vector<DBType> *> column_iterators{};
        query_t results{};
    };

    sqlite3 *database;

    void bind_parameters(QueryContext &context, const std::vector<DBType> &parameters);

    QueryContext prepare_statement(const std::string &basic_string);

    bool step_query(QueryContext &context);
};


#endif //SFTPMEDIASTREAMER_SQLITE3WRAPPER_H
