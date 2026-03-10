#include "UploadService.h"
#include <iostream>
#include "../managers/FileStructureBuilder.h"
#include "../managers/NamespaceManager.h"
#include "../include/chunk_store.hpp"   

void UploadService::UploadFile(const std::string& filePath)
{
    std::cout << "Uploading file: " << filePath << std::endl;

    layer1::ChunkStore store("data_storage", 4096);
    if (!store.init()) {
        std::cout << "Failed to initialize Layer 1 store\n";
        return;
    }

    layer1::FileChunkManifest manifest = store.ingest_file_manifest(filePath, filePath);

    FileStructureBuilder builder;
    auto record = builder.BuildFileRecord(manifest.relative_path, manifest.ordered_chunk_ids);

    NamespaceManager ns;
    ns.RegisterFile(record);

    std::cout << "Upload complete\n";
}
