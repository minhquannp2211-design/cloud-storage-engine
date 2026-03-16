#pragma once
#include <string>

class Database {

public:

    void executeUpdate(const std::string& query);

    void executeInsert(const std::string& query);

};