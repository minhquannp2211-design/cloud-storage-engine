#include "DeleteService.h"
#include "../managers/NamespaceManager.h"
#include "../database/Database.h"
#include "../models/FileStatus.h"
#include "../include/chunk_store.hpp"
#include <iostream>

bool DeleteService::DeleteFile(const std::string& path)
{
    std::cout << "Deleting file: " << path << std::endl;

    try
    {
        auto db = Database::GetInstance("metadata.db");
        NamespaceManager ns;

        // Check nếu file tồn tại
        if (!ns.FileExists(path))
        {
            std::cout << "File not found\n";
            return false;
        }

        auto opt = ns.GetFile(path);
        if (!opt.has_value()) {
            std::cout << "File not found (concurrent deletion?)\n";
            return false;
        }

        auto file = opt.value();

        // Idempotency check: nếu đã DELETED hoặc DELETING, skip
        if (file.status == FileStatus::DELETED || file.status == FileStatus::DELETING)
        {
            std::cout << "File already in deletion state\n";
            return true;  // Idempotent: return success
        }

        layer1::ChunkStore store("data_storage", 4096);
        if (!store.init()) {
            std::cout << "Failed to initialize Layer 1 store\n";
            return false;
        }

        // BEGIN TRANSACTION
        db->BeginTransaction();

        try
        {
            // Update status to DELETING
            std::string query = "UPDATE files SET status = 'DELETING', updated_at = " + 
                               std::to_string(time(nullptr)) + " WHERE path = '" + path + "'";
            db->executeUpdate(query);

            // Decrement chunk references
            for (const auto& chunk_id : file.chunkHashes) {
                if (!store.dec_ref(chunk_id)) {
                    std::cout << "Warning: dec_ref failed for chunk " << chunk_id << "\n";
                }
            }

            // Run garbage collection
            std::size_t removed = store.gc();

            // Update status to DELETED
            query = "UPDATE files SET status = 'DELETED', updated_at = " + 
                   std::to_string(time(nullptr)) + " WHERE path = '" + path + "'";
            db->executeUpdate(query);

            // Remove from namespace
            ns.DeleteFile(path);

            // COMMIT TRANSACTION
            db->Commit();

            std::cout << "File deleted: " << path << std::endl;
            std::cout << "GC removed " << removed << " chunk(s)\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error during delete: " << e.what() << "\n";
            
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
        std::cerr << "Fatal error in DeleteFile: " << e.what() << "\n";
        return false;
    }
}


