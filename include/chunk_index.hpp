#pragma once

#include "types.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace layer1 {

    class ChunkIndex {
    public:
        bool contains(const ChunkID& chunk_id) const;
        std::optional<ChunkMeta> get(const ChunkID& chunk_id) const;
        void upsert(const ChunkMeta& meta);
        bool increment_ref(const ChunkID& chunk_id);
        bool decrement_ref(const ChunkID& chunk_id);
        bool erase(const ChunkID& chunk_id);

        const std::unordered_map<ChunkID, ChunkMeta>& all() const;

        bool load_from_file(const std::string& index_file);
        bool save_to_file(const std::string& index_file) const;

    private:
        std::unordered_map<ChunkID, ChunkMeta> table_;
    };

} // namespace layer1
