#pragma once

#include "bloom_filter.hpp"
#include "chunk_index.hpp"
#include "chunker.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace layer1 {

struct StoreChunkResult {
    ChunkID chunk_id;
    std::uint64_t size = 0;
    bool inserted_new = false;
    std::uint64_t ref_count = 0;
};

struct FileChunkManifest {
    std::string relative_path;
    std::uint64_t file_size = 0;
    std::vector<ChunkID> ordered_chunk_ids;
};

struct FolderIngestResult {
    std::size_t total_files = 0;
    std::size_t total_chunks = 0;
    std::size_t unique_new_chunks = 0;
    std::size_t duplicate_chunks = 0;
    std::vector<FileChunkManifest> files;
};

class ChunkStore {
public:
    explicit ChunkStore(const std::string& base_dir, std::size_t fixed_chunk_size);

    bool init();

    StoreChunkResult store_chunk(const ByteBuffer& chunk);
    std::vector<StoreChunkResult> store_file(const std::string& file_path);

    FileChunkManifest ingest_file_manifest(const std::string& file_path,
                                           const std::string& relative_path);
    FolderIngestResult ingest_folder(const std::string& folder_path);

    bool export_manifest_text(const FolderIngestResult& result,
                              const std::string& out_file) const;

    std::optional<ByteBuffer> read_chunk(const ChunkID& chunk_id) const;
    bool dec_ref(const ChunkID& chunk_id);
    std::size_t gc();

    std::vector<ByteBuffer> chunk_file_only(const std::string& file_path) const;

private:
    bool rebuild_bloom_from_index();
    ChunkID hash_chunk(const ByteBuffer& chunk) const;

    std::string make_relative_chunk_path(const ChunkID& chunk_id) const;
    std::string make_absolute_chunk_path(const ChunkID& chunk_id) const;

    bool write_chunk_file(const ChunkID& chunk_id, const ByteBuffer& chunk) const;
    std::optional<ByteBuffer> read_chunk_file_raw(const ChunkID& chunk_id) const;
    bool verify_chunk_integrity(const ChunkID& chunk_id, const ByteBuffer& chunk) const;

    void mark_dirty();
    bool maybe_flush();
    bool persist_if_dirty();
    bool flush_index();

    std::string base_dir_;
    std::string index_file_;
    Chunker chunker_;
    BloomFilter bloom_;
    ChunkIndex index_;

    std::size_t dirty_ops_ = 0;
    std::size_t flush_threshold_ = 256;
};

} // namespace layer1
