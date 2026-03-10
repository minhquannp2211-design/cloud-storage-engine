#pragma once
#include <string>

class Hash {
public:
    static std::string SHA256(const std::string& data);
};