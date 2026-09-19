cmake_minimum_required(VERSION 3.16)

project(RediForge
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(rediforge
    src/main.cpp
    src/server/server.cpp
    src/server/client_handler.cpp
    src/server/client.cpp
    src/protocol/resp_parser.cpp
    src/protocol/resp_value.cpp
    src/commands/command_handler.cpp
    src/database/database.cpp
    src/database/redis_value.cpp
    src/database/lfu.cpp
    src/database/lru.cpp
    src/Replica/replication_manager.cpp
    src/election/election_manager.cpp
    src/discovery/discovery_manager.cpp
)

target_include_directories(rediforge
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/protocol
    ${PROJECT_SOURCE_DIR}/include/commands
    ${PROJECT_SOURCE_DIR}/include/database
    ${PROJECT_SOURCE_DIR}/include/Replica
    ${PROJECT_SOURCE_DIR}/include/election
    ${PROJECT_SOURCE_DIR}/include/discovery
)