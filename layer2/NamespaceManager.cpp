#include "NamespaceManager.h"
#include "../database/Database.h"
#include <iostream>
#include <sstream>

std::unordered_map<std::string, FileRecord> NamespaceManager::fileTable;
std::shared_ptr<NamespaceNode> NamespaceManager::root = std::make_shared<NamespaceNode>("", false);
std::mutex NamespaceManager::fileTableMutex;
bool NamespaceManager::initialized = false;

void NamespaceManager::Initialize()
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    
    if (initialized) return;
    
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Load all non-deleted files từ database
        auto results = db->executeSelect(
            "SELECT fileID, userID, fileName, path, totalSize, status, created_at, updated_at "
            "FROM files WHERE status != 'DELETED' ORDER BY path"
        );
        
        for (const auto& row : results)
        {
            if (row.size() < 8) continue;
            
            FileRecord rec;
            rec.fileID = std::stoi(row[0]);
            rec.userID = std::stoi(row[1]);
            rec.fileName = row[2];
            rec.path = row[3];
            rec.totalSize = std::stol(row[4]);
            rec.status = StringToFileStatus(row[5]);
            rec.created_at = std::stol(row[6]);
            rec.updated_at = std::stol(row[7]);
            
            // Load chunks
            auto chunkResults = db->executeSelect(
                "SELECT chunkHash FROM file_chunks WHERE fileID = " + std::to_string(rec.fileID) + 
                " ORDER BY sequence"
            );
            
            for (const auto& chunkRow : chunkResults)
            {
                if (!chunkRow.empty())
                    rec.chunkHashes.push_back(chunkRow[0]);
            }
            
            fileTable[rec.path] = rec;
            
            // Build tree structure
            std::string path = rec.path;
            size_t pos = 0;
            auto current = root;
            
            while ((pos = path.find('/')) != std::string::npos)
            {
                std::string part = path.substr(0, pos);
                current = current->GetOrCreateChild(part, false);
                path = path.substr(pos + 1);
            }
            
            // Final file node
            if (!path.empty())
            {
                auto fileNode = current->GetOrCreateChild(path, true);
                fileNode->fileRecord = &fileTable[rec.path];
            }
        }
        
        initialized = true;
        std::cout << "NamespaceManager initialized with " << fileTable.size() << " files\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error initializing NamespaceManager: " << e.what() << "\n";
        initialized = false;
    }
}

void NamespaceManager::RegisterFile(const FileRecord& file)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Insert/update file record
        std::string path = file.path.empty() ? file.fileName : file.path;
        
        time_t now = time(nullptr);
        
        std::string query = "INSERT INTO files (userID, fileName, path, totalSize, status, created_at, updated_at) "
                           "VALUES (" + std::to_string(file.userID) + ", '" + file.fileName + "', '" + 
                           path + "', " + std::to_string(file.totalSize) + ", '" + 
                           FileStatusToString(file.status) + "', " + std::to_string(file.created_at ? file.created_at : now) +
                           ", " + std::to_string(now) + ")";
        
        db->executeInsert(query);
        
        // Update in-memory cache
        FileRecord rec = file;
        rec.path = path;
        if (rec.created_at == 0) rec.created_at = now;
        rec.updated_at = now;
        
        fileTable[path] = rec;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error registering file: " << e.what() << "\n";
    }
}

std::optional<FileRecord> NamespaceManager::GetFile(const std::string& path)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    auto it = fileTable.find(path);
    if (it == fileTable.end()) return std::nullopt;
    return it->second;
}

void NamespaceManager::DeleteFile(const std::string& path)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Update status to DELETED instead of hard delete (for recovery)
        std::string query = "UPDATE files SET status = 'DELETED', updated_at = " + 
                           std::to_string(time(nullptr)) + " WHERE path = '" + path + "'";
        
        db->executeUpdate(query);
        
        // Remove from in-memory cache
        fileTable.erase(path);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error deleting file: " << e.what() << "\n";
    }
}

bool NamespaceManager::FileExists(const std::string& path)
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    return fileTable.find(path) != fileTable.end();
}

std::vector<FileRecord> NamespaceManager::GetAllFiles()
{
    std::lock_guard<std::mutex> g(fileTableMutex);
    std::vector<FileRecord> result;
    
    for (const auto& pair : fileTable)
    {
        result.push_back(pair.second);
    }
    
    return result;
}
