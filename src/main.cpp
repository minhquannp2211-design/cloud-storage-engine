#include "chunk_store.hpp"

#include <iostream>
#include <string>

using namespace layer1;

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: layer1_demo <folder_path>\n";
            return 1;
        }

        std::string folder_path = argv[1];

        ChunkStore store("data_storage", 8);
        if (!store.init()) {
            std::cerr << "Failed to initialize ChunkStore\n";
            return 1;
        }

        auto result = store.ingest_folder(folder_path);

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
