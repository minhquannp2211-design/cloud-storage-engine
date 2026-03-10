#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace layer1 {

using ByteBuffer = std::vector<std::uint8_t>;
using ChunkID = std::string;

struct ChunkMeta {
    ChunkID chunk_id;
    std::string relative_path;
    std::uint64_t size = 0;
    std::uint64_t ref_count = 0;
};

} // namespace layer1