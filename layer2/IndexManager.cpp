#include "IndexManager.h"
#include <iostream>
#include <unordered_map>

static std::unordered_map<std::string, int> chunkRefTable;
std::mutex IndexManager::chunkRefMutex;

bool IndexManager::CheckChunkExists(const std::string& hash)
{
    std::lock_guard<std::mutex> g(chunkRefMutex);
    return chunkRefTable.find(hash) != chunkRefTable.end();
}

void IndexManager::RegisterNewChunk(const std::string& hash)
{
    std::lock_guard<std::mutex> g(chunkRefMutex);
    chunkRefTable[hash] = 1;
}

void IndexManager::IncrementReference(const std::string& hash)
{
    std::lock_guard<std::mutex> g(chunkRefMutex);
    auto it = chunkRefTable.find(hash);
    if (it != chunkRefTable.end())
        it->second++;
}

void IndexManager::DecrementReference(const std::string& hash)
{
    std::lock_guard<std::mutex> g(chunkRefMutex);
    auto it = chunkRefTable.find(hash);
    if (it != chunkRefTable.end())
    {
        it->second--;

        if (it->second <= 0)
        {
            chunkRefTable.erase(it);
            std::cout << "Chunk removed: " << hash << std::endl;
        }
    }
}