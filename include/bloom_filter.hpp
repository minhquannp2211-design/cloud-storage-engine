#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace layer1 {

class BloomFilter {
public:
    BloomFilter(std::size_t bit_count = 1 << 20, std::size_t hash_count = 3);

    void add(const std::string& key);
    bool possibly_contains(const std::string& key) const;
    void clear();

private:
    std::size_t nth_hash(std::size_t n, const std::string& key) const;

private:
    std::size_t bit_count_;
    std::size_t hash_count_;
    std::vector<bool> bits_;
};

} // namespace layer1   