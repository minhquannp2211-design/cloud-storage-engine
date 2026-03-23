#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "FileRecord.h"

// Node trong directory tree/trie
class NamespaceNode
{
public:
    std::string name;                                   // Tên của node này (file hoặc folder)
    bool isFile{false};                                 // true nếu đây là file, false nếu folder
    FileRecord* fileRecord{nullptr};                    // Con trỏ đến FileRecord nếu isFile=true
    
    std::unordered_map<std::string, 
        std::shared_ptr<NamespaceNode>> children;       // Con (cho folder)
    
    std::shared_ptr<NamespaceNode> parent;              // Cha (nullptr cho root)
    
    NamespaceNode() = default;
    NamespaceNode(const std::string& name, bool isFile = false)
        : name(name), isFile(isFile) {}
    
    // Trả về full path từ root
    std::string GetFullPath() const;
    
    // Tìm child node theo tên
    std::shared_ptr<NamespaceNode> FindChild(const std::string& name) const;
    
    // Tạo hoặc lấy child folder
    std::shared_ptr<NamespaceNode> GetOrCreateChild(const std::string& name, bool isFile = false);
};