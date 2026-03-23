#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "FileStatus.h"

struct FileRecord
{
    int fileID{0};
    int userID{0};
    std::string fileName;                    // Logical filename/path
    long totalSize{0};
    std::vector<std::string> chunkHashes;
    
    // Metadata mới
    FileStatus status{FileStatus::COMMITTED}; // Trạng thái hiện tại
    std::string path;                         // Logical path (có thể chứa /)
    time_t created_at{0};                     // Thời gian tạo
    time_t updated_at{0};                     // Lần cập nhật cuối
};