#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

struct Probe {
    int passed = 0;
    int failed = 0;

    void expect(bool cond, const std::string& msg) {
        if (cond) {
            ++passed;
            std::cout << "[PASS] " << msg << "\n";
        } else {
            ++failed;
            std::cout << "[FAIL] " << msg << "\n";
        }
    }

    int exit_code() const { return failed == 0 ? 0 : 1; }
};

static void reset_runtime() {
    fs::remove("metadata.db");
    fs::remove_all("data_storage");
    fs::remove_all("cli_probe_runtime");
    fs::create_directories("cli_probe_runtime/input");
    fs::create_directories("cli_probe_runtime/logs");
}

static void write_text(const fs::path& path, const std::string& data) {
    fs::create_directories(path.parent_path());
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs << data;
}

static std::string read_text(const fs::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

static int run_shell(const std::string& cmd) {
    std::cout << "$ " << cmd << "\n";
    return std::system(cmd.c_str());
}

static std::string q(const std::string& s) {
    return "\"" + s + "\"";
}

int main(int argc, char* argv[]) {
    Probe t;
    try {
        std::string app = (argc >= 2) ? argv[1] : "..\\cmake-build-debug\\app_demo.exe";
        reset_runtime();

        const std::string cloud = "/sample.txt";
        const std::string local = "cli_probe_runtime/input/sample.txt";
        const std::string out   = "cli_probe_runtime/logs/download.txt";
        const std::string outAfterDelete = "cli_probe_runtime/logs/download_after_delete.txt";

        const std::string uploadLog = "cli_probe_runtime/logs/upload.txt";
        const std::string getLog    = "cli_probe_runtime/logs/get.txt";
        const std::string delLog    = "cli_probe_runtime/logs/delete.txt";
        const std::string del2Log   = "cli_probe_runtime/logs/delete_again.txt";

        const std::string content = "CLI_END_TO_END_SAMPLE_PAYLOAD\nsecond line\n";
        write_text(local, content);

        std::cout << "=== CLI probe start ===\n";

        int rc = 0;

        rc = run_shell(app + " PUT " + q(cloud) + " " + q(local) + " > " + q(uploadLog) + " 2>&1");
        t.expect(rc == 0, "PUT exits with code 0");

        rc = run_shell(app + " GET " + q(cloud) + " " + q(out) + " > " + q(getLog) + " 2>&1");
        t.expect(rc == 0, "GET exits with code 0");

        std::string downloaded = read_text(out);
        t.expect(downloaded == content, "downloaded file matches original");

        rc = run_shell(app + " DEL " + q(cloud) + " > " + q(delLog) + " 2>&1");
        t.expect(rc == 0, "DEL exits with code 0");

        rc = run_shell(app + " GET " + q(cloud) + " " + q(outAfterDelete) + " > nul 2>&1");
        t.expect(rc != 0, "GET after delete returns error");

        rc = run_shell(app + " DEL " + q(cloud) + " > " + q(del2Log) + " 2>&1");
        t.expect(rc == 0 || rc == 1, "second DEL has controlled behavior");

        std::string deleteAgainText = read_text(del2Log);
        t.expect(
            deleteAgainText.find("File not found") != std::string::npos ||
            deleteAgainText.find("already in deletion state") != std::string::npos ||
            deleteAgainText.find("not found") != std::string::npos,
            "second DEL prints observable state message"
        );

        std::cout << "\n=== CLI probe summary ===\n";
        std::cout << "Passed: " << t.passed << "\n";
        std::cout << "Failed: " << t.failed << "\n";
        std::cout << "Logs stored under cli_probe_runtime/logs/\n";

        return t.exit_code();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 2;
    }
}