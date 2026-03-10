#include "Database.h"
#include <iostream>

void Database::executeUpdate(const std::string& query) {
    std::cout << "Executing: " << query << std::endl;
}

void Database::executeInsert(const std::string& query) {
    std::cout << "Insert: " << query << std::endl;
}