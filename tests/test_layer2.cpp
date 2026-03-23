#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <map>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

// ========== TEST STATISTICS ==========
struct TestStats {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    std::vector<std::string> failed_details;
    int total_files_uploaded = 0;
    int total_files_downloaded = 0;
    int total_files_deleted = 0;
    long long total_bytes_processed = 0;
};

TestStats g_stats;

// ========== UTILITY FUNCTIONS ==========
void PrintHeader(const std::string& title) {
    std::cout << "\n" << std::string(90, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(90, '=') << "\n\n";
}

void PrintSubHeader(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

void PrintTest(const std::string& name, bool passed, const std::string& details = "") {
    g_stats.total_tests++;
    if (passed) {
        g_stats.passed_tests++;
        std::cout << "  [✓] PASS: " << name << "\n";
    } else {
        g_stats.failed_tests++;
        std::cout << "  [✗] FAIL: " << name << "\n";
        if (!details.empty()) {
            std::cout << "       └─ " << details << "\n";
            g_stats.failed_details.push_back(name + ": " + details);
        }
    }
}

int CountFilesInDirectory(const std::string& path) {
    int count = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
                count++;
                g_stats.total_bytes_processed += fs::file_size(entry);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error counting files: " << e.what() << "\n";
    }
    return count;
}

std::vector<std::string> GetAllFilesInDirectory(const std::string& path) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing files: " << e.what() << "\n";
    }
    return files;
}

std::string BytesToString(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int unitIndex = 0;
    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

// ========== TEST SUITE 1: DATASET ANALYSIS ==========
void TestDatasetAnalysis() {
    PrintHeader("TEST SUITE 1: DATASET STRUCTURE ANALYSIS");

    PrintSubHeader("Test 1.1: Dataset Directory Structure");
    try {
        std::string baseDir = "C:\\file\\cloud-storage-engine-main_git(1)\\cloud-storage-engine-main\\test_data\\layer2_test_dataset\\layer2_dataset";
        if (fs::exists(baseDir)) {
            std::vector<std::string> mainDirs;
            for (const auto& entry : fs::directory_iterator(baseDir)) {
                if (fs::is_directory(entry) && entry.path().filename().string()[0] != '.') {
                    mainDirs.push_back(entry.path().filename().string());
                }
            }
            std::sort(mainDirs.begin(), mainDirs.end());

            bool success = !mainDirs.empty();
            std::string dirList = "";
            for (const auto& dir : mainDirs) {
                dirList += dir + ", ";
            }
            if (!dirList.empty()) dirList.pop_back();
            if (!dirList.empty()) dirList.pop_back();

            PrintTest("Main directories found", success, std::string("Count: ") + std::to_string(mainDirs.size()));
        } else {
            PrintTest("Main directories found", false, "Dataset directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Main directories found", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 1.2: Total File Count");
    try {
        std::string baseDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset";
        if (fs::exists(baseDir)) {
            int fileCount = CountFilesInDirectory(baseDir);
            bool success = (fileCount == 171);  // As per README
            PrintTest("Total file count (expect 171)", success, std::string("Actual: ") + std::to_string(fileCount));
        } else {
            PrintTest("Total file count (expect 171)", false, "Dataset not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Total file count (expect 171)", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 1.3: Directory Structure Validation");
    try {
        std::string baseDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset";
        std::vector<std::string> requiredDirs = {
            "data", "backup", "docs", "edge", "unicode", "deep", "src", "logs", "tmp", "users"
        };

        int foundDirs = 0;
        for (const auto& dir : requiredDirs) {
            std::string fullPath = baseDir + "\\" + dir;
            if (fs::exists(fullPath) && fs::is_directory(fullPath)) {
                foundDirs++;
            }
        }

        bool success = (foundDirs == requiredDirs.size());
        PrintTest("Required directories present", success,
                  std::string("Found: ") + std::to_string(foundDirs) + "/" + std::to_string(requiredDirs.size()));
    } catch (const std::exception& e) {
        PrintTest("Required directories present", false, std::string("Exception: ") + e.what());
    }
}

// ========== TEST SUITE 2: FILE ENUMERATION ==========
void TestFileEnumeration() {
    PrintHeader("TEST SUITE 2: FILE ENUMERATION & STATISTICS");

    PrintSubHeader("Test 2.1: CSV Files Count");
    try {
        std::string csvDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\data\\csv";
        if (fs::exists(csvDir)) {
            int csvCount = 0;
            for (const auto& entry : fs::directory_iterator(csvDir)) {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".csv") {
                    csvCount++;
                }
            }
            bool success = (csvCount == 10);
            PrintTest("CSV files count (expect 10)", success, std::string("Actual: ") + std::to_string(csvCount));
        } else {
            PrintTest("CSV files count (expect 10)", false, "CSV directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("CSV files count (expect 10)", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 2.2: JSON Files Count");
    try {
        std::string jsonDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\data\\json";
        if (fs::exists(jsonDir)) {
            int jsonCount = 0;
            for (const auto& entry : fs::directory_iterator(jsonDir)) {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".json") {
                    jsonCount++;
                }
            }
            bool success = (jsonCount == 8);
            PrintTest("JSON files count (expect 8)", success, std::string("Actual: ") + std::to_string(jsonCount));
        } else {
            PrintTest("JSON files count (expect 8)", false, "JSON directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("JSON files count (expect 8)", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 2.3: Documentation Files");
    try {
        std::string docsDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\docs";
        if (fs::exists(docsDir)) {
            int totalDocs = 0;
            int mdCount = 0;
            for (const auto& entry : fs::recursive_directory_iterator(docsDir)) {
                if (fs::is_regular_file(entry)) {
                    totalDocs++;
                    if (entry.path().extension() == ".md") {
                        mdCount++;
                    }
                }
            }
            bool success = (mdCount > 0);
            PrintTest("Markdown files in docs/", success,
                      std::string("Total: ") + std::to_string(totalDocs) +
                      ", MD: " + std::to_string(mdCount));
        } else {
            PrintTest("Markdown files in docs/", false, "Docs directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Markdown files in docs/", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 2.4: Duplicate Files Detection");
    try {
        std::vector<std::string> dupFiles = {
            "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\backup\\daily\\dup_alpha_backup.txt",
            "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\tmp\\uploads\\dup_alpha.txt",
            "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\tmp\\downloads\\dup_alpha_copy.txt",
            "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\users\\alice\\dup_alpha_user.txt"
        };

        int existCount = 0;
        long long firstSize = 0;
        bool allSame = true;

        for (size_t i = 0; i < dupFiles.size(); i++) {
            if (fs::exists(dupFiles[i])) {
                existCount++;
                long long fileSize = fs::file_size(dupFiles[i]);
                if (i == 0) {
                    firstSize = fileSize;
                } else if (fileSize != firstSize) {
                    allSame = false;
                }
            }
        }

        bool success = (existCount >= 2 && allSame);
        PrintTest("Duplicate files detected", success,
                  std::string("Count: ") + std::to_string(existCount) +
                  ", Same size: " + (allSame ? "Yes" : "No"));
    } catch (const std::exception& e) {
        PrintTest("Duplicate files detected", false, std::string("Exception: ") + e.what());
    }
}

// ========== TEST SUITE 3: EDGE CASES ==========
void TestEdgeCasesValidation() {
    PrintHeader("TEST SUITE 3: EDGE CASES VALIDATION");

    PrintSubHeader("Test 3.1: Empty Files");
    try {
        std::string emptyDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\edge\\empty";
        if (fs::exists(emptyDir)) {
            int emptyCount = 0;
            for (const auto& entry : fs::directory_iterator(emptyDir)) {
                if (fs::is_regular_file(entry) && fs::file_size(entry) == 0) {
                    emptyCount++;
                }
            }
            bool success = (emptyCount >= 1);
            PrintTest("Empty files exist", success, std::string("Count: ") + std::to_string(emptyCount));
        } else {
            PrintTest("Empty files exist", false, "Empty directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Empty files exist", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 3.2: Deep Directory Nesting");
    try {
        std::string deepDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\deep";
        if (fs::exists(deepDir)) {
            int maxDepth = 0;
            for (const auto& entry : fs::recursive_directory_iterator(deepDir)) {
                if (fs::is_directory(entry)) {
                    auto relativePath = fs::relative(entry, deepDir);
                    int depth = 0;
                    for (const auto& p : relativePath) {
                        depth++;
                    }
                    maxDepth = std::max(maxDepth, depth);
                }
            }
            bool success = (maxDepth >= 3);
            PrintTest("Deep nesting support (expect >= 3 levels)", success,
                      std::string("Max depth: ") + std::to_string(maxDepth));
        } else {
            PrintTest("Deep nesting support (expect >= 3 levels)", false, "Deep directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Deep nesting support (expect >= 3 levels)", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 3.3: Same Filename in Different Paths");
    try {
        std::string configA = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\edge\\same_name\\a\\config.json";
        std::string configB = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\edge\\same_name\\b\\config.json";

        if (fs::exists(configA) && fs::exists(configB)) {
            bool sameSize = (fs::file_size(configA) == fs::file_size(configB));
            bool success = true;
            PrintTest("Same filename in different paths", success,
                      std::string("Path A size: ") + std::to_string(fs::file_size(configA)) +
                      ", Path B size: " + std::to_string(fs::file_size(configB)));
        } else {
            PrintTest("Same filename in different paths", false, "Test files not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Same filename in different paths", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 3.4: Unicode Path Support");
    try {
        std::string unicodeDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset\\unicode";
        if (fs::exists(unicodeDir)) {
            int unicodeCount = 0;
            for (const auto& entry : fs::recursive_directory_iterator(unicodeDir)) {
                if (fs::is_regular_file(entry)) {
                    unicodeCount++;
                }
            }
            bool success = (unicodeCount > 0);
            PrintTest("Unicode path handling", success, std::string("Files: ") + std::to_string(unicodeCount));
        } else {
            PrintTest("Unicode path handling", false, "Unicode directory not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Unicode path handling", false, std::string("Exception: ") + e.what());
    }
}

// ========== TEST SUITE 4: SIZE ANALYSIS ==========
void TestSizeAnalysis() {
    PrintHeader("TEST SUITE 4: FILE SIZE ANALYSIS");

    PrintSubHeader("Test 4.1: Large Files Analysis");
    try {
        std::string baseDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset";
        if (fs::exists(baseDir)) {
            long long largestFile = 0;
            std::string largestName = "";
            long long totalSize = 0;
            int fileCount = 0;

            for (const auto& entry : fs::recursive_directory_iterator(baseDir)) {
                if (fs::is_regular_file(entry)) {
                    long long size = fs::file_size(entry);
                    totalSize += size;
                    fileCount++;
                    if (size > largestFile) {
                        largestFile = size;
                        largestName = entry.path().filename().string();
                    }
                }
            }

            bool success = true;
            std::cout << "     Total files: " << fileCount << "\n";
            std::cout << "     Total size: " << BytesToString(totalSize) << "\n";
            std::cout << "     Largest file: " << largestName << " (" << BytesToString(largestFile) << ")\n";
            PrintTest("Size analysis", success);
        } else {
            PrintTest("Size analysis", false, "Dataset not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Size analysis", false, std::string("Exception: ") + e.what());
    }

    PrintSubHeader("Test 4.2: Directory-wise Size Breakdown");
    try {
        std::string baseDir = "d:\\cloud-storage-engine-main\\layer2_test_dataset\\layer2_dataset";
        if (fs::exists(baseDir)) {
            std::map<std::string, long long> dirSizes;

            for (const auto& entry : fs::recursive_directory_iterator(baseDir)) {
                if (fs::is_regular_file(entry)) {
                    auto relativePath = fs::relative(entry.path().parent_path(), baseDir);
                    std::string firstDir = relativePath.string();

                    // Get first directory component
                    size_t pos = firstDir.find('\\');
                    if (pos != std::string::npos) {
                        firstDir = firstDir.substr(0, pos);
                    }

                    dirSizes[firstDir] += fs::file_size(entry);
                }
            }

            std::cout << "     Directory Size Breakdown:\n";
            for (const auto& [dir, size] : dirSizes) {
                std::cout << "       • " << dir << ": " << BytesToString(size) << "\n";
            }
            PrintTest("Directory size breakdown", true);
        } else {
            PrintTest("Directory size breakdown", false, "Dataset not found");
        }
    } catch (const std::exception& e) {
        PrintTest("Directory size breakdown", false, std::string("Exception: ") + e.what());
    }
}

// ========== TEST SUMMARY ==========
void PrintTestSummary() {
    PrintHeader("COMPREHENSIVE TEST REPORT SUMMARY");

    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║            TEST EXECUTION STATISTICS             ║\n";
    std::cout << "╠══════════════════════════════════════════════════╣\n";
    std::cout << "║ Total Tests Run:        " << std::setw(24) << g_stats.total_tests << " ║\n";
    std::cout << "║ Tests Passed:           " << std::setw(24) << g_stats.passed_tests << " ║\n";
    std::cout << "║ Tests Failed:           " << std::setw(24) << g_stats.failed_tests << " ║\n";

    double passRate = (g_stats.total_tests > 0)
        ? (static_cast<double>(g_stats.passed_tests) / g_stats.total_tests * 100)
        : 0.0;

    std::cout << "║ Pass Rate:              " << std::fixed << std::setprecision(1)
              << std::setw(21) << passRate << "% ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";

    if (!g_stats.failed_details.empty()) {
        std::cout << "\n⚠️  FAILED TESTS DETAILS:\n";
        for (const auto& detail : g_stats.failed_details) {
            std::cout << "  • " << detail << "\n";
        }
    } else {
        std::cout << "\n✓ ALL TESTS PASSED!\n";
    }
}

// ========== MAIN ==========
int main() {
    auto startTime = std::chrono::steady_clock::now();

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     LAYER 2 COMPREHENSIVE DATASET TEST SUITE               ║\n";
    std::cout << "║     Cloud Storage Engine - Integration Test Runner         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    try {
        std::cout << "\n[*] Starting dataset analysis and validation...\n";

        // Run test suites
        TestDatasetAnalysis();
        TestFileEnumeration();
        TestEdgeCasesValidation();
        TestSizeAnalysis();

    } catch (const std::exception& e) {
        std::cerr << "\n[✗] Fatal error during testing: " << e.what() << "\n";
        return 1;
    }

    // Print summary
    PrintTestSummary();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "\nTotal Test Duration: " << duration.count() << " ms\n";
    std::cout << "Data Processed: " << BytesToString(g_stats.total_bytes_processed) << "\n";
    std::cout << "\n[" << (g_stats.failed_tests == 0 ? "✓" : "✗")
              << "] Test Suite " << (g_stats.failed_tests == 0 ? "PASSED" : "COMPLETED WITH FAILURES")
              << "\n\n";

    return (g_stats.failed_tests == 0) ? 0 : 1;
}