#include "DownloadService.h"
#include "../managers/NamespaceManager.h"
#include "../include/chunk_store.hpp"

#include <fstream>
#include <stdexcept>

std::string DownloadService::DownloadFile(const std::string& path) {
    NamespaceManager ns;
    auto opt = ns.GetFile(path);
    if (!opt.has_value()) {
        throw std::runtime_error("File not found: " + path);
    }

    auto file = opt.value();

    layer1::ChunkStore store("data_storage", 4096);
    if (!store.init()) {
        throw std::runtime_error("Failed to initialize Layer 1 store");
    }

    std::string result;
    for (const auto& chunk_id : file.chunkHashes) {
        auto data = store.read_chunk(chunk_id);
        if (!data.has_value()) {
            throw std::runtime_error("Missing or corrupt chunk: " + chunk_id);
        }

        result.append(data->begin(), data->end());
    }

    return result;
}

bool DownloadService::DownloadFileToPath(const std::string& logicalPath, const std::string& outputPath) {
    std::string content = DownloadFile(logicalPath);

    std::ofstream ofs(outputPath, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error("Cannot open output file: " + outputPath);
    }

    ofs << content;
    return true;
}