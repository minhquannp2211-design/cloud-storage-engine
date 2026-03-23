#pragma once
#include <string>

class DeleteService {
public:
    // Delete file với transaction support
    // Transaction: DELETING -> DELETED
    // Idempotent: safe to call multiple times
    bool DeleteFile(const std::string& path);
};