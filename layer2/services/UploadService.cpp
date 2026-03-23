#include "UploadService.h"
#include <iostream>
#include "../managers/FileStructureBuilder.h"
#include "../managers/NamespaceManager.h"
#include "../database/Database.h"
#include "../models/FileStatus.h"
#include "../include/chunk_store.hpp"

bool UploadService::UploadFile(const std::string& filePath)
{
    return UploadFile(filePath, filePath);
}
bool UploadService::UploadFile(const std::string& localFilePath, const std::string& logicalPath)
{
    std::cout << "Uploading file: " << localFilePath
              << " as logical path: " << logicalPath << std::endl;

    try
    {
        auto db = Database::GetInstance("metadata.db");

        layer1::ChunkStore store("data_storage", 4096);
        if (!store.init()) {
            std::cout << "Failed to initialize Layer 1 store\n";
            return false;
        }

        layer1::FileChunkManifest manifest =
            store.ingest_file_manifest(localFilePath, logicalPath);

        FileStructureBuilder builder;
        NamespaceManager ns;
        int userId = 1;

        db->BeginTransaction();

        try
        {
            int fileID = builder.CreateFileEntry(
                logicalPath,
                userId,
                static_cast<long>(manifest.file_size),
                logicalPath
            );

            builder.LinkChunksToFile(fileID, manifest.ordered_chunk_ids);

            FileRecord record = builder.BuildFileRecord(
                logicalPath,
                manifest.ordered_chunk_ids
            );
            record.fileID = fileID;
            record.userID = userId;
            record.totalSize = static_cast<long>(manifest.file_size);
            record.path = logicalPath;
            record.status = FileStatus::UPLOADING;
            record.created_at = time(nullptr);
            record.updated_at = record.created_at;

            ns.RegisterFile(record);

            builder.UpdateFileStatus(fileID, FileStatus::COMMITTED);
            db->Commit();

            std::cout << "Upload complete (fileID=" << fileID << ")\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error during upload: " << e.what() << "\n";
            if (db->IsInTransaction())
            {
                db->Rollback();
            }
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error in UploadFile: " << e.what() << "\n";
        return false;
    }
}

bool UploadService::UploadFolder(const std::string& folderPath)
{
    std::cout << "Uploading folder: " << folderPath << std::endl;

    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        layer1::ChunkStore store("data_storage", 4096);
        if (!store.init()) {
            std::cout << "Failed to initialize Layer 1 store\n";
            return false;
        }

        layer1::FolderIngestResult result = store.ingest_folder(folderPath);

        FileStructureBuilder builder;
        NamespaceManager ns;
        int userId = 1;

        // BEGIN TRANSACTION
        db->BeginTransaction();

        try
        {
            for (const auto& file : result.files) {
                // Create file entry với status UPLOADING
                int fileID = builder.CreateFileEntry(
                    file.relative_path,
                    userId,
                    static_cast<long>(file.file_size),
                    file.relative_path
                );

                // Link chunks
                builder.LinkChunksToFile(fileID, file.ordered_chunk_ids);

                // Register trong namespace
                FileRecord record = builder.BuildFileRecord(
                    file.relative_path,
                    file.ordered_chunk_ids
                );
                record.fileID = fileID;
                record.userID = userId;
                record.totalSize = static_cast<long>(file.file_size);
                record.path = file.relative_path;
                record.status = FileStatus::UPLOADING;
                record.created_at = time(nullptr);
                record.updated_at = record.created_at;

                ns.RegisterFile(record);

                // Update status to COMMITTED
                builder.UpdateFileStatus(fileID, FileStatus::COMMITTED);
            }

            // COMMIT TRANSACTION
            db->Commit();

            std::cout << "Upload complete (" << result.files.size() << " files)\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error during folder upload: " << e.what() << "\n";
            
            // ROLLBACK TRANSACTION
            if (db->IsInTransaction())
            {
                db->Rollback();
            }
            
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error in UploadFolder: " << e.what() << "\n";
        return false;
    }
}
