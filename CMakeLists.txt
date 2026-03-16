cmake_minimum_required(VERSION 3.16)
project(cloud_storage_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    include
    managers
    services
    models
    database
    config
    utils
)

add_library(layer1
    src/bloom_filter.cpp
    src/chunker.cpp
    src/chunk_index.cpp
    src/chunk_store.cpp
)

target_include_directories(layer1 PUBLIC include)


add_executable(layer1_demo
    src/main.cpp
)

target_link_libraries(layer1_demo PRIVATE layer1)

add_library(appcore
    layer2/database/Database.cpp

    layer2/managers/FileStructureBuilder.cpp
    layer2/managers/IndexManager.cpp
    layer2/managers/NamespaceManager.cpp

    layer2/services/UploadService.cpp
    layer2/services/DownloadService.cpp
    layer2/services/DeleteService.cpp

    layer2/utils/Chunker.cpp
    layer2/utils/Hash.cpp
)

target_include_directories(appcore PUBLIC
    include
    managers
    services
    models
    database
    config
    utils
)

target_link_libraries(appcore PRIVATE layer1)

add_executable(app_demo
    layer2/app/main.cpp
)

target_link_libraries(app_demo PRIVATE appcore layer1)

enable_testing()

add_executable(test_smoke
    tests/test_smoke.cpp
)

target_link_libraries(test_smoke PRIVATE layer1)

add_test(NAME smoke_test COMMAND test_smoke)
