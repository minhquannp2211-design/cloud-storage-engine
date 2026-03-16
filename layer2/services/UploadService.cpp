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
    NamespaceManager ns;
    int userId = 1;

    int fileID = builder.CreateFileEntry(
        manifest.relative_path,
        userId,
        static_cast<long>(manifest.file_size)
    );

    builder.LinkChunksToFile(fileID, manifest.ordered_chunk_ids);

    FileRecord record = builder.BuildFileRecord(
        manifest.relative_path,
        manifest.ordered_chunk_ids
    );
    record.fileID = fileID;
    record.userID = userId;
    record.totalSize = static_cast<long>(manifest.file_size);

    ns.RegisterFile(record);

    std::cout << "Upload complete\n";
}

void UploadService::UploadFolder(const std::string& folderPath)
{
    std::cout << "Uploading folder: " << folderPath << std::endl;

    layer1::ChunkStore store("data_storage", 4096);
    if (!store.init()) {
        std::cout << "Failed to initialize Layer 1 store\n";
        return;
    }

    layer1::FolderIngestResult result = store.ingest_folder(folderPath);

    FileStructureBuilder builder;
    NamespaceManager ns;
    int userId = 1;

    for (const auto& file : result.files) {
        int fileID = builder.CreateFileEntry(
            file.relative_path,
            userId,
            static_cast<long>(file.file_size)
        );

        builder.LinkChunksToFile(fileID, file.ordered_chunk_ids);

        FileRecord record = builder.BuildFileRecord(
            file.relative_path,
            file.ordered_chunk_ids
        );
        record.fileID = fileID;
        record.userID = userId;
        record.totalSize = static_cast<long>(file.file_size);

        ns.RegisterFile(record);
    }

    std::cout << "Upload complete\n";
}
