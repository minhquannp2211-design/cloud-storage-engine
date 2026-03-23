#include "chunk_store.hpp"
#include "picosha2.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace layer1 {

ChunkStore::ChunkStore(const std::string& base_dir, std::size_t fixed_chunk_size)
    : base_dir_(base_dir),
      index_file_(base_dir + "/index.txt"),
      chunker_(fixed_chunk_size),
      bloom_(1u << 20, 3) {}

bool ChunkStore::init() {
    try {
        fs::create_directories(base_dir_);
        if (!index_.load_from_file(index_file_)) {
            return false;
        }
        dirty_ops_ = 0;
        return rebuild_bloom_from_index();
    } catch (...) {
        return false;
    }
}

bool ChunkStore::rebuild_bloom_from_index() {
    bloom_.clear();
    for (const auto& [chunk_id, meta] : index_.all()) {
        (void)meta;
        bloom_.add(chunk_id);
    }
    return true;
}

ChunkID ChunkStore::hash_chunk(const ByteBuffer& chunk) const {
    return picosha2::hash256_hex_string(chunk.begin(), chunk.end());
}

std::string ChunkStore::make_relative_chunk_path(const ChunkID& chunk_id) const {
    const std::string a = chunk_id.size() >= 2 ? chunk_id.substr(0, 2) : "00";
    const std::string b = chunk_id.size() >= 4 ? chunk_id.substr(2, 2) : "00";
    return a + "/" + b + "/" + chunk_id + ".bin";
}

std::string ChunkStore::make_absolute_chunk_path(const ChunkID& chunk_id) const {
    return (fs::path(base_dir_) / make_relative_chunk_path(chunk_id)).string();
}

bool ChunkStore::write_chunk_file(const ChunkID& chunk_id, const ByteBuffer& chunk) const {
    const fs::path full_path = fs::path(make_absolute_chunk_path(chunk_id));
    const fs::path temp_path = full_path.string() + ".tmp";

    try {
        fs::create_directories(full_path.parent_path());

        if (fs::exists(full_path)) {
            return true;
        }

        std::ofstream ofs(temp_path, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            return false;
        }

        if (!chunk.empty()) {
            ofs.write(reinterpret_cast<const char*>(chunk.data()),
                      static_cast<std::streamsize>(chunk.size()));
        }
        ofs.flush();
        ofs.close();

        if (!ofs.good()) {
            std::error_code ignored;
            fs::remove(temp_path, ignored);
            return false;
        }

        if (fs::exists(full_path)) {
            std::error_code ignored;
            fs::remove(temp_path, ignored);
            return true;
        }

        std::error_code ec;
        fs::rename(temp_path, full_path, ec);
        if (ec) {
            std::error_code ignored;
            fs::remove(temp_path, ignored);
            return false;
        }

        return true;
    } catch (...) {
        std::error_code ignored;
        fs::remove(temp_path, ignored);
        return false;
    }
}

std::optional<ByteBuffer> ChunkStore::read_chunk_file_raw(const ChunkID& chunk_id) const {
    const fs::path full_path = fs::path(make_absolute_chunk_path(chunk_id));
    std::ifstream ifs(full_path, std::ios::binary);
    if (!ifs) {
        return std::nullopt;
    }

    std::error_code ec;
    const auto file_size = fs::file_size(full_path, ec);
    if (ec) {
        return std::nullopt;
    }

    ByteBuffer data(static_cast<std::size_t>(file_size));
    if (file_size > 0) {
        ifs.read(reinterpret_cast<char*>(data.data()),
                 static_cast<std::streamsize>(data.size()));
        if (!ifs) {
            return std::nullopt;
        }
    }

    return data;
}

bool ChunkStore::verify_chunk_integrity(const ChunkID& chunk_id, const ByteBuffer& chunk) const {
    return hash_chunk(chunk) == chunk_id;
}

void ChunkStore::mark_dirty() {
    ++dirty_ops_;
}

bool ChunkStore::maybe_flush() {
    if (dirty_ops_ >= flush_threshold_) {
        return flush_index();
    }
    return true;
}

bool ChunkStore::persist_if_dirty() {
    if (dirty_ops_ == 0) {
        return true;
    }
    return flush_index();
}

bool ChunkStore::flush_index() {
    if (!index_.save_to_file(index_file_)) {
        return false;
    }
    dirty_ops_ = 0;
    return true;
}

StoreChunkResult ChunkStore::store_chunk(const ByteBuffer& chunk) {
    StoreChunkResult result{};
    result.size = static_cast<std::uint64_t>(chunk.size());

    const ChunkID chunk_id = hash_chunk(chunk);
    result.chunk_id = chunk_id;

    const bool bloom_hit = bloom_.possibly_contains(chunk_id);
    if (bloom_hit && index_.contains(chunk_id)) {
        index_.increment_ref(chunk_id);
        const auto meta = index_.get(chunk_id);

        result.inserted_new = false;
        result.ref_count = meta ? meta->ref_count : 0;

        mark_dirty();
        if (!maybe_flush()) {
            throw std::runtime_error("Failed to persist duplicate chunk ref-count: " + chunk_id);
        }
        return result;
    }

    if (!write_chunk_file(chunk_id, chunk)) {
        throw std::runtime_error("Failed to write chunk file: " + chunk_id);
    }

    ChunkMeta meta;
    meta.chunk_id = chunk_id;
    meta.relative_path = make_relative_chunk_path(chunk_id);
    meta.size = static_cast<std::uint64_t>(chunk.size());
    meta.ref_count = 1;

    index_.upsert(meta);
    bloom_.add(chunk_id);

    result.inserted_new = true;
    result.ref_count = 1;

    mark_dirty();
    if (!maybe_flush()) {
        throw std::runtime_error("Failed to persist chunk index entry: " + chunk_id);
    }
    return result;
}

std::vector<StoreChunkResult> ChunkStore::store_file(const std::string& file_path) {
    const auto chunks = chunker_.chunk_file(file_path);

    std::vector<StoreChunkResult> results;
    results.reserve(chunks.size());

    for (const auto& chunk : chunks) {
        results.push_back(store_chunk(chunk));
    }

    if (!persist_if_dirty()) {
        throw std::runtime_error("Failed to flush index after storing file: " + file_path);
    }

    return results;
}

FileChunkManifest ChunkStore::ingest_file_manifest(const std::string& file_path,
                                                   const std::string& relative_path) {
    FileChunkManifest manifest;
    manifest.relative_path = relative_path;

    const auto chunks = chunker_.chunk_file(fs::path(file_path));

    std::error_code ec;
    const auto file_sz = fs::file_size(file_path, ec);
    manifest.file_size = ec ? 0 : static_cast<std::uint64_t>(file_sz);

    manifest.ordered_chunk_ids.reserve(chunks.size());

    for (const auto& chunk : chunks) {
        const auto result = store_chunk(chunk);
        manifest.ordered_chunk_ids.push_back(result.chunk_id);
    }

    if (!persist_if_dirty()) {
        throw std::runtime_error("Failed to flush index after ingest_file_manifest: " + file_path);
    }

    return manifest;
}

FolderIngestResult ChunkStore::ingest_folder(const std::string& folder_path) {
    FolderIngestResult result;

    const fs::path root(folder_path);
    if (!fs::exists(root) || !fs::is_directory(root)) {
        throw std::runtime_error("Folder does not exist or is not a directory: " + folder_path);
    }

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const fs::path abs_path = entry.path();
        const fs::path rel_path = fs::relative(abs_path, root);

        FileChunkManifest manifest;
        manifest.relative_path = rel_path.generic_string();

        std::error_code ec;
        const auto file_sz = fs::file_size(abs_path, ec);
        manifest.file_size = ec ? 0 : static_cast<std::uint64_t>(file_sz);

        const auto chunks = chunker_.chunk_file(abs_path);
        manifest.ordered_chunk_ids.reserve(chunks.size());

        for (const auto& chunk : chunks) {
            const auto store_res = store_chunk(chunk);
            manifest.ordered_chunk_ids.push_back(store_res.chunk_id);

            ++result.total_chunks;
            if (store_res.inserted_new) {
                ++result.unique_new_chunks;
            } else {
                ++result.duplicate_chunks;
            }
        }

        if (!persist_if_dirty()) {
            throw std::runtime_error("Failed to flush index after file: " + abs_path.string());
        }

        result.files.push_back(std::move(manifest));
        ++result.total_files;
    }

    return result;
}

bool ChunkStore::export_manifest_text(const FolderIngestResult& result,
                                      const std::string& out_file) const {
    std::ofstream ofs(out_file, std::ios::trunc);
    if (!ofs) {
        return false;
    }

    ofs << "total_files=" << result.total_files << "\n";
    ofs << "total_chunks=" << result.total_chunks << "\n";
    ofs << "unique_new_chunks=" << result.unique_new_chunks << "\n";
    ofs << "duplicate_chunks=" << result.duplicate_chunks << "\n";
    ofs << "\n";

    for (const auto& file : result.files) {
        ofs << "FILE " << file.relative_path << "\n";
        ofs << "SIZE " << file.file_size << "\n";
        ofs << "CHUNKS " << file.ordered_chunk_ids.size() << "\n";

        for (const auto& chunk_id : file.ordered_chunk_ids) {
            ofs << chunk_id << "\n";
        }
        ofs << "\n";
    }

    return true;
}

std::optional<ByteBuffer> ChunkStore::read_chunk(const ChunkID& chunk_id) const {
    const auto meta = index_.get(chunk_id);
    if (!meta) {
        return std::nullopt;
    }

    const auto data = read_chunk_file_raw(chunk_id);
    if (!data) {
        return std::nullopt;
    }

    if (data->size() != meta->size) {
        return std::nullopt;
    }

    if (!verify_chunk_integrity(chunk_id, *data)) {
        return std::nullopt;
    }

    return data;
}

bool ChunkStore::dec_ref(const ChunkID& chunk_id) {
    if (!index_.decrement_ref(chunk_id)) {
        return false;
    }

    mark_dirty();
    return maybe_flush();
}

std::size_t ChunkStore::gc() {
    std::vector<ChunkID> to_delete;
    to_delete.reserve(index_.all().size());

    for (const auto& [chunk_id, meta] : index_.all()) {
        if (meta.ref_count == 0) {
            to_delete.push_back(chunk_id);
        }
    }

    std::size_t deleted_count = 0;
    for (const auto& chunk_id : to_delete) {
        const fs::path full_path = fs::path(make_absolute_chunk_path(chunk_id));

        std::error_code ec;
        const bool removed = fs::remove(full_path, ec);
        if (ec) {
            continue; // do not erase index if file-system removal failed
        }

        if (removed || !fs::exists(full_path, ec)) {
            if (index_.erase(chunk_id)) {
                ++deleted_count;
                mark_dirty();
            }
        }
    }

    if (!persist_if_dirty()) {
        throw std::runtime_error("Failed to flush index after GC");
    }

    rebuild_bloom_from_index();
    return deleted_count;
}

std::vector<ByteBuffer> ChunkStore::chunk_file_only(const std::string& file_path) const {
    return chunker_.chunk_file(file_path);
}

} // namespace layer1
