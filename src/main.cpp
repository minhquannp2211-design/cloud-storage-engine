#include "chunk_store.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace layer1;

static void write_demo_file(const std::string& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs << content;
}

int main() {
    try {
        ChunkStore store("data_storage", 8);
        if (!store.init()) {
            std::cerr << "Failed to initialize ChunkStore\n";
            return 1;
        }

        fs::create_directories("uploaded_folder/sub");
        write_demo_file("uploaded_folder/a.txt", "AAAABBBBCCCCDDDDAAAABBBB");
        write_demo_file("uploaded_folder/b.txt", "AAAABBBB");
        write_demo_file("uploaded_folder/sub/c.txt", "CCCCDDDDAAAABBBB");

        auto result = store.ingest_folder("uploaded_folder");

        std::cout << "Total files: " << result.total_files << "\n";
        std::cout << "Total chunks: " << result.total_chunks << "\n";
        std::cout << "Unique new chunks: " << result.unique_new_chunks << "\n";
        std::cout << "Duplicate chunks: " << result.duplicate_chunks << "\n";

        for (const auto& file : result.files) {
            std::cout << "FILE: " << file.relative_path
                      << " | size=" << file.file_size
                      << " | chunks=" << file.ordered_chunk_ids.size() << "\n";

            for (const auto& chunk_id : file.ordered_chunk_ids) {
                std::cout << "  - " << chunk_id << "\n";
            }
        }

        if (!store.export_manifest_text(result, "layer2_manifest.txt")) {
            std::cerr << "Failed to export manifest\n";
            return 1;
        }

        std::cout << "Manifest exported to layer2_manifest.txt\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << "\n";
        return 1;
    }
}