#pragma once
#include <vector>
#include <string>
#include "../models/FileRecord.h"

class FileStructureBuilder
{
public:
    // Initialize từ database: rebuild g_nextFileID, g_fileMeta, g_fileLayout
    static void Initialize();
    
    // Create a new file entry với status UPLOADING
    int CreateFileEntry(const std::string& fileName, int userId, long totalSize, const std::string& path = "");

    // Link ordered list of chunk hashes to a file
    void LinkChunksToFile(int fileID, const std::vector<std::string>& listChunkHashes);

    // Return ordered list of chunk hashes for fileID
    std::vector<std::string> GetFileLayout(int fileID);

    // Get file metadata
    FileRecord GetFileMetadata(int fileID);
    
    // Update file status
    void UpdateFileStatus(int fileID, FileStatus newStatus);

    // Helper to build FileRecord
    FileRecord BuildFileRecord(
        const std::string& fileName,
        const std::vector<std::string>& chunks);
};
