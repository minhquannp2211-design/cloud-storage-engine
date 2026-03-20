#include "Database.h"
#include <iostream>
#include <stdexcept>

std::shared_ptr<Database> Database::instance = nullptr;

std::shared_ptr<Database> Database::GetInstance(const std::string& dbPath)
{
    if (!instance)
    {
        instance = std::shared_ptr<Database>(new Database());
        instance->dbFilePath = dbPath;
        
        // Open database
        int rc = sqlite3_open(dbPath.c_str(), &instance->db);
        if (rc != SQLITE_OK)
        {
            throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(instance->db)));
        }
        
        // Enable foreign keys
        sqlite3_exec(instance->db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
        
        // Initialize schema
        if (!instance->InitializeSchema())
        {
            throw std::runtime_error("Failed to initialize database schema");
        }
    }
    return instance;
}

bool Database::InitializeSchema()
{
    if (!db) return false;
    
    const char* schema = R"(
    CREATE TABLE IF NOT EXISTS files (
        fileID INTEGER PRIMARY KEY AUTOINCREMENT,
        userID INTEGER NOT NULL,
        fileName TEXT NOT NULL,
        path TEXT,
        totalSize INTEGER NOT NULL,
        status TEXT DEFAULT 'COMMITTED',
        created_at INTEGER,
        updated_at INTEGER,
        UNIQUE(path, userID)
    );
    
    CREATE TABLE IF NOT EXISTS file_chunks (
        chunkID INTEGER PRIMARY KEY AUTOINCREMENT,
        fileID INTEGER NOT NULL,
        sequence INTEGER NOT NULL,
        chunkHash TEXT NOT NULL,
        FOREIGN KEY(fileID) REFERENCES files(fileID) ON DELETE CASCADE,
        UNIQUE(fileID, sequence)
    );
    
    CREATE TABLE IF NOT EXISTS transactions (
        transactionID INTEGER PRIMARY KEY AUTOINCREMENT,
        fileID INTEGER,
        operation TEXT NOT NULL,
        status TEXT NOT NULL,
        created_at INTEGER,
        FOREIGN KEY(fileID) REFERENCES files(fileID)
    );
    
    CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);
    CREATE INDEX IF NOT EXISTS idx_files_status ON files(status);
    CREATE INDEX IF NOT EXISTS idx_file_chunks_fileID ON file_chunks(fileID);
    )";
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, schema, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

Database::~Database()
{
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

void Database::executeUpdate(const std::string& query)
{
    if (!db)
        throw std::runtime_error("Database not initialized");
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK)
    {
        std::string error = "SQL error: " + std::string(errMsg ? errMsg : "unknown");
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }
}

void Database::executeInsert(const std::string& query)
{
    executeUpdate(query);
}

std::vector<std::vector<std::string>> Database::executeSelect(const std::string& query)
{
    if (!db)
        throw std::runtime_error("Database not initialized");
    
    std::vector<std::vector<std::string>> results;
    sqlite3_stmt* stmt = nullptr;
    
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error("SQL error: " + std::string(sqlite3_errmsg(db)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::vector<std::string> row;
        int cols = sqlite3_column_count(stmt);
        
        for (int i = 0; i < cols; ++i)
        {
            const char* val = (const char*)sqlite3_column_text(stmt, i);
            row.push_back(val ? std::string(val) : "");
        }
        
        results.push_back(row);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

void Database::BeginTransaction()
{
    if (inTransaction)
        throw std::runtime_error("Transaction already in progress");
    
    executeUpdate("BEGIN TRANSACTION;");
    inTransaction = true;
}

void Database::Commit()
{
    if (!inTransaction)
        throw std::runtime_error("No transaction in progress");
    
    executeUpdate("COMMIT;");
    inTransaction = false;
}

void Database::Rollback()
{
    if (!inTransaction)
        throw std::runtime_error("No transaction in progress");
    
    executeUpdate("ROLLBACK;");
    inTransaction = false;
}
