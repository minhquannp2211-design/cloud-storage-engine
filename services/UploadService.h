#pragma once

#include <string>
#include "../managers/IndexManager.h"
#include "../managers/FileStructureBuilder.h"
#include "../managers/NamespaceManager.h"

class UploadService
{
public:
    void UploadFile(const std::string& filePath);
    void UploadFolder(const std::string& folderPath);
};
