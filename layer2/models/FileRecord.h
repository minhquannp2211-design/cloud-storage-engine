#pragma once
#include <string>
#include <vector>

struct FileRecord
{
    int fileID{0};
    int userID{0};
    std::string fileName;
    long totalSize{0};
    std::vector<std::string> chunkHashes;
};