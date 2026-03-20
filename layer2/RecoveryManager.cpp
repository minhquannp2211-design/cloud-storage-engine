#include "RecoveryManager.h"
#include "../database/Database.h"
#include "../models/FileStatus.h"
#include <iostream>

void RecoveryManager::RecoverFromCrash()
{
    std::cout << "RecoveryManager: Starting crash recovery...\n";
    
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Handle incomplete uploads
        RollbackIncompleteUploads();
        
        // Handle incomplete deletes
        CompleteIncompleteDeletes();
        
        std::cout << "RecoveryManager: Recovery completed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "RecoveryManager error: " << e.what() << "\n";
    }
}

void RecoveryManager::RollbackIncompleteUploads()
{
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Find all files in UPLOADING status
        auto results = db->executeSelect(
            "SELECT fileID, path FROM files WHERE status = 'UPLOADING'"
        );
        
        if (!results.empty())
        {
            std::cout << "RecoveryManager: Found " << results.size() << " incomplete uploads\n";
            
            // For each incomplete upload, set status to DELETED (rollback)
            for (const auto& row : results)
            {
                if (row.size() >= 1)
                {
                    std::string fileID = row[0];
                    std::string path = row.size() > 1 ? row[1] : "unknown";
                    
                    // Rollback: set status to DELETED
                    std::string query = "UPDATE files SET status = 'DELETED', updated_at = " + 
                                       std::to_string(time(nullptr)) + " WHERE fileID = " + fileID;
                    
                    db->executeUpdate(query);
                    
                    std::cout << "RecoveryManager: Rolled back incomplete upload for " << path << "\n";
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "RecoveryManager error during rollback: " << e.what() << "\n";
    }
}

void RecoveryManager::CompleteIncompleteDeletes()
{
    try
    {
        auto db = Database::GetInstance("metadata.db");
        
        // Find all files in DELETING status
        auto results = db->executeSelect(
            "SELECT fileID, path FROM files WHERE status = 'DELETING'"
        );
        
        if (!results.empty())
        {
            std::cout << "RecoveryManager: Found " << results.size() << " incomplete deletes\n";
            
            // Complete deletes: set status to DELETED
            for (const auto& row : results)
            {
                if (row.size() >= 1)
                {
                    std::string fileID = row[0];
                    std::string path = row.size() > 1 ? row[1] : "unknown";
                    
                    std::string query = "UPDATE files SET status = 'DELETED', updated_at = " + 
                                       std::to_string(time(nullptr)) + " WHERE fileID = " + fileID;
                    
                    db->executeUpdate(query);
                    
                    std::cout << "RecoveryManager: Completed delete for " << path << "\n";
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "RecoveryManager error during delete completion: " << e.what() << "\n";
    }
}
