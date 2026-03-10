#include "chunk_index.hpp"

#include <fstream>
#include <sstream>

namespace layer1 {

bool ChunkIndex::contains(const ChunkID& chunk_id) const {
    return table_.find(chunk_id) != table_.end();
}

std::optional<ChunkMeta> ChunkIndex::get(const ChunkID& chunk_id) const {
    auto it = table_.find(chunk_id);
    if (it == table_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ChunkIndex::upsert(const ChunkMeta& meta) {
    table_[meta.chunk_id] = meta;
}

bool ChunkIndex::increment_ref(const ChunkID& chunk_id) {
    auto it = table_.find(chunk_id);
    if (it == table_.end()) {
        return false;
    }
    ++it->second.ref_count;
    return true;
}

bool ChunkIndex::decrement_ref(const ChunkID& chunk_id) {
    auto it = table_.find(chunk_id);
    if (it == table_.end()) {
        return false;
    }
    if (it->second.ref_count == 0) {
        return false;
    }
    --it->second.ref_count;
    return true;
}

bool ChunkIndex::erase(const ChunkID& chunk_id) {
    return table_.erase(chunk_id) > 0;
}

const std::unordered_map<ChunkID, ChunkMeta>& ChunkIndex::all() const {
    return table_;
}

bool ChunkIndex::load_from_file(const std::string& index_file) {
    table_.clear();

    std::ifstream ifs(index_file);
    if (!ifs) {
        return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        ChunkMeta meta;

        std::getline(ss, meta.chunk_id, '|');
        std::getline(ss, meta.relative_path, '|');

        std::string size_str;
        std::string ref_str;
        std::getline(ss, size_str, '|');
        std::getline(ss, ref_str, '|');

        if (meta.chunk_id.empty() || meta.relative_path.empty() ||
            size_str.empty() || ref_str.empty()) {
            continue;
        }

        meta.size = std::stoull(size_str);
        meta.ref_count = std::stoull(ref_str);

        table_[meta.chunk_id] = meta;
    }

    return true;
}

bool ChunkIndex::save_to_file(const std::string& index_file) const {
    std::ofstream ofs(index_file, std::ios::trunc);
    if (!ofs) {
        return false;
    }

    for (const auto& [chunk_id, meta] : table_) {
        ofs << meta.chunk_id << "|"
            << meta.relative_path << "|"
            << meta.size << "|"
            << meta.ref_count << "\n";
    }

    return true;
}

} // namespace layer1