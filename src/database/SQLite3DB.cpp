//
// Created by fred on 19/12/17.
//

#include <thread>
#include <Types.h>
#include <iostream>
#include "database/SQLite3DB.h"

SQLite3DB::SQLite3DB()
: database(nullptr)
{

}

SQLite3DB::~SQLite3DB()
{
    close();
}

bool SQLite3DB::open(const std::string &filepath)
{
    //Open the database
    int ret = sqlite3_open(filepath.c_str(), &database);
    if(ret != SQLITE_OK)
        return false;

    //Set the options we want
    unsafe_query("PRAGMA foreign_keys = ON"); //Enforce constraints
    return true;
}

void SQLite3DB::close()
{
    while(sqlite3_close(database) != SQLITE_OK)
        std::this_thread::yield();
    database = nullptr;
}

std::string SQLite3DB::get_last_error()
{
    if(!database)
        return "";
    return sqlite3_errmsg(database);
}

SQLite3DB::query_t SQLite3DB::query(const std::string &query, const std::vector<DBType> &parameters)
{
    QueryContext context;
    auto cleanup = [&]() {
        sqlite3_finalize(context.stmt);
    };

    try
    {
        //Prepare the statement and bind parameters
        context = prepare_statement(query);
        bind_parameters(context, parameters);

        //Execute repeatedly until we have all results
        while(step_query(context));
    }
    catch(...)
    {
        cleanup();
        throw;
    }

    cleanup();
    return context.results;
}

void SQLite3DB::query(const std::string &query, const std::vector<DBType> &parameters, size_t batch_size,
                      const std::function<bool(query_t &)> &callback)
{
    QueryContext context;
    auto cleanup = [&]() {
        sqlite3_finalize(context.stmt);
    };

    try
    {
        //Prepare the statement and bind parameters
        context = prepare_statement(query);
        bind_parameters(context, parameters);

        //Execute repeatedly until we have all results
        size_t queries_made = 0;
        while(step_query(context))
        {
            if(++queries_made > batch_size)
            {
                //Call the callback then clear row data
                callback(context.results);
                for(auto &col : context.results)
                    col.second.clear();
                queries_made = 0;
            }
        }

        if(queries_made > 0)
            callback(context.results);
    }
    catch(...)
    {
        cleanup();
        throw;
    }

    cleanup();
}


uint64_t SQLite3DB::insert_query(const std::string &query_str, const std::vector<DBType> &values)
{
    query(query_str, values);
    return static_cast<uint64_t>(sqlite3_last_insert_rowid(database));
}

void SQLite3DB::unsafe_query(const std::string &query)
{
    int ret = sqlite3_exec(database, query.c_str(), nullptr, nullptr, nullptr);
    if(ret != SQLITE_OK)
        throw std::runtime_error("Unsafe query: \"" + query + "\" failed. Error: " + sqlite3_errstr(ret));
}

void SQLite3DB::bind_parameters(QueryContext &context, const std::vector<DBType> &parameters)
{
    //Bind parameters
    auto expected_parameter_count = static_cast<uint32_t>(sqlite3_bind_parameter_count(context.stmt));
    if(expected_parameter_count != parameters.size())
        throw std::runtime_error("Prepared statement expects " + std::to_string(expected_parameter_count) + " parameters. But " + std::to_string(parameters.size()) +  " has been provided.");

    for(uint32_t a = 0; a < parameters.size(); ++a)
    {
        if(parameters[a].get_type() == DBType::INTEGRAL)
        {
            sqlite3_bind_int64(context.stmt, a + 1, parameters[a].get<sqlite3_int64>());
        }
        else if(parameters[a].get_type() == DBType::FLOATING_POINT)
        {
            sqlite3_bind_double(context.stmt, a + 1, parameters[a].get<double>());
        }
        else if(parameters[a].get_type() == DBType::STRING)
        {
            const std::string &str = parameters[a].get_buffer();
            sqlite3_bind_text(context.stmt, a + 1, str.c_str(), static_cast<int>(str.size()), SQLITE_TRANSIENT);
        }
        else if(parameters[a].get_type() == DBType::BLOB)
        {
            const std::string &str = parameters[a].get_buffer();
            sqlite3_bind_blob64(context.stmt, a + 1, str.c_str(), str.size(), SQLITE_TRANSIENT);
        }
        else
        {
            throw std::logic_error("Unsupported type passed to query: " + std::to_string(parameters[a].get_type()));
        }
    }
}

SQLite3DB::QueryContext SQLite3DB::prepare_statement(const std::string &query)
{
    QueryContext context;
    int ret = sqlite3_prepare_v2(database, query.c_str(), static_cast<int>(query.size()), &context.stmt, nullptr);
    if(ret != SQLITE_OK)
        throw std::runtime_error("sqlite3_prepare() failed: " + std::string(sqlite3_errstr(ret)));

    return context;
}

bool SQLite3DB::step_query(QueryContext &context)
{
    //Execute
    int ret = sqlite3_step(context.stmt);

    //Create result columns if not done, this must be done after sqlite3_step and not before, unfortunately
    if(context.column_iterators.empty())
    {
        int column_count = sqlite3_column_count(context.stmt);
        for(int col = 0; col < column_count; ++col)
        {
            std::string col_name = sqlite3_column_name(context.stmt, col);
            auto iter = context.results.emplace(std::make_pair(std::move(col_name), std::vector<DBType>{})).first;
            context.column_iterators.emplace_back(&iter->second);

            int type = sqlite3_column_type(context.stmt, col);
            context.column_types.emplace_back(type);
        }
    }

    //Exit if query is over. We do this after column creation to ensure that the result
    //Columns exist, as they should exist even if there are no results.
    if(ret == SQLITE_DONE)
        return false;
    else if(ret != SQLITE_ROW)
        throw std::logic_error("sqlite3_step() failed: " + std::string(sqlite3_errstr(ret)));

    //Get results
    for(size_t col = 0; col < context.column_types.size(); ++col)
    {
        switch(context.column_types[col])
        {
            case SQLITE_INTEGER:
            {
                int64_t value = sqlite3_column_int64(context.stmt, static_cast<int>(col));
                context.column_iterators[col]->emplace_back(value);
                break;
            }
            case SQLITE_BLOB:
            {
                auto *data_ptr = static_cast<const char*>(sqlite3_column_blob(context.stmt, static_cast<int>(col)));
                auto data_len = static_cast<size_t>(sqlite3_column_bytes(context.stmt, static_cast<int>(col)));
                std::string data = std::string(data_ptr, data_len);
                context.column_iterators[col]->emplace_back(std::move(data));
                break;
            }
            case SQLITE_NULL:
            {
                context.column_iterators[col]->emplace_back(nullptr);
                break;
            }
            case SQLITE_TEXT:
            {
                const char *data_ptr = (char*)sqlite3_column_text(context.stmt, static_cast<int>(col));
                int32_t data_len = sqlite3_column_bytes(context.stmt, static_cast<int>(col));
                std::string data = std::string(data_ptr, static_cast<unsigned long>(data_len));
                context.column_iterators[col]->emplace_back(std::move(data));
                break;
            }
            case SQLITE_FLOAT:
            {
                double value = sqlite3_column_double(context.stmt, static_cast<int>(col));
                context.column_iterators[col]->emplace_back(value);
                break;
            }
            default:
                throw std::logic_error("Unsupported SQLite3 type: " + std::to_string(context.column_types[col]));
        }
    }
    return true;
}
