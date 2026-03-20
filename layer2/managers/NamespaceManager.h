#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <memory>
#include "../models/FileRecord.h"
#include "../models/NamespaceNode.h"

class NamespaceManager
{
private:
    static std::unordered_map<std::string, FileRecord> fileTable;  // Path -> FileRecord
    static std::shared_ptr<NamespaceNode> root;                    // Root của directory tree
    static std::mutex fileTableMutex;
    static bool initialized;

public:
    // Initialize: load tất cả files từ database
    static void Initialize();
    
    // Register file vào namespace và DB
    void RegisterFile(const FileRecord& file);

    // Get file by path
    std::optional<FileRecord> GetFile(const std::string& path);

    // Delete file khỏi namespace và DB
    void DeleteFile(const std::string& path);

    bool FileExists(const std::string& path);
    
    // Get all files
    std::vector<FileRecord> GetAllFiles();
    
    // Get root directory
    std::shared_ptr<NamespaceNode> GetRoot() const { return root; }
};
