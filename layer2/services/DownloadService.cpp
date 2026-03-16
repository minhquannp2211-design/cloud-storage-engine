#include "DownloadService.h"
#include "../managers/NamespaceManager.h"
#include "../include/chunk_store.hpp"

#include <fstream>

std::string DownloadService::DownloadFile(const std::string& path) {
    NamespaceManager ns;
    auto opt = ns.GetFile(path);
    if (!opt.has_value()) {
        return std::string();
    }

    auto file = opt.value();

    layer1::ChunkStore store("data_storage", 4096);
    if (!store.init()) {
        return std::string();
    }

    std::string result;
    for (const auto& chunk_id : file.chunkHashes) {
        auto data = store.read_chunk(chunk_id);
        if (!data.has_value()) {
            return std::string(); // thiếu chunk hoặc chunk corrupt
        }

        result.append(data->begin(), data->end());
    }

    return result;
}
