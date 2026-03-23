#pragma once
#include <string>
#include <vector>
#include <memory>

#ifdef HAVE_SQLITE3
#include <sqlite3.h>
#else
typedef struct sqlite3 sqlite3;
#endif

class Database {
private:
    sqlite3* db{nullptr};
    static std::shared_ptr<Database> instance;

    Database() = default;
    bool InitializeSchema();

public:
    static std::shared_ptr<Database> GetInstance(const std::string& dbPath = "metadata.db");
    static void ResetInstance();

    ~Database();

    void Close();

    void executeUpdate(const std::string& query);
    void executeInsert(const std::string& query);
    std::vector<std::vector<std::string>> executeSelect(const std::string& query);

    void BeginTransaction();
    void Commit();
    void Rollback();
    bool IsInTransaction() const { return inTransaction; }

private:
    bool inTransaction{false};
    std::string dbFilePath;
};