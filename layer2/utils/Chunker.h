#pragma once
#include <vector>
#include <string>
#include "../models/Chunk.h"

class Chunker {
public:
    static std::vector<Chunk> Split(const std::string& filePath);
};