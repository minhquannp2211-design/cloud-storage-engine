#include "bloom_filter.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

namespace layer1 {
namespace {

std::pair<std::size_t, std::size_t> compute_base_hashes(const std::string& key) {
    static std::hash<std::string> hasher;

    const std::size_t h1 = hasher(key);
    const std::size_t h2 = (hasher("salt:" + key) << 1u) | 1u; // always odd
    return {h1, h2};
}

} // namespace

BloomFilter::BloomFilter(std::size_t bit_count, std::size_t hash_count)
    : bit_count_(std::max<std::size_t>(bit_count, 8)),
      hash_count_(std::max<std::size_t>(hash_count, 1)),
      bits_(bit_count_, false) {}

void BloomFilter::add(const std::string& key) {
    const auto [h1, h2] = compute_base_hashes(key);
    for (std::size_t i = 0; i < hash_count_; ++i) {
        bits_[(h1 + i * h2) % bit_count_] = true;
    }
}

bool BloomFilter::possibly_contains(const std::string& key) const {
    const auto [h1, h2] = compute_base_hashes(key);
    for (std::size_t i = 0; i < hash_count_; ++i) {
        if (!bits_[(h1 + i * h2) % bit_count_]) {
            return false;
        }
    }
    return true;
}

void BloomFilter::clear() {
    std::fill(bits_.begin(), bits_.end(), false);
}

std::size_t BloomFilter::nth_hash(std::size_t n, const std::string& key) const {
    const auto [h1, h2] = compute_base_hashes(key);
    return (h1 + n * h2) % bit_count_;
}

} // namespace layer1
