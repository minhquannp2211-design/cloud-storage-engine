#pragma once
#include <string>

enum class FileStatus
{
    UPLOADING  = 0,   // Upload đang diễn ra
    COMMITTED  = 1,   // Upload hoàn tất, dữ liệu ổn định
    DELETING   = 2,   // Deletion đang diễn ra
    DELETED    = 3    // Đã xóa hoàn toàn
};

inline const char* FileStatusToString(FileStatus status)
{
    switch (status)
    {
        case FileStatus::UPLOADING: return "UPLOADING";
        case FileStatus::COMMITTED: return "COMMITTED";
        case FileStatus::DELETING:  return "DELETING";
        case FileStatus::DELETED:   return "DELETED";
        default:                    return "UNKNOWN";
    }
}

inline FileStatus StringToFileStatus(const std::string& str)
{
    if (str == "UPLOADING") return FileStatus::UPLOADING;
    if (str == "COMMITTED") return FileStatus::COMMITTED;
    if (str == "DELETING")  return FileStatus::DELETING;
    if (str == "DELETED")   return FileStatus::DELETED;
    return FileStatus::COMMITTED; // default
}
