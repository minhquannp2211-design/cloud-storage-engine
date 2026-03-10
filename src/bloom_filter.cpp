#include "bloom_filter.hpp"

#include <functional>

namespace layer1 {

BloomFilter::BloomFilter(std::size_t bit_count, std::size_t hash_count)
    : bit_count_(bit_count), hash_count_(hash_count), bits_(bit_count, false) {}

void BloomFilter::add(const std::string& key) {
    for (std::size_t i = 0; i < hash_count_; ++i) {
        bits_[nth_hash(i, key)] = true;
    }
}

bool BloomFilter::possibly_contains(const std::string& key) const {
    for (std::size_t i = 0; i < hash_count_; ++i) {
        if (!bits_[nth_hash(i, key)]) {
            return false;
        }
    }
    return true;
}

void BloomFilter::clear() {
    std::fill(bits_.begin(), bits_.end(), false);
}

std::size_t BloomFilter::nth_hash(std::size_t n, const std::string& key) const {
    static std::hash<std::string> hasher;
    std::string mixed = key + "#" + std::to_string(n);
    return hasher(mixed) % bit_count_;
}

} // namespace layer1