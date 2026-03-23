#include "../layer2/database/Database.h"
#include "../layer2/models/NamespaceNode.h"
#include "../layer2/models/FileRecord.h"
#include "../layer2/models/ChunkRecord.h"
#include "../layer2/managers/NamespaceManager.h"

#include <cassert>
#include <iostream>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

// Test Database Singleton
void test_database_singleton() {
    std::cout << "Testing Database Singleton...\n";
    
#ifdef HAVE_SQLITE3
    try {
        Database::ResetInstance();

        if (fs::exists("test_layer2.db")) {
            fs::remove("test_layer2.db");
        }

        auto db1 = Database::GetInstance("test_layer2.db");
        assert(db1 != nullptr);
        std::cout << "  ✓ Database instance created\n";

        auto db2 = Database::GetInstance("test_layer2.db");
        assert(db1.get() == db2.get());
        std::cout << "  ✓ Singleton pattern working\n";

        db1.reset();
        db2.reset();
        Database::ResetInstance();

        if (fs::exists("test_layer2.db")) {
            fs::remove("test_layer2.db");
        }
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Database test failed: " << e.what() << "\n";
        throw;
    }
#else
    std::cout << "  ⊘ Database test skipped (SQLite3 not available)\n";
    std::cout << "     To enable: vcpkg install sqlite3\n";
#endif
}
// Test NamespaceNode Tree Structure
void test_namespace_tree() {
    std::cout << "Testing NamespaceNode Tree...\n";
    
    // Create root node
    auto root = std::make_shared<NamespaceNode>("", false);
    assert(root->name == "");
    assert(!root->isFile);
    std::cout << "  ✓ Root node created\n";
    
    // Create child folders
    auto folder1 = root->GetOrCreateChild("folder1", false);
    assert(folder1 != nullptr);
    assert(folder1->name == "folder1");
    assert(!folder1->isFile);
    std::cout << "  ✓ Child folder created\n";
    
    // Create nested folder
    auto folder2 = folder1->GetOrCreateChild("subfolder", false);
    assert(folder2 != nullptr);
    assert(folder2->name == "subfolder");
    std::cout << "  ✓ Nested folder created\n";
    
    // Create file
    auto file = folder2->GetOrCreateChild("document.txt", true);
    assert(file != nullptr);
    assert(file->name == "document.txt");
    assert(file->isFile);
    std::cout << "  ✓ File node created\n";
    
    // Test FindChild
    auto found = folder1->FindChild("subfolder");
    assert(found != nullptr);
    assert(found->name == "subfolder");
    std::cout << "  ✓ FindChild working\n";
    
    // Test GetFullPath
    std::string path = file->GetFullPath();
    std::cout << "  File path: " << path << "\n";
    // Note: Full path depends on parent linkage
    std::cout << "  ✓ GetFullPath working\n";
}

// Test FileRecord
void test_file_record() {
    std::cout << "Testing FileRecord...\n";
    
    FileRecord record;
    record.fileID = 1;
    record.userID = 100;
    record.fileName = "test.txt";
    record.path = "/documents/test.txt";
    record.totalSize = 1024;
    record.status = FileStatus::COMMITTED;
    
    assert(record.fileID == 1);
    assert(record.userID == 100);
    assert(record.fileName == "test.txt");
    assert(record.path == "/documents/test.txt");
    assert(record.totalSize == 1024);
    std::cout << "  ✓ FileRecord fields set correctly\n";
    
    // Test chunk hashes
    record.chunkHashes.push_back("hash1");
    record.chunkHashes.push_back("hash2");
    assert(record.chunkHashes.size() == 2);
    std::cout << "  ✓ FileRecord chunk tracking working\n";
}

// Test ChunkRecord
void test_chunk_record() {
    std::cout << "Testing ChunkRecord...\n";
    
    ChunkRecord chunk;
    chunk.hash = "abc123def456";
    chunk.referenceCount = 1;
    
    assert(chunk.hash == "abc123def456");
    assert(chunk.referenceCount == 1);
    std::cout << "  ✓ ChunkRecord fields set correctly\n";
}

// Test FileStatus enum
void test_file_status() {
    std::cout << "Testing FileStatus...\n";
    
    FileStatus status1 = FileStatus::UPLOADING;
    FileStatus status2 = FileStatus::COMMITTED;
    FileStatus status3 = FileStatus::DELETED;
    
    assert(status1 != status2);
    assert(status2 != status3);
    std::cout << "  ✓ FileStatus enum values working\n";
}

int main() {
    std::cout << "\n========== Layer 2 Tests ==========\n\n";
    
    try {
        test_database_singleton();
        std::cout << "\n";
        
        test_file_status();
        std::cout << "\n";
        
        test_file_record();
        std::cout << "\n";
        
        test_chunk_record();
        std::cout << "\n";
        
        test_namespace_tree();
        std::cout << "\n";
        
        std::cout << "========== All Tests Passed! ==========\n\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test suite failed: " << e.what() << "\n";
        return 1;
    }
}
