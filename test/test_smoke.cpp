#include "chunk_store.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace layer1;
namespace fs = std::filesystem;

static void write_file(const std::string& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs << content;
}

int main() {
    fs::remove_all("test_storage");
    fs::remove("test_input.txt");

    ChunkStore store("test_storage", 4);
    assert(store.init());

    write_file("test_input.txt", "ABCDEFGHABCDEFGH");

    auto results = store.store_file("test_input.txt");
    assert(!results.empty());

    // Có dedup vì 2 nửa giống nhau theo chunk size = 4:
    // ABCD EFGH ABCD EFGH
    // chunk 1 và 3 trùng; chunk 2 và 4 trùng
    assert(results.size() == 4);

    auto c1 = results[0].chunk_id;
    auto c2 = results[1].chunk_id;
    auto c3 = results[2].chunk_id;
    auto c4 = results[3].chunk_id;

    assert(c1 == c3);
    assert(c2 == c4);

    auto data = store.read_chunk(c1);
    assert(data.has_value());
    assert(data->size() == 4);

    assert(store.dec_ref(c1));
    assert(store.dec_ref(c1));
    // Lần thứ 3 sẽ fail vì ref_count đã về 0
    assert(!store.dec_ref(c1));

    std::size_t removed = store.gc();
    assert(removed >= 1);

    auto after_gc = store.read_chunk(c1);
    assert(!after_gc.has_value());

    std::cout << "Smoke test passed.\n";
    return 0;
}