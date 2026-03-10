#include "DeleteService.h"
#include "../managers/NamespaceManager.h"
#include "../managers/IndexManager.h"

#include <iostream>

void DeleteService::DeleteFile(const std::string& fileName)
{
    NamespaceManager ns;
    IndexManager index;

    if (!ns.FileExists(fileName))
    {
        std::cout << "File not found\n";
        return;
    }

    auto opt = ns.GetFile(fileName);
    if (!opt.has_value()) {
        std::cout << "File not found (concurrent deletion?)\n";
        return;
    }

    auto file = opt.value();

    for (const auto& hash : file.chunkHashes)
        index.DecrementReference(hash);

    ns.DeleteFile(fileName);

    std::cout << "File deleted: " << fileName << std::endl;
}