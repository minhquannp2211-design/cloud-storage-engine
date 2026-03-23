#include "chunk_index.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace layer1 {
namespace {

bool parse_index_line(const std::string& line, ChunkMeta& meta) {
    std::stringstream ss(line);

    std::getline(ss, meta.chunk_id, '|');
    std::getline(ss, meta.relative_path, '|');

    std::string size_str;
    std::string ref_str;
    std::getline(ss, size_str, '|');
    std::getline(ss, ref_str, '|');

    if (meta.chunk_id.empty() || meta.relative_path.empty() ||
        size_str.empty() || ref_str.empty()) {
        return false;
    }

    try {
        meta.size = std::stoull(size_str);
        meta.ref_count = std::stoull(ref_str);
    } catch (...) {
        return false;
    }

    return true;
}

std::vector<ChunkMeta> ordered_entries(const std::unordered_map<ChunkID, ChunkMeta>& table) {
    std::vector<ChunkMeta> entries;
    entries.reserve(table.size());

    for (const auto& [chunk_id, meta] : table) {
        (void)chunk_id;
        entries.push_back(meta);
    }

    std::sort(entries.begin(), entries.end(),
              [](const ChunkMeta& lhs, const ChunkMeta& rhs) {
                  return lhs.chunk_id < rhs.chunk_id;
              });

    return entries;
}

bool write_snapshot_file(const fs::path& path,
                         const std::unordered_map<ChunkID, ChunkMeta>& table) {
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) {
        return false;
    }

    const auto entries = ordered_entries(table);
    for (const auto& meta : entries) {
        ofs << meta.chunk_id << "|"
            << meta.relative_path << "|"
            << meta.size << "|"
            << meta.ref_count << "\n";
    }

    ofs.flush();
    return ofs.good();
}

} // namespace

bool ChunkIndex::contains(const ChunkID& chunk_id) const {
    return table_.find(chunk_id) != table_.end();
}

std::optional<ChunkMeta> ChunkIndex::get(const ChunkID& chunk_id) const {
    const auto it = table_.find(chunk_id);
    if (it == table_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ChunkIndex::upsert(const ChunkMeta& meta) {
    table_[meta.chunk_id] = meta;
}

bool ChunkIndex::increment_ref(const ChunkID& chunk_id) {
    const auto it = table_.find(chunk_id);
    if (it == table_.end()) {
        return false;
    }
    ++it->second.ref_count;
    return true;
}

bool ChunkIndex::decrement_ref(const ChunkID& chunk_id) {
    const auto it = table_.find(chunk_id);
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

    fs::path primary(index_file);
    fs::path backup = primary;
    backup += ".bak";

    fs::path load_path;
    if (fs::exists(primary)) {
        load_path = primary;
    } else if (fs::exists(backup)) {
        load_path = backup;
    } else {
        return true; // fresh store; not an error
    }

    std::ifstream ifs(load_path);
    if (!ifs) {
        return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        ChunkMeta meta;
        if (!parse_index_line(line, meta)) {
            continue;
        }

        table_[meta.chunk_id] = meta;
    }

    return true;
}

bool ChunkIndex::save_to_file(const std::string& index_file) const {
    fs::path target(index_file);
    fs::path temp = target;
    temp += ".tmp";
    fs::path backup = target;
    backup += ".bak";

    std::error_code ec;
    if (!target.parent_path().empty()) {
        fs::create_directories(target.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    if (!write_snapshot_file(temp, table_)) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }

    fs::remove(backup, ec);
    ec.clear();

    if (fs::exists(target)) {
        fs::rename(target, backup, ec);
        if (ec) {
            std::error_code ignored;
            fs::remove(temp, ignored);
            return false;
        }
    }

    fs::rename(temp, target, ec);
    if (ec) {
        if (fs::exists(backup)) {
            std::error_code ignored;
            fs::rename(backup, target, ignored);
        }
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }

    fs::remove(backup, ec);
    return true;
}

} // namespace layer1
