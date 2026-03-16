#include "NamespaceManager.h"

std::unordered_map<std::string, FileRecord> NamespaceManager::fileTable;
std::mutex NamespaceManager::fileTableMutex;

void NamespaceManager::RegisterFile(const FileRecord& file)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    fileTable[file.fileName] = file;
}

std::optional<FileRecord> NamespaceManager::GetFile(const std::string& name)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    auto it = fileTable.find(name);
    if (it == fileTable.end()) return std::nullopt;
    return it->second;
}

void NamespaceManager::DeleteFile(const std::string& name)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    fileTable.erase(name);
}

bool NamespaceManager::FileExists(const std::string& name)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    return fileTable.find(name) != fileTable.end();
}