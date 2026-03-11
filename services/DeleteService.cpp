#include "DeleteService.h"
#include "../include/chunk_store.hpp"

#include <iostream>

void DeleteService::DeleteFile(const std::string& fileName)
{
    NamespaceManager ns;

    if (!ns.FileExists(fileName))
    {
        std::cout << "File not found\n";
        return;
    }

    auto opt = ns.GetFile(fileName);
    if (!opt.has_value()) {
        std::cout << "File not found (concurrent deletion?)\n";
        return;
    }

    auto file = opt.value();

    layer1::ChunkStore store("data_storage", 4096);
    if (!store.init()) {
        std::cout << "Failed to initialize Layer 1 store\n";
        return;
    }

    for (const auto& chunk_id : file.chunkHashes) {
        if (!store.dec_ref(chunk_id)) {
            std::cout << "Warning: dec_ref failed for chunk " << chunk_id << "\n";
        }
    }

    std::size_t removed = store.gc();

    ns.DeleteFile(fileName);

    std::cout << "File deleted: " << fileName << std::endl;
    std::cout << "GC removed " << removed << " chunk(s)\n";
}

