#pragma once

#include <string>
#include "../managers/IndexManager.h"
#include "../managers/FileStructureBuilder.h"
#include "../managers/NamespaceManager.h"

class UploadService
{
public:
    // Upload file với transaction support
    // Transaction: UPLOADING -> COMMITTED
    bool UploadFile(const std::string& filePath);
    // New: explicit local file + logical path for CLI
    bool UploadFile(const std::string& localFilePath, const std::string& logicalPath);
    // Upload folder với transaction support
    bool UploadFolder(const std::string& folderPath);
};
