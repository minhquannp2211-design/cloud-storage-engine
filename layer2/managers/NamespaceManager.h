#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include "../models/FileRecord.h"

class NamespaceManager
{
private:
    static std::unordered_map<std::string, FileRecord> fileTable;
    static std::mutex fileTableMutex;

public:
    void RegisterFile(const FileRecord& file);

    std::optional<FileRecord> GetFile(const std::string& name);

    void DeleteFile(const std::string& name);

    bool FileExists(const std::string& name);
};