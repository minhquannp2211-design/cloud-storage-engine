#include "UploadService.h"
#include <iostream>
#include "../utils/Chunker.h"
#include "../utils/Hash.h"

void UploadService::UploadFile(const std::string& filePath)
{
    std::cout << "Uploading file: " << filePath << std::endl;

    auto chunks = Chunker::Split(filePath);
    if (chunks.empty()) {
        std::cout << "No chunks produced (file missing or empty)\n";
        return;
    }

    IndexManager index;
    FileStructureBuilder builder;
    std::vector<std::string> hashes;
    hashes.reserve(chunks.size());

    for (auto& c : chunks)
    {
        const std::string& h = c.hash;
        if (!index.CheckChunkExists(h))
            index.RegisterNewChunk(h);
        else
            index.IncrementReference(h);

        hashes.push_back(h);
    }

    auto record = builder.BuildFileRecord(filePath, hashes);
    NamespaceManager ns;
    ns.RegisterFile(record);

    std::cout << "Upload complete\n";
}