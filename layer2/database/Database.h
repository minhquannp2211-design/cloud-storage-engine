#pragma once
#include <string>
#include <vector>
#include <memory>

// SQLite3 include - may not be available on all systems
#ifdef HAVE_SQLITE3
#include <sqlite3.h>
#else
// Forward declaration if sqlite3.h is not available
typedef struct sqlite3 sqlite3;
#endif

class Database {

private:
    sqlite3* db{nullptr};
    static std::shared_ptr<Database> instance;
    
    Database() = default;
    bool InitializeSchema();

public:
    // Singleton
    static std::shared_ptr<Database> GetInstance(const std::string& dbPath = "metadata.db");
    
    ~Database();

    // Basic CRUD
    void executeUpdate(const std::string& query);
    void executeInsert(const std::string& query);
    std::vector<std::vector<std::string>> executeSelect(const std::string& query);
    
    // Transaction support
    void BeginTransaction();
    void Commit();
    void Rollback();
    bool IsInTransaction() const { return inTransaction; }
    
private:
    bool inTransaction{false};
    std::string dbFilePath;
};
