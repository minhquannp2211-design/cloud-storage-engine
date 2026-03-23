#pragma once
#include <string>

struct ChunkRecord
{
    std::string hash;
    int referenceCount;
};