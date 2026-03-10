#pragma once

#include "bloom_filter.hpp"
#include "chunk_index.hpp"
#include "chunker.hpp"
#include "types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace layer1 {

struct StoreChunkResult {
    ChunkID chunk_id;
    bool inserted_new = false;
    std::uint64_t ref_count = 0;
    std::uint64_t size = 0;
};

struct FileChunkManifest {
    std::string relative_path;
    std::uint64_t file_size = 0;
    std::vector<ChunkID> ordered_chunk_ids;
};

struct FolderIngestResult {
    std::vector<FileChunkManifest> files;
    std::uint64_t total_files = 0;
    std::uint64_t total_chunks = 0;
    std::uint64_t unique_new_chunks = 0;
    std::uint64_t duplicate_chunks = 0;
};

class ChunkStore {
public:
    explicit ChunkStore(const std::string& base_dir, std::size_t fixed_chunk_size = 4096);

    bool init();

    StoreChunkResult store_chunk(const ByteBuffer& chunk);
    std::vector<StoreChunkResult> store_file(const std::string& file_path);

    struct FileChunkManifest {
        std::string relative_path;
        std::uint64_t file_size = 0;
        std::vector<ChunkID> ordered_chunk_ids;
    };

    struct FolderIngestResult {
        std::vector<FileChunkManifest> files;
        std::uint64_t total_files = 0;
        std::uint64_t total_chunks = 0;
        std::uint64_t unique_new_chunks = 0;
        std::uint64_t duplicate_chunks = 0;
    };

    std::optional<ByteBuffer> read_chunk(const ChunkID& chunk_id) const;

    bool dec_ref(const ChunkID& chunk_id);
    std::size_t gc();

    std::vector<ByteBuffer> chunk_file_only(const std::string& file_path) const;

    bool flush_index() const;

private:
    ChunkID hash_chunk(const ByteBuffer& chunk) const;
    std::string make_relative_chunk_path(const ChunkID& chunk_id) const;
    std::string make_absolute_chunk_path(const ChunkID& chunk_id) const;

    bool write_chunk_file(const ChunkID& chunk_id, const ByteBuffer& chunk) const;
    std::optional<ByteBuffer> read_chunk_file_raw(const ChunkID& chunk_id) const;
    bool verify_chunk_integrity(const ChunkID& chunk_id, const ByteBuffer& chunk) const;
    bool rebuild_bloom_from_index();

private:
    std::string base_dir_;
    std::string index_file_;
    Chunker chunker_;
    BloomFilter bloom_;
    ChunkIndex index_;
};

} // namespace layer1