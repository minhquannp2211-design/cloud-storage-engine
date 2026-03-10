#include "DownloadService.h"
#include "../managers/NamespaceManager.h"

std::string DownloadService::DownloadFile(const std::string& path){
    NamespaceManager ns;
    auto opt = ns.GetFile(path);
    if (!opt.has_value()) return std::string();
    auto file = opt.value();

    std::string result;
    for (const auto& h : file.chunkHashes) {
        // For demo, append first two chars of each chunk-hash like original behavior
        if (h.size() >= 2) result += h.substr(0,2);
    }

    return result;
}