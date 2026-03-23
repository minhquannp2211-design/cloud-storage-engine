cmake_minimum_required(VERSION 3.20)
project(cloud_storage_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

message(STATUS "CMAKE_C_COMPILER   = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_GENERATOR    = ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_BUILD_TYPE   = ${CMAKE_BUILD_TYPE}")

set(COMMON_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2/database
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2/models
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2/managers
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2/services
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2/app
)

# Dùng package config do vcpkg cung cấp
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(layer1 STATIC
        src/bloom_filter.cpp
        src/chunker.cpp
        src/chunk_index.cpp
        src/chunk_store.cpp
)
target_include_directories(layer1 PUBLIC ${COMMON_INCLUDE_DIRS})

add_library(layer2_core STATIC
        layer2/database/Database.cpp
        layer2/models/NamespaceNode.cpp
        layer2/managers/FileStructureBuilder.cpp
        layer2/managers/IndexManager.cpp
        layer2/managers/NamespaceManager.cpp
        layer2/managers/RecoveryManager.cpp
        layer2/services/DeleteService.cpp
        layer2/services/DownloadService.cpp
        layer2/services/UploadService.cpp
        layer2/utils/Chunker.cpp
        layer2/utils/Hash.cpp
)

target_include_directories(layer2_core PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(layer2_core PUBLIC
        layer1
        unofficial::sqlite3::sqlite3
)
target_compile_definitions(layer2_core PUBLIC HAVE_SQLITE3=1)

# =========================
# Layer 1
# =========================
add_library(layer1 STATIC
        src/bloom_filter.cpp
        src/chunker.cpp
        src/chunk_index.cpp
        src/chunk_store.cpp
)

target_include_directories(layer1 PUBLIC ${COMMON_INCLUDE_DIRS})

# =========================
# Layer 2 core
# =========================
add_library(layer2_core STATIC
        layer2/database/Database.cpp
        layer2/models/NamespaceNode.cpp
        layer2/managers/FileStructureBuilder.cpp
        layer2/managers/IndexManager.cpp
        layer2/managers/NamespaceManager.cpp
        layer2/managers/RecoveryManager.cpp
        layer2/services/DeleteService.cpp
        layer2/services/DownloadService.cpp
        layer2/services/UploadService.cpp
        layer2/utils/Chunker.cpp
        layer2/utils/Hash.cpp
)

target_include_directories(layer2_core PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(layer2_core PUBLIC layer1)

if (SQLite3_FOUND)
    target_link_libraries(layer2_core PUBLIC SQLite::SQLite3)
    target_compile_definitions(layer2_core PUBLIC HAVE_SQLITE3=1)
endif()

# =========================
# Layer 1 demo
# =========================
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(layer1_demo
            src/main.cpp
    )
    target_include_directories(layer1_demo PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(layer1_demo PRIVATE layer1)

    if (MINGW)
        target_link_libraries(layer1_demo PRIVATE stdc++fs)
    endif()
endif()

# =========================
# Layer 2 CLI app
# =========================
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/layer2/app/main.cpp")
    add_executable(app_demo
            layer2/app/main.cpp
    )
    target_include_directories(app_demo PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(app_demo PRIVATE layer2_core)

    if (MINGW)
        target_link_libraries(app_demo PRIVATE stdc++fs)
    endif()
endif()

# =========================
# Tests
# =========================
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_layer2.cpp")
    add_executable(test_layer2
            tests/test_layer2.cpp
    )
    target_include_directories(test_layer2 PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(test_layer2 PRIVATE layer2_core)

    if (MINGW)
        target_link_libraries(test_layer2 PRIVATE stdc++fs)
    endif()

    add_test(NAME test_layer2 COMMAND test_layer2)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_smoke.cpp")
    add_executable(test_smoke
            tests/test_smoke.cpp
    )
    target_include_directories(test_smoke PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(test_smoke PRIVATE layer1)

    if (MINGW)
        target_link_libraries(test_smoke PRIVATE stdc++fs)
    endif()

    add_test(NAME test_smoke COMMAND test_smoke)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_probe.cpp" AND TARGET app_demo)
    add_executable(cli_probe
            tests/cli_probe.cpp
    )
    target_include_directories(cli_probe PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(cli_probe PRIVATE layer2_core)

    if (MINGW)
        target_link_libraries(cli_probe PRIVATE stdc++fs)
    endif()

    add_test(NAME cli_probe COMMAND cli_probe $<TARGET_FILE:app_demo>)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/layer1_deep_runner.cpp")
    add_executable(layer1_deep_runner
            tests/layer1_deep_runner.cpp
    )
    target_include_directories(layer1_deep_runner PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(layer1_deep_runner PRIVATE layer1)

    if (MINGW)
        target_link_libraries(layer1_deep_runner PRIVATE stdc++fs)
    endif()
endif()
