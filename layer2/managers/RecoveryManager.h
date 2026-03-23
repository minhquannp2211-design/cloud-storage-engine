#pragma once
#include <string>
#include <vector>
#include "../models/FileRecord.h"

class RecoveryManager
{
public:
    // Startup recovery: scan database for incomplete transactions
    static void RecoverFromCrash();
    
private:
    // Rollback files in UPLOADING status (incomplete uploads)
    static void RollbackIncompleteUploads();
    
    // Complete or rollback files in DELETING status
    static void CompleteIncompleteDeletes();
};
