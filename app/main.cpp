#include "../services/UploadService.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage:\n";
        std::cout << "  app_demo upload-file <file_path>\n";
        std::cout << "  app_demo upload-folder <folder_path>\n";
        return 1;
    }

    std::string command = argv[1];
    std::string path = argv[2];

    UploadService uploadService;

    if (command == "upload-file") {
        uploadService.UploadFile(path);
    } else if (command == "upload-folder") {
        uploadService.UploadFolder(path);
    } else {
        std::cout << "Unknown command: " << command << "\n";
        return 1;
    }

    return 0;
}
