#pragma once
#include <vector>
#include <string>
#include "../models/FileRecord.h"

class FileStructureBuilder
{
public:
    // Create a new file entry in the (logical) database and return a generated FileID.
    int CreateFileEntry(const std::string& fileName, int userId, long totalSize);

    // Link an ordered list of chunk hashes to a file (maintains sequence order).
    void LinkChunksToFile(int fileID, const std::vector<std::string>& listChunkHashes);

    // Return the ordered list of chunk hashes for the given fileID.
    std::vector<std::string> GetFileLayout(int fileID);

    // Helper to build an in-memory FileRecord
    FileRecord BuildFileRecord(
        const std::string& fileName,
        const std::vector<std::string>& chunks);
};