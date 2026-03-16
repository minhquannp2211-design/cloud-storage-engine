#include "Chunker.h"
#include "../config/Config.h"
#include "Hash.h"
#include <fstream>

std::vector<Chunk> Chunker::Split(const std::string& filePath){

    std::ifstream file(filePath, std::ios::binary);

    std::vector<Chunk> chunks;

    if (!file.is_open())
        return chunks;

    std::string buffer;
    buffer.resize(CHUNK_SIZE);

    while (file)
    {
        file.read(&buffer[0], CHUNK_SIZE);
        std::streamsize s = file.gcount();
        if (s <= 0) break;
        Chunk c;
        c.data.assign(buffer.data(), static_cast<size_t>(s));
        c.hash = Hash::SHA256(c.data);
        chunks.push_back(std::move(c));
    }

    return chunks;
}