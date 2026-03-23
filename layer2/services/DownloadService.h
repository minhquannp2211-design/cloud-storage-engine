#pragma once
#include <string>

class DownloadService {
public:
    std::string DownloadFile(const std::string& path);
    bool DownloadFileToPath(const std::string& logicalPath, const std::string& outputPath);
};
