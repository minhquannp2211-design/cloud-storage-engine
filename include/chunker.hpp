#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace layer1 {

using ByteBuffer = std::vector<std::uint8_t>;

class Chunker {
public:
    explicit Chunker(std::size_t chunk_size);

    std::vector<ByteBuffer> chunk_bytes(const ByteBuffer& data) const;
    std::vector<ByteBuffer> chunk_file(const std::string& file_path) const;
    std::vector<ByteBuffer> chunk_file(const std::filesystem::path& file_path) const;

    std::size_t chunk_size() const;

private:
    std::size_t chunk_size_;
};

} // namespace layer1
