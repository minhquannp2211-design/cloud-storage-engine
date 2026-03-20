#include "FileStructureBuilder.h"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <iostream>
#include "../database/Database.h"
#include "../models/FileStatus.h"

static std::atomic<int> g_nextFileID{1};
static std::unordered_map<int, FileRecord> g_fileMeta;
static std::unordered_map<int, std::vector<std::string>> g_fileLayout;
static std::mutex g_fs_mutex;
static bool g_initialized = false;

void FileStructureBuilder::Initialize()
{
    std::lock_guard<std::mutex> g(g_fs_mutex);
    
    if (g_initialized) return;
    
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Load all files từ DB
        auto results = db->executeSelect(
            "SELECT fileID, userID, fileName, path, totalSize, status, created_at, updated_at "
            "FROM files WHERE status != 'DELETED' ORDER BY fileID DESC LIMIT 1"
        );
        
        // Rebuild g_nextFileID từ max(fileID) + 1
        if (!results.empty() && !results[0].empty())
        {
            int maxID = std::stoi(results[0][0]);
            g_nextFileID.store(maxID + 1);
        }
        
        // Load all files từ DB
        auto allResults = db->executeSelect(
            "SELECT fileID, userID, fileName, path, totalSize, status, created_at, updated_at "
            "FROM files WHERE status != 'DELETED'"
        );
        
        for (const auto& row : allResults)
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
            
            g_fileMeta[rec.fileID] = rec;
            g_fileLayout[rec.fileID] = rec.chunkHashes;
        }
        
        g_initialized = true;
        std::cout << "FileStructureBuilder initialized with " << g_fileMeta.size() << " files, "
                  << "next fileID = " << g_nextFileID.load() << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error initializing FileStructureBuilder: " << e.what() << "\n";
        g_initialized = false;
    }
}

int FileStructureBuilder::CreateFileEntry(const std::string& fileName, int userId, long totalSize, const std::string& path)
{
    int id = static_cast<int>(g_nextFileID.fetch_add(1));

    FileRecord rec;
    rec.fileID = id;
    rec.userID = userId;
    rec.fileName = fileName;
    rec.path = path.empty() ? fileName : path;
    rec.totalSize = totalSize;
    rec.status = FileStatus::UPLOADING;  // Start with UPLOADING status
    rec.created_at = time(nullptr);
    rec.updated_at = rec.created_at;

    {
        std::lock_guard<std::mutex> g(g_fs_mutex);
        g_fileMeta[id] = rec;
    }

    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Insert vào DB với status UPLOADING
        std::string query = "INSERT INTO files (fileID, userID, fileName, path, totalSize, status, created_at, updated_at) "
                           "VALUES (" + std::to_string(id) + ", " + std::to_string(userId) + ", '" + 
                           fileName + "', '" + rec.path + "', " + std::to_string(totalSize) + ", '" +
                           FileStatusToString(FileStatus::UPLOADING) + "', " + std::to_string(rec.created_at) +
                           ", " + std::to_string(rec.updated_at) + ")";
        
        db->executeInsert(query);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error inserting file entry: " << e.what() << "\n";
    }

    return id;
}

void FileStructureBuilder::LinkChunksToFile(int fileID, const std::vector<std::string>& listChunkHashes)
{
    std::lock_guard<std::mutex> g(g_fs_mutex);
    
    g_fileLayout[fileID] = listChunkHashes;
    
    auto it = g_fileMeta.find(fileID);
    if (it != g_fileMeta.end())
    {
        it->second.chunkHashes = listChunkHashes;
        it->second.updated_at = time(nullptr);
    }

    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Insert sequence entries vào intermediate table
        for (size_t i = 0; i < listChunkHashes.size(); ++i)
        {
            const auto& h = listChunkHashes[i];
            std::string query = "INSERT INTO file_chunks (fileID, sequence, chunkHash) VALUES (" + 
                               std::to_string(fileID) + ", " + std::to_string(i) + ", '" + h + "')";
            db->executeInsert(query);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error linking chunks: " << e.what() << "\n";
    }
}

std::vector<std::string> FileStructureBuilder::GetFileLayout(int fileID)
{
    std::lock_guard<std::mutex> g(g_fs_mutex);
    auto it = g_fileLayout.find(fileID);
    if (it == g_fileLayout.end()) return {};
    return it->second;
}

FileRecord FileStructureBuilder::GetFileMetadata(int fileID)
{
    std::lock_guard<std::mutex> g(g_fs_mutex);
    auto it = g_fileMeta.find(fileID);
    if (it == g_fileMeta.end()) return FileRecord{};
    return it->second;
}

void FileStructureBuilder::UpdateFileStatus(int fileID, FileStatus newStatus)
{
    {
        std::lock_guard<std::mutex> g(g_fs_mutex);
        auto it = g_fileMeta.find(fileID);
        if (it != g_fileMeta.end())
        {
            it->second.status = newStatus;
            it->second.updated_at = time(nullptr);
        }
    }

    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        std::string query = "UPDATE files SET status = '" + std::string(FileStatusToString(newStatus)) + 
                           "', updated_at = " + std::to_string(time(nullptr)) + " WHERE fileID = " + 
                           std::to_string(fileID);
        
        db->executeUpdate(query);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error updating file status: " << e.what() << "\n";
    }
}

FileRecord FileStructureBuilder::BuildFileRecord(
    const std::string& fileName,
    const std::vector<std::string>& chunks)
{
    FileRecord record;
    record.fileName = fileName;
    record.chunkHashes = chunks;
    return record;
}
