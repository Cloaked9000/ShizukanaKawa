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

SQLite3DB::SQLite3DB(const std::string &filepath, bool default_pragmas)
: SQLite3DB()
{
    if(!open(filepath, default_pragmas))
    {
        throw std::runtime_error("Failed to open database");
    }
}

SQLite3DB::~SQLite3DB()
{
    try
    {
        close();
    }
    catch(const std::exception &e)
    {
        frlog << Log::warn << "An exception occurred whilst closing an SQLite database: " << e.what() << Log::end;
    }
    catch(const char *msg)
    {
        frlog << Log::warn << "An exception occurred whilst closing an SQLite database: " << msg << Log::end;
    }
    catch(...)
    {
        frlog << Log::warn << "An unknown error occurred whilst closing an SQLite database!" << Log::end;
    }
}

bool SQLite3DB::open(const std::string &filepath, bool default_pragmas)
{
    //Open the database
    database_filepath = filepath;
    int ret = sqlite3_open_v2(filepath.c_str(), &database, SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if(ret != SQLITE_OK)
    {
        if(ret != SQLITE_CANTOPEN) //Only print this if it's an interesting error (as in, not that it just doesn't exist)
        {
            frlog << Log::warn << "Failed to open database '" << filepath << "': " << sqlite3_errstr(ret) << Log::end;
        }

        close();
        return false;
    }

    //Pick optimal SQLite3 settings for our needs
    sqlite3_busy_timeout(database, 30000);
    try
    {
        if(!default_pragmas)
        {
            init_pragmas();
        }
    }
    catch(const std::exception &e)
    {
        frlog << Log::warn << "An exception occurred whilst setting up the SQLite database instance for '" << filepath << "': " << e.what() << Log::end;
        close();
        return false;
    }

    return true;
}

void SQLite3DB::close()
{
    while(sqlite3_close(database) != SQLITE_OK)
    {
        std::this_thread::yield(); //todo: add timeout
    }
    database = nullptr;
}

SQLite3DB::query_t SQLite3DB::query(const std::string &query, const std::vector<DBType> &parameters)
{
    //Prepare the query
    query_t results;

    //Compile the statement
    CompiledStatement stmt = compile_statement(query);

    //Bind parameters
    stmt.bind_parameters(parameters);

    //Execute repeatedly until we have all results
    while(stmt.step(database, results));

    return results;
}

void SQLite3DB::query(SQLite3DB::CompiledStatement &query, const std::vector<DBType> &values,
                                query_t &results, size_t batch_size, const std::function<bool()> &callback)
{
    query.bind_parameters(values);

    size_t queries_made = 0;
    while(query.step(database, results))
    {
        if(++queries_made > batch_size)
        {
            //Call the callback then clear row data
            if(!callback())
            {
                break;
            }
            for(auto &col : results)
            {
                col.second.clear();
            }
            queries_made = 0;
        }
    }

    if(queries_made > 0)
    {
        callback();
    }
    query.reset();
}

SQLite3DB::query_t SQLite3DB::query(SQLite3DB::CompiledStatement &query, const std::vector<DBType> &values)
{
    query_t results;
    query.bind_parameters(values);
    while(query.step(database, results));
    query.reset();
    return results;
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
    {
        throw std::runtime_error("Unsafe query: \"" + query + "\" failed. Error(" + std::to_string(ret) + "): " + sqlite3_errstr(ret) + ". Database error: " + std::string(sqlite3_errmsg(database)));
    }
}

SQLite3DB::CompiledStatement SQLite3DB::compile_statement(const std::string &query)
{
    CompiledStatement stmt;
    int ret = sqlite3_prepare_v2(database, query.c_str(), static_cast<int>(query.size()), &stmt.statement, nullptr);
    if(ret != SQLITE_OK)
    {
        throw std::runtime_error("sqlite3_prepare() failed (" + std::to_string(ret) + "): " + std::string(sqlite3_errstr(ret)) + ". Database error: " + std::string(sqlite3_errmsg(database)) + ". Query: " + query);
    }
    return stmt;
}

void SQLite3DB::CompiledStatement::bind_parameters(const std::vector<DBType> &parameters)
{
    //Clear the statement first
    reset();

    //Bind parameters
    auto expected_parameter_count = static_cast<uint32_t>(sqlite3_bind_parameter_count(statement));
    if(expected_parameter_count != parameters.size())
    {
        throw std::runtime_error("Prepared statement expects " + std::to_string(expected_parameter_count) + " parameters. But " + std::to_string(parameters.size()) +  " has been provided.");
    }

    static_assert(DBType::TYPE_COUNT == 7, "Unknown types added");
    for(uint32_t a = 0; a < parameters.size(); ++a)
    {
        if(parameters[a].get_type() == DBType::UNSINGED_INTEGRAL)
        {
            sqlite3_bind_int64(statement, a + 1, parameters[a].get<uint64_t>());
        }
        else if(parameters[a].get_type() == DBType::SIGNED_INTEGRAL)
        {
            sqlite3_bind_int64(statement, a + 1, parameters[a].get<int64_t>());
        }
        else if(parameters[a].get_type() == DBType::FLOATING_POINT)
        {
            sqlite3_bind_double(statement, a + 1, parameters[a].get<double>());
        }
        else if(parameters[a].get_type() == DBType::STRING)
        {
            const auto &str = parameters[a].get<std::string>();
            sqlite3_bind_text(statement, a + 1, str.c_str(), static_cast<int>(str.size()), SQLITE_STATIC);
        }
        else if(parameters[a].get_type() == DBType::NILL)
        {
            sqlite3_bind_null(statement, a + 1);
        }
        else if(parameters[a].get_type() == DBType::UNIX_TIME)
        {
            abort(); //not currently implemented
        }
        else if(parameters[a].get_type() == DBType::BLOB)
        {
            const auto &str = parameters[a].get<std::string>();
            sqlite3_bind_blob(statement, a + 1, str.c_str(), static_cast<int>(str.size()), SQLITE_STATIC);
        }
        else
        {
            throw std::logic_error("Unsupported type passed to query: " + std::to_string(parameters[a].get_type()));
        }
    }
}

void SQLite3DB::CompiledStatement::reset()
{
    if(statement)
    {
        sqlite3_reset(statement);
    }
    column_count = 0;
    column_iterators.clear();
    column_types.clear();
}

bool SQLite3DB::CompiledStatement::step(sqlite3 *database, query_t &results)
{
    //Execute
    int ret = sqlite3_step(statement);

    //Create result columns if not done, this must be done after sqlite3_step and not before, unfortunately
    if(column_iterators.empty())
    {
        column_count = sqlite3_column_count(statement);
        column_types.reserve(column_count);
        column_iterators.reserve(column_count);
        for(int col = 0; col < column_count; ++col)
        {
            std::string col_name = sqlite3_column_name(statement, col);
            auto iter = results.emplace(std::make_pair(std::move(col_name), std::vector<DBType>{})).first;
            column_iterators.emplace_back(&iter->second);

            int type = sqlite3_column_type(statement, col);
            column_types.emplace_back(type);
        }
    }

    switch(ret)
    {
        case SQLITE_ROW: //Returned results
            break;
        case SQLITE_DONE: //No more results
            return false;
        default:
            throw std::logic_error("sqlite3_step() failed: " + std::string(sqlite3_errstr(ret)) + ". " + sqlite3_errmsg(database));
    }


    //Get results
    for(size_t col = 0; col < column_types.size(); ++col)
    {
        switch(column_types[col])
        {
            case SQLITE_INTEGER:
            {
                uint64_t value = sqlite3_column_int64(statement, static_cast<int64_t>(col));
                column_iterators[col]->emplace_back(value);
                break;
            }
            case SQLITE_BLOB:
            {
                auto *data_ptr = static_cast<const char*>(sqlite3_column_blob(statement, static_cast<int>(col)));
                auto data_len = static_cast<size_t>(sqlite3_column_bytes(statement, static_cast<int>(col)));
                std::string data = std::string(data_ptr, data_len);
                column_iterators[col]->emplace_back(std::move(data));
                break;
            }
            case SQLITE_NULL:
            {
                column_iterators[col]->emplace_back(nullptr);
                break;
            }
            case SQLITE_TEXT:
            {
                const char *data_ptr = (char*)sqlite3_column_text(statement, static_cast<int>(col));
                int32_t data_len = sqlite3_column_bytes(statement, static_cast<int>(col));
                std::string data = std::string(data_ptr, data_len);
                column_iterators[col]->emplace_back(std::move(data));
                break;
            }
            case SQLITE_FLOAT:
            {
                double value = sqlite3_column_double(statement, static_cast<int>(col));
                column_iterators[col]->emplace_back(value);
                break;
            }
            default:
                throw std::logic_error("Unsupported SQLite3 type: " + std::to_string(column_types[col]));
        }
    }
    return true;
}

void SQLite3DB::init_pragmas()
{
    unsafe_query("PRAGMA journal_mode = WAL;");
    unsafe_query("PRAGMA synchronous = NORMAL;");
    unsafe_query("PRAGMA temp_store = MEMORY;");
}
