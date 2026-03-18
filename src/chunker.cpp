#include "chunker.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace fs = std::filesystem;

namespace layer1 {

    Chunker::Chunker(std::size_t chunk_size) : chunk_size_(chunk_size) {}

    std::vector<ByteBuffer> Chunker::chunk_bytes(const ByteBuffer& data) const {
        std::vector<ByteBuffer> chunks;
        if (chunk_size_ == 0) {
            return chunks;
        }

        for (std::size_t i = 0; i < data.size(); i += chunk_size_) {
            std::size_t end = std::min(i + chunk_size_, data.size());
            chunks.emplace_back(data.begin() + static_cast<std::ptrdiff_t>(i),
                                data.begin() + static_cast<std::ptrdiff_t>(end));
        }
        return chunks;
    }

    std::vector<ByteBuffer> Chunker::chunk_file(const std::string& file_path) const {
        return chunk_file(fs::path(file_path));
    }

    std::vector<ByteBuffer> Chunker::chunk_file(const fs::path& file_path) const {
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs) {
            throw std::runtime_error("Cannot open file for chunking: " + file_path.string());
        }

        ByteBuffer data((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());

        return chunk_bytes(data);
    }

} // namespace layer1
