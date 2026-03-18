#pragma once

#include "types.hpp"
#include <cstddef>
#include <string>
#include <vector>
#include <filesystem>

namespace layer1 {

    class Chunker {
    public:
        explicit Chunker(std::size_t chunk_size = 4096);

        std::vector<ByteBuffer> chunk_bytes(const ByteBuffer& data) const;
        std::vector<ByteBuffer> chunk_file(const std::string& file_path) const;
        std::vector<ByteBuffer> chunk_file(const std::filesystem::path& file_path) const;

    private:
        std::size_t chunk_size_;
    };

} // namespace layer1
