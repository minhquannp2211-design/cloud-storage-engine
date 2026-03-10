#include "FileStructureBuilder.h"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "../database/Database.h"

static std::atomic<int> g_nextFileID{1};
static std::unordered_map<int, FileRecord> g_fileMeta;
static std::unordered_map<int, std::vector<std::string>> g_fileLayout;
static std::mutex g_fs_mutex;

int FileStructureBuilder::CreateFileEntry(const std::string& fileName, int userId, long totalSize)
{
    int id = static_cast<int>(g_nextFileID.fetch_add(1));

    FileRecord rec;
    rec.fileID = id;
    rec.userID = userId;
    rec.fileName = fileName;
    rec.totalSize = totalSize;

    {
        std::lock_guard<std::mutex> g(g_fs_mutex);
        g_fileMeta[id] = rec;
    }

    // Log to (placeholder) database
    Database db;
    db.executeInsert("INSERT INTO files (fileID, fileName, userID, totalSize) VALUES ('" + std::to_string(id) + "','" + fileName + "','" + std::to_string(userId) + "','" + std::to_string(totalSize) + "')");

    return id;
}

void FileStructureBuilder::LinkChunksToFile(int fileID, const std::vector<std::string>& listChunkHashes)
{
    std::lock_guard<std::mutex> g(g_fs_mutex);
    g_fileLayout[fileID] = listChunkHashes;
    // update meta too
    auto it = g_fileMeta.find(fileID);
    if (it != g_fileMeta.end())
        it->second.chunkHashes = listChunkHashes;

    Database db;
    // Insert sequence entries into intermediate table (sequence starts at 0)
    for (size_t i = 0; i < listChunkHashes.size(); ++i)
    {
        const auto& h = listChunkHashes[i];
        db.executeInsert("INSERT INTO file_chunks (fileID, sequence, chunkHash) VALUES ('" + std::to_string(fileID) + "','" + std::to_string(i) + "','" + h + "')");
    }
}

std::vector<std::string> FileStructureBuilder::GetFileLayout(int fileID)
{
    std::lock_guard<std::mutex> g(g_fs_mutex);
    auto it = g_fileLayout.find(fileID);
    if (it == g_fileLayout.end()) return {};
    return it->second;
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