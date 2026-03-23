#include "chunker.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace layer1 {

Chunker::Chunker(std::size_t chunk_size) : chunk_size_(chunk_size) {}

std::vector<ByteBuffer> Chunker::chunk_bytes(const ByteBuffer& data) const {
    std::vector<ByteBuffer> chunks;
    if (chunk_size_ == 0) {
        return chunks;
    }

    chunks.reserve((data.size() + chunk_size_ - 1) / chunk_size_);

    for (std::size_t i = 0; i < data.size(); i += chunk_size_) {
        const std::size_t end = std::min(i + chunk_size_, data.size());
        chunks.emplace_back(data.begin() + static_cast<std::ptrdiff_t>(i),
                            data.begin() + static_cast<std::ptrdiff_t>(end));
    }
    return chunks;
}

std::vector<ByteBuffer> Chunker::chunk_file(const std::string& file_path) const {
    return chunk_file(fs::path(file_path));
}

std::vector<ByteBuffer> Chunker::chunk_file(const fs::path& file_path) const {
    std::vector<ByteBuffer> chunks;
    if (chunk_size_ == 0) {
        return chunks;
    }

    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Cannot open file for chunking: " + file_path.string());
    }

    std::error_code ec;
    const auto file_size = fs::file_size(file_path, ec);
    if (!ec && file_size > 0) {
        chunks.reserve(static_cast<std::size_t>((file_size + chunk_size_ - 1) / chunk_size_));
    }

    ByteBuffer buffer(chunk_size_);
    while (ifs) {
        ifs.read(reinterpret_cast<char*>(buffer.data()),
                 static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytes_read = ifs.gcount();
        if (bytes_read <= 0) {
            break;
        }

        chunks.emplace_back(buffer.begin(),
                            buffer.begin() + static_cast<std::ptrdiff_t>(bytes_read));
    }

    return chunks;
}

std::size_t Chunker::chunk_size() const {
    return chunk_size_;
}

} // namespace layer1
