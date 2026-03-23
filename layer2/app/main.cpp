#include "../services/UploadService.h"
#include "../services/DownloadService.h"
#include "../services/DeleteService.h"
#include "../managers/NamespaceManager.h"
#include "../managers/FileStructureBuilder.h"
#include "../managers/RecoveryManager.h"

#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

static void print_usage() {
    std::cout << "Usage:\n";
    std::cout << "  app_demo PUT <logical_path> <local_file>\n";
    std::cout << "  app_demo GET <logical_path> <output_file>\n";
    std::cout << "  app_demo DEL <logical_path>\n";
    std::cout << "\nLegacy commands:\n";
    std::cout << "  app_demo upload-file <file_path>\n";
    std::cout << "  app_demo upload-folder <folder_path>\n";
    std::cout << "  app_demo download-file <logical_path>\n";
    std::cout << "  app_demo delete-file <logical_path>\n";
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "=== Initializing Layer 2 ===\n";
        RecoveryManager::RecoverFromCrash();
        NamespaceManager::Initialize();
        FileStructureBuilder::Initialize();
        std::cout << "=== Initialization complete ===\n\n";
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << "\n";
        return 1;
    }

    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];

    UploadService uploadService;
    DownloadService downloadService;
    DeleteService deleteService;

    try {
        // ===== New CLI contract for probe =====
        if (command == "PUT") {
            if (argc != 4) {
                std::cerr << "PUT requires: <logical_path> <local_file>\n";
                return 1;
            }

            std::string logicalPath = argv[2];
            std::string localFile   = argv[3];

            bool ok = uploadService.UploadFile(localFile, logicalPath);
            return ok ? 0 : 1;
        }

        if (command == "GET") {
            if (argc != 4) {
                std::cerr << "GET requires: <logical_path> <output_file>\n";
                return 1;
            }

            std::string logicalPath = argv[2];
            std::string outputFile = argv[3];

            try {
                bool ok = downloadService.DownloadFileToPath(logicalPath, outputFile);
                return ok ? 0 : 1;
            } catch (const std::exception& e) {
                std::cerr << e.what() << "\n";
                return 1;
            }
        }

        if (command == "DEL") {
            if (argc != 3) {
                std::cerr << "DEL requires: <logical_path>\n";
                print_usage();
                return 1;
            }

            std::string logicalPath = argv[2];
            deleteService.DeleteFile(logicalPath);
            std::cout << "DEL success: " << logicalPath << "\n";
            return 0;
        }

        // ===== Legacy commands =====
        if (command == "upload-file") {
            if (argc != 3) {
                std::cerr << "upload-file requires: <file_path>\n";
                return 1;
            }
            uploadService.UploadFile(argv[2]);
            return 0;
        }

        if (command == "upload-folder") {
            if (argc != 3) {
                std::cerr << "upload-folder requires: <folder_path>\n";
                return 1;
            }
            uploadService.UploadFolder(argv[2]);
            return 0;
        }

        if (command == "download-file") {
            if (argc != 3) {
                std::cerr << "download-file requires: <logical_path>\n";
                return 1;
            }
            std::string content = downloadService.DownloadFile(argv[2]);
            std::cout << content;
            return 0;
        }

        if (command == "delete-file") {
            if (argc != 3) {
                std::cerr << "delete-file requires: <logical_path>\n";
                return 1;
            }
            deleteService.DeleteFile(argv[2]);
            return 0;
        }

        std::cerr << "Unknown command: " << command << "\n";
        print_usage();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Command failed: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Command failed: unknown error\n";
        return 1;
    }
}