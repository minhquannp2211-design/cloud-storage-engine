
#include "chunk_store.hpp"
#include "bloom_filter.hpp"
#include "picosha2.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;
using namespace layer1;

struct TestRunner {
    int passed = 0;
    int failed = 0;
    void expect(bool condition, const std::string& message) {
        if (condition) {
            ++passed;
            std::cout << "[PASS] " << message << "\n";
        } else {
            ++failed;
            std::cout << "[FAIL] " << message << "\n";
        }
    }
    void note(const std::string& message) const {
        std::cout << "       " << message << "\n";
    }

    int exit_code() const {
        return failed == 0 ? 0 : 1;
    }
};

struct DatasetFileInfo {
    fs::path absolute_path;
    std::string relative_path;
    std::uint64_t size = 0;
    bool is_empty = false;
    bool has_non_ascii_path = false;
};

struct DatasetScanSummary {
    std::vector<DatasetFileInfo> files;
    std::uint64_t total_bytes = 0;
    std::size_t empty_files = 0;
    std::size_t unicode_like_paths = 0;
    std::size_t duplicate_basename_groups = 0;
    std::size_t duplicate_basename_files = 0;
};

struct DatasetFixtures {
    std::vector<DatasetFileInfo> exact_duplicate_group;
    bool has_empty_file = false;
    DatasetFileInfo empty_file;
    bool has_unicode_file = false;
    DatasetFileInfo unicode_file;
    bool has_same_basename_pair = false;
    DatasetFileInfo same_name_a;
    DatasetFileInfo same_name_b;
    bool has_recovery_file = false;
    DatasetFileInfo recovery_file;
};

static std::string to_utf8_generic(const fs::path& p) {
    return p.generic_u8string();
}

static bool contains_non_ascii(const std::string& s) {
    for (unsigned char c : s) {
        if (c >= 128) {
            return true;
        }
    }
    return false;
}

static std::string format_bytes(std::uint64_t bytes) {
    static const char* suffixes[] = {"B", "KiB", "MiB", "GiB"};
    double value = static_cast<double>(bytes);
    std::size_t idx = 0;
    while (value >= 1024.0 && idx + 1 < (sizeof(suffixes) / sizeof(suffixes[0]))) {
        value /= 1024.0;
        ++idx;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(idx == 0 ? 0 : 2) << value << ' ' << suffixes[idx];
    return oss.str();
}

static ByteBuffer read_file_bytes(const fs::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Cannot open file: " + to_utf8_generic(path));
    }

    return ByteBuffer((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
}

static std::string hash_bytes_sha256(const ByteBuffer& bytes) {
    std::string data_str(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return picosha2::hash256_hex_string(data_str);
}

static std::string hash_file_whole(const fs::path& path) {
    return hash_bytes_sha256(read_file_bytes(path));
}

static ByteBuffer reconstruct_file(ChunkStore& store, const FileChunkManifest& manifest) {
    ByteBuffer out;
    for (const auto& chunk_id : manifest.ordered_chunk_ids) {
        auto part = store.read_chunk(chunk_id);
        if (!part.has_value()) {
            throw std::runtime_error("Missing chunk while reconstructing: " + chunk_id);
        }
        out.insert(out.end(), part->begin(), part->end());
    }
    return out;
}

static DatasetScanSummary scan_dataset(const fs::path& root) {
    DatasetScanSummary summary;
    std::map<std::string, std::size_t> basename_freq;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        DatasetFileInfo info;
        info.absolute_path = entry.path();
        info.relative_path = to_utf8_generic(fs::relative(info.absolute_path, root));

        std::error_code ec;
        const auto file_size = fs::file_size(info.absolute_path, ec);
        info.size = ec ? 0 : static_cast<std::uint64_t>(file_size);
        info.is_empty = (info.size == 0);
        info.has_non_ascii_path = contains_non_ascii(info.relative_path);

        summary.total_bytes += info.size;
        summary.empty_files += info.is_empty ? 1u : 0u;
        summary.unicode_like_paths += info.has_non_ascii_path ? 1u : 0u;
        basename_freq[info.absolute_path.filename().u8string()]++;

        summary.files.push_back(std::move(info));
    }

    std::sort(summary.files.begin(), summary.files.end(),
              [](const DatasetFileInfo& a, const DatasetFileInfo& b) {
                  return a.relative_path < b.relative_path;
              });

    for (const auto& [name, count] : basename_freq) {
        if (count > 1) {
            ++summary.duplicate_basename_groups;
            summary.duplicate_basename_files += count;
        }
    }

    return summary;
}

static DatasetFixtures select_fixtures(const DatasetScanSummary& scan) {
    DatasetFixtures fx;
    std::map<std::string, std::vector<DatasetFileInfo>> basename_groups;
    std::map<std::string, std::vector<DatasetFileInfo>> content_groups;

    for (const auto& info : scan.files) {
        if (!fx.has_recovery_file && !info.is_empty) {
            fx.has_recovery_file = true;
            fx.recovery_file = info;
        }
        if (!fx.has_empty_file && info.is_empty) {
            fx.has_empty_file = true;
            fx.empty_file = info;
        }
        if (!fx.has_unicode_file && info.has_non_ascii_path) {
            fx.has_unicode_file = true;
            fx.unicode_file = info;
        }

        basename_groups[info.absolute_path.filename().u8string()].push_back(info);

        if (!info.is_empty) {
            const std::string key = std::to_string(info.size) + ":" + hash_file_whole(info.absolute_path);
            content_groups[key].push_back(info);
        }
    }

    for (const auto& [name, group] : basename_groups) {
        (void)name;
        if (group.size() >= 2) {
            fx.has_same_basename_pair = true;
            fx.same_name_a = group[0];
            fx.same_name_b = group[1];
            break;
        }
    }

    for (const auto& [key, group] : content_groups) {
        (void)key;
        if (group.size() < 2) continue;
        if (group.size() > fx.exact_duplicate_group.size()) {
            fx.exact_duplicate_group = group;
        } else if (group.size() == fx.exact_duplicate_group.size() && !group.empty() &&
                   (fx.exact_duplicate_group.empty() || group.front().size > fx.exact_duplicate_group.front().size)) {
            fx.exact_duplicate_group = group;
        }
    }

    return fx;
}

static bool vector_eq(const std::vector<ChunkID>& a, const std::vector<ChunkID>& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

static fs::path detect_dataset_root() {
    std::vector<fs::path> suffixes = {
        fs::path("test_data/cloud_engine_dataset_v1"),
        fs::path("test_data/layer2_test_dataset/layer2_dataset"),
        fs::path("test_data")
    };

    fs::path cur = fs::current_path();
    for (int up = 0; up < 6; ++up) {
        for (const auto& suffix : suffixes) {
            fs::path candidate = cur / suffix;
            if (fs::exists(candidate) && fs::is_directory(candidate)) {
                return fs::weakly_canonical(candidate);
            }
        }
        if (!cur.has_parent_path()) {
            break;
        }
        cur = cur.parent_path();
    }

    throw std::runtime_error(
        "Cannot detect dataset root. Pass it explicitly:\n"
        "  ./app_demo <dataset_root> [storage_root] [chunk_size]");
}

static std::string to_printable(const fs::path& p) {
    return to_utf8_generic(p);
}

static void print_dataset_overview(const DatasetScanSummary& scan) {
    std::cout << "=== Dataset scan overview ===\n";
    std::cout << "Regular files : " << scan.files.size() << "\n";
    std::cout << "Total bytes   : " << scan.total_bytes << " (" << format_bytes(scan.total_bytes) << ")\n";
    std::cout << "Empty files   : " << scan.empty_files << "\n";
    std::cout << "Unicode paths : " << scan.unicode_like_paths << "\n";
    std::cout << "Same-name groups across folders : " << scan.duplicate_basename_groups
              << " groups / " << scan.duplicate_basename_files << " files\n";

    std::vector<DatasetFileInfo> by_size = scan.files;
    std::sort(by_size.begin(), by_size.end(),
              [](const DatasetFileInfo& a, const DatasetFileInfo& b) {
                  if (a.size != b.size) return a.size > b.size;
                  return a.relative_path < b.relative_path;
              });

    std::cout << "Largest files :\n";
    const std::size_t top_n = std::min<std::size_t>(5, by_size.size());
    for (std::size_t i = 0; i < top_n; ++i) {
        std::cout << "  - " << by_size[i].relative_path << " | "
                  << by_size[i].size << " bytes\n";
    }

    std::cout << "Sample files  :\n";
    for (std::size_t i = 0; i < std::min<std::size_t>(5, scan.files.size()); ++i) {
        std::cout << "  - " << scan.files[i].relative_path << "\n";
    }
    std::cout << "\n";
}

static void print_fixture_summary(const DatasetFixtures& fx) {
    std::cout << "=== Auto-selected fixtures ===\n";
    if (!fx.exact_duplicate_group.empty()) {
        std::cout << "Exact-duplicate group (" << fx.exact_duplicate_group.size() << " files, "
                  << fx.exact_duplicate_group.front().size << " bytes each):\n";
        for (std::size_t i = 0; i < std::min<std::size_t>(3, fx.exact_duplicate_group.size()); ++i) {
            std::cout << "  - " << fx.exact_duplicate_group[i].relative_path << "\n";
        }
    } else {
        std::cout << "Exact-duplicate group: not found\n";
    }

    std::cout << "Recovery file: "
              << (fx.has_recovery_file ? fx.recovery_file.relative_path : std::string("not found")) << "\n";
    std::cout << "Empty file   : "
              << (fx.has_empty_file ? fx.empty_file.relative_path : std::string("not found")) << "\n";
    std::cout << "Unicode file : "
              << (fx.has_unicode_file ? fx.unicode_file.relative_path : std::string("not found")) << "\n";

    if (fx.has_same_basename_pair) {
        std::cout << "Same-name pair:\n";
        std::cout << "  - " << fx.same_name_a.relative_path << "\n";
        std::cout << "  - " << fx.same_name_b.relative_path << "\n";
    } else {
        std::cout << "Same-name pair: not found\n";
    }
    std::cout << "\n";
}

static std::map<std::string, FileChunkManifest> build_manifest_map(const FolderIngestResult& result) {
    std::map<std::string, FileChunkManifest> out;
    for (const auto& manifest : result.files) {
        out.emplace(manifest.relative_path, manifest);
    }
    return out;
}

static bool read_text_file(const fs::path& path, std::string& out) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return false;
    }
    out.assign((std::istreambuf_iterator<char>(ifs)),
               std::istreambuf_iterator<char>());
    return true;
}

int main(int argc, char* argv[]) {
    TestRunner tr;

    try {
        fs::path dataset_root = (argc >= 2) ? fs::path(argv[1]) : detect_dataset_root();
        fs::path storage_root = (argc >= 3) ? fs::path(argv[2]) : fs::path("demo_storage");
        std::size_t chunk_size = (argc >= 4) ? static_cast<std::size_t>(std::stoull(argv[3])) : 4096;

        dataset_root = fs::weakly_canonical(dataset_root);

        std::cout << "=== Layer 1 demo runner ===\n";
        std::cout << "Dataset root : " << to_printable(dataset_root) << "\n";
        std::cout << "Storage root : " << to_printable(storage_root) << "\n";
        std::cout << "Chunk size   : " << chunk_size << " bytes\n\n";

        if (!fs::exists(dataset_root) || !fs::is_directory(dataset_root)) {
            throw std::runtime_error("Dataset root does not exist or is not a directory.");
        }

        DatasetScanSummary scan = scan_dataset(dataset_root);
        print_dataset_overview(scan);

        tr.expect(!scan.files.empty(), "filesystem scan finds at least one regular file in dataset");
        tr.expect(scan.total_bytes >= 0, "dataset byte scan completes");

        DatasetFixtures fx = select_fixtures(scan);
        print_fixture_summary(fx);

        tr.expect(fx.exact_duplicate_group.size() >= 2,
                  "dataset contains at least one exact-duplicate group (>= 2 files)");
        tr.expect(fx.has_empty_file, "dataset contains at least one empty file");
        tr.expect(fx.has_recovery_file, "dataset contains at least one non-empty file for recovery");

        if (scan.unicode_like_paths > 0) {
            tr.expect(fx.has_unicode_file, "dataset exposes at least one Unicode-like relative path");
        } else {
            tr.note("Dataset has no Unicode path; Unicode-specific checks will be skipped.");
        }

        if (scan.duplicate_basename_groups > 0) {
            tr.expect(fx.has_same_basename_pair,
                      "dataset exposes at least one same-name pair across folders");
        } else {
            tr.note("Dataset has no same-name pair; basename collision checks will be skipped.");
        }

        // ---------------------------------------------------------------------
        // CASE 1: Dedup / read / delete / GC
        // ---------------------------------------------------------------------
        if (fx.exact_duplicate_group.size() >= 2) {
            fs::path case_dir = storage_root / "case_dedup";
            fs::remove_all(case_dir);

            ChunkStore store(case_dir.string(), chunk_size);
            tr.expect(store.init(), "init() succeeds for dedup case");

            const auto& dup1 = fx.exact_duplicate_group[0];
            const auto& dup2 = fx.exact_duplicate_group[1];
            const bool has_third_dup = fx.exact_duplicate_group.size() >= 3;
            const DatasetFileInfo* dup3 = has_third_dup ? &fx.exact_duplicate_group[2] : nullptr;

            auto m1 = store.ingest_file_manifest(dup1.absolute_path.string(), dup1.relative_path);
            auto m2 = store.ingest_file_manifest(dup2.absolute_path.string(), dup2.relative_path);
            tr.expect(!m1.ordered_chunk_ids.empty(), "duplicate test files are chunked");
            tr.expect(vector_eq(m1.ordered_chunk_ids, m2.ordered_chunk_ids),
                      "same-content files produce identical ordered chunk IDs (file 1 vs file 2)");

            FileChunkManifest m3;
            if (has_third_dup) {
                m3 = store.ingest_file_manifest(dup3->absolute_path.string(), dup3->relative_path);
                tr.expect(vector_eq(m1.ordered_chunk_ids, m3.ordered_chunk_ids),
                          "same-content files produce identical ordered chunk IDs (file 1 vs file 3)");
            } else {
                tr.note("Dedup group has only 2 files; third-copy check skipped.");
            }

            std::set<ChunkID> unique_ids(m1.ordered_chunk_ids.begin(), m1.ordered_chunk_ids.end());
            const bool chunk_ids_look_valid =
                std::all_of(m1.ordered_chunk_ids.begin(), m1.ordered_chunk_ids.end(),
                            [](const ChunkID& id) { return !id.empty() && id.size() == 64; });
            tr.expect(chunk_ids_look_valid,
                      "generated chunk IDs are non-empty and look like fixed-length content hashes");

            ByteBuffer original = read_file_bytes(dup1.absolute_path);
            ByteBuffer rebuilt = reconstruct_file(store, m1);
            tr.expect(original == rebuilt, "read_chunk() can reconstruct original file exactly");

            bool all_dec_ref_first = true;
            for (const auto& chunk_id : m1.ordered_chunk_ids) {
                all_dec_ref_first = store.dec_ref(chunk_id) && all_dec_ref_first;
            }
            tr.expect(all_dec_ref_first, "dec_ref succeeds for all chunks of first logical copy");
            std::size_t removed_after_first_delete = store.gc();
            tr.expect(removed_after_first_delete == 0,
                      "GC does not remove shared chunks after deleting only one logical file");

            bool all_dec_ref_second = true;
            for (const auto& chunk_id : m2.ordered_chunk_ids) {
                all_dec_ref_second = store.dec_ref(chunk_id) && all_dec_ref_second;
            }
            tr.expect(all_dec_ref_second, "dec_ref succeeds for all chunks of second logical copy");

            if (has_third_dup) {
                std::size_t removed_after_second_delete = store.gc();
                tr.expect(removed_after_second_delete == 0,
                          "GC still keeps chunks because one logical copy still references them");

                bool all_dec_ref_third = true;
                for (const auto& chunk_id : m3.ordered_chunk_ids) {
                    all_dec_ref_third = store.dec_ref(chunk_id) && all_dec_ref_third;
                }
                tr.expect(all_dec_ref_third, "dec_ref succeeds for all chunks of final logical copy");

                std::size_t removed_after_third_delete = store.gc();
                tr.expect(removed_after_third_delete == unique_ids.size(),
                          "GC removes exactly the unique physical chunks after the final reference is deleted");
            } else {
                std::size_t removed_after_second_delete = store.gc();
                tr.expect(removed_after_second_delete == unique_ids.size(),
                          "GC removes exactly the unique physical chunks after the final reference is deleted");
            }

            auto first_chunk_after_gc = store.read_chunk(m1.ordered_chunk_ids.front());
            tr.expect(!first_chunk_after_gc.has_value(),
                      "removed chunk is no longer readable after final GC");
        }

        std::cout << "\n";

        // ---------------------------------------------------------------------
        // CASE 2: Recovery basic (re-open store from persisted index)
        // ---------------------------------------------------------------------
        if (fx.has_recovery_file) {
            fs::path case_dir = storage_root / "case_recovery";
            fs::remove_all(case_dir);

            FileChunkManifest manifest_before_restart;
            {
                ChunkStore store(case_dir.string(), chunk_size);
                tr.expect(store.init(), "init() succeeds for recovery case");
                manifest_before_restart =
                    store.ingest_file_manifest(fx.recovery_file.absolute_path.string(),
                                               fx.recovery_file.relative_path);
                tr.expect(!manifest_before_restart.ordered_chunk_ids.empty(),
                          "recovery file has at least one chunk");
            }

            {
                ChunkStore store(case_dir.string(), chunk_size);
                tr.expect(store.init(), "re-opening ChunkStore succeeds using persisted index");
                ByteBuffer original = read_file_bytes(fx.recovery_file.absolute_path);
                ByteBuffer rebuilt = reconstruct_file(store, manifest_before_restart);
                tr.expect(original == rebuilt,
                          "data remains readable after rebuilding ChunkStore from disk");
            }
        }

        std::cout << "\n";

        // ---------------------------------------------------------------------
        // CASE 3: Full-folder ingest + full verification sweep
        // ---------------------------------------------------------------------
        {
            fs::path case_dir = storage_root / "case_full_folder";
            fs::remove_all(case_dir);

            ChunkStore store(case_dir.string(), chunk_size);
            tr.expect(store.init(), "init() succeeds for full-folder case");

            FolderIngestResult folder_result = store.ingest_folder(dataset_root.string());
            auto manifest_by_path = build_manifest_map(folder_result);

            tr.expect(folder_result.total_files == scan.files.size(),
                      "ingest_folder() sees the same number of regular files as filesystem scan");

            std::uint64_t manifest_total_bytes = 0;
            std::uint64_t manifest_chunk_refs = 0;
            std::size_t zero_chunk_files = 0;
            std::size_t non_empty_files_with_chunks = 0;
            bool all_relative_paths_unique = (manifest_by_path.size() == folder_result.files.size());
            bool every_manifest_matches_fs_metadata = true;
            bool every_file_reconstructed_exactly = true;
            std::string first_bad_path;
            std::uint64_t verified_chunk_reads = 0;
            std::set<ChunkID> unique_chunk_ids;

            for (const auto& info : scan.files) {
                auto it = manifest_by_path.find(info.relative_path);
                if (it == manifest_by_path.end()) {
                    every_manifest_matches_fs_metadata = false;
                    if (first_bad_path.empty()) first_bad_path = info.relative_path + " (missing manifest)";
                    continue;
                }

                const auto& manifest = it->second;
                manifest_total_bytes += manifest.file_size;
                manifest_chunk_refs += manifest.ordered_chunk_ids.size();

                const bool metadata_ok =
                    (manifest.file_size == info.size) &&
                    ((info.is_empty && manifest.ordered_chunk_ids.empty()) ||
                     (!info.is_empty && !manifest.ordered_chunk_ids.empty()));

                if (!metadata_ok) {
                    every_manifest_matches_fs_metadata = false;
                    if (first_bad_path.empty()) first_bad_path = info.relative_path + " (size/chunk count mismatch)";
                }

                if (manifest.ordered_chunk_ids.empty()) {
                    ++zero_chunk_files;
                } else {
                    ++non_empty_files_with_chunks;
                }

                try {
                    ByteBuffer original = read_file_bytes(info.absolute_path);
                    ByteBuffer rebuilt = reconstruct_file(store, manifest);
                    if (original != rebuilt) {
                        every_file_reconstructed_exactly = false;
                        if (first_bad_path.empty()) first_bad_path = info.relative_path + " (reconstruction mismatch)";
                    }
                } catch (const std::exception&) {
                    every_file_reconstructed_exactly = false;
                    if (first_bad_path.empty()) first_bad_path = info.relative_path + " (read/reconstruct exception)";
                }

                verified_chunk_reads += manifest.ordered_chunk_ids.size();
                unique_chunk_ids.insert(manifest.ordered_chunk_ids.begin(), manifest.ordered_chunk_ids.end());
            }

            tr.expect(manifest_total_bytes == scan.total_bytes,
                      "sum of manifest file sizes matches filesystem byte scan");
            tr.expect(folder_result.total_chunks == manifest_chunk_refs,
                      "folder_result.total_chunks matches the sum of per-file chunk references");
            tr.expect(folder_result.unique_new_chunks + folder_result.duplicate_chunks == folder_result.total_chunks,
                      "unique_new_chunks + duplicate_chunks equals total_chunks");
            tr.expect(folder_result.duplicate_chunks > 0 && unique_chunk_ids.size() < folder_result.total_chunks,
                      "dataset actually exercises dedup in folder ingest");
            tr.expect(all_relative_paths_unique,
                      "every manifest relative path is unique across the whole dataset");
            tr.expect(every_manifest_matches_fs_metadata,
                      "every manifest matches filesystem metadata (exists, size, empty/non-empty)");
            tr.expect(zero_chunk_files == scan.empty_files,
                      "all empty files are represented with zero chunks");
            tr.expect(non_empty_files_with_chunks + zero_chunk_files == scan.files.size(),
                      "every scanned file is represented in manifest accounting");

            if (fx.has_unicode_file) {
                tr.expect(manifest_by_path.count(fx.unicode_file.relative_path) == 1,
                          "a Unicode path is preserved in manifest");
            } else {
                tr.note("No Unicode path present in dataset; manifest Unicode-path assertion skipped.");
            }

            if (fx.has_same_basename_pair) {
                tr.expect(manifest_by_path.count(fx.same_name_a.relative_path) == 1 &&
                              manifest_by_path.count(fx.same_name_b.relative_path) == 1,
                          "same file name in different folders is preserved by relative path");
            } else {
                tr.note("No same-name pair present in dataset; relative-path collision assertion skipped.");
            }

            tr.expect(every_file_reconstructed_exactly,
                      "full verification sweep reconstructs every ingested file byte-for-byte");
            tr.expect(verified_chunk_reads == folder_result.total_chunks,
                      "verification sweep touches every chunk reference recorded in folder manifest");

            tr.note("Full-folder stats: files=" + std::to_string(folder_result.total_files) +
                    ", chunk_refs=" + std::to_string(folder_result.total_chunks) +
                    ", unique_new=" + std::to_string(folder_result.unique_new_chunks) +
                    ", duplicate_refs=" + std::to_string(folder_result.duplicate_chunks) +
                    ", unique_chunk_ids=" + std::to_string(unique_chunk_ids.size()));
            if (!first_bad_path.empty()) {
                tr.note("First verification mismatch: " + first_bad_path);
            }

            const fs::path manifest_out = case_dir / "folder_manifest.txt";
            tr.expect(store.export_manifest_text(folder_result, manifest_out.string()),
                      "export_manifest_text() writes summary file");
            tr.expect(fs::exists(manifest_out), "manifest file exists on disk");

            std::string manifest_text;
            const bool manifest_text_ok = read_text_file(manifest_out, manifest_text);
            tr.expect(manifest_text_ok &&
                          manifest_text.find("total_files=" + std::to_string(folder_result.total_files)) != std::string::npos &&
                          manifest_text.find("duplicate_chunks=" + std::to_string(folder_result.duplicate_chunks)) != std::string::npos,
                      "exported manifest summary contains the expected aggregate counters");
        }

        std::cout << "\n";

        // ---------------------------------------------------------------------
        // CASE 4: Bloom filter false positive demo
        // ---------------------------------------------------------------------
        {
            BloomFilter tiny_bloom(32, 3);
            std::unordered_set<std::string> inserted;

            for (int i = 0; i < 50; ++i) {
                std::string key = "inserted_" + std::to_string(i);
                inserted.insert(key);
                tiny_bloom.add(key);
            }

            bool found_false_positive = false;
            std::string candidate;

            for (int i = 0; i < 5000; ++i) {
                candidate = "candidate_" + std::to_string(i);
                if (inserted.find(candidate) == inserted.end() &&
                    tiny_bloom.possibly_contains(candidate)) {
                    found_false_positive = true;
                    break;
                }
            }

            tr.expect(found_false_positive,
                      "tiny BloomFilter demo produces a false positive as expected");
            if (found_false_positive) {
                tr.note("example false-positive key: " + candidate);
            }
        }

        std::cout << "\n=== Summary ===\n";
        std::cout << "Passed: " << tr.passed << "\n";
        std::cout << "Failed: " << tr.failed << "\n";

        return tr.exit_code();
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] " << ex.what() << "\n";
        return 2;
    }
}
