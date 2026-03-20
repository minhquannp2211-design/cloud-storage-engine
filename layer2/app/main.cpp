#include "../services/UploadService.h"
#include "../services/DownloadService.h"
#include "../services/DeleteService.h"
#include "../managers/NamespaceManager.h"
#include "../managers/FileStructureBuilder.h"
#include "../managers/RecoveryManager.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Initialize managers from database
    try
    {
        std::cout << "=== Initializing Layer 2 ===\n";
        RecoveryManager::RecoverFromCrash();
        NamespaceManager::Initialize();
        FileStructureBuilder::Initialize();
        std::cout << "=== Initialization complete ===\n\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Initialization failed: " << e.what() << "\n";
        return 1;
    }
    
    if (argc < 3) {
        std::cout << "Usage:\n";
        std::cout << "  app_demo upload-file <file_path>\n";
        std::cout << "  app_demo upload-folder <folder_path>\n";
        std::cout << "  app_demo download-file <logical_path>\n";
        std::cout << "  app_demo delete-file <logical_path>\n";
        return 1;
    }

    std::string command = argv[1];
    std::string path = argv[2];

    UploadService uploadService;
    DownloadService downloadService;
    DeleteService deleteService;

    if (command == "upload-file") {
        uploadService.UploadFile(path);
    } else if (command == "upload-folder") {
        uploadService.UploadFolder(path);
    } else if (command == "download-file") {
        std::string content = downloadService.DownloadFile(path);
        std::cout << content << "\n";
    } else if (command == "delete-file") {
        deleteService.DeleteFile(path);
    } else {
        std::cout << "Unknown command: " << command << "\n";
        return 1;
    }

    return 0;
}

