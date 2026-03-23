#pragma once
#include <string>
#include <mutex>

class IndexManager
{
public:
    bool CheckChunkExists(const std::string& hash);

    void RegisterNewChunk(const std::string& hash);

    void IncrementReference(const std::string& hash);

    void DecrementReference(const std::string& hash);
private:
    static std::mutex chunkRefMutex;
};