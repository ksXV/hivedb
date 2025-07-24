cmake_minimum_required(VERSION 3.11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-pedantic -Wall -Wextra -Werror -Wshadow -g -O0 -ggdb)

project(hive VERSION 1.0)

include(FetchContent)

add_subdirectory(lib/catch2)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

add_executable(tests
tests/lexer.cpp
tests/parser.cpp
tests/storage_engine.cpp

src/parser/lexer.cpp
src/parser/tokens.cpp
src/parser/parser.cpp

src/storage_engine/storage_engine.cpp
src/data_types/data_types.cpp
src/data_types/integer.cpp
src/data_types/real.cpp
src/data_types/varchar.cpp
        include/buffer_pool/lru_k.hpp
        src/buffer_pool/lru_k.cpp
        include/disk/disk_scheduler.hpp
        include/misc/channel.hpp
        src/misc/channel.cpp
        src/disk/disk_scheduler.cpp
        include/disk/disk_manager.hpp
)

add_executable(hive
src/main.cpp
src/parser/lexer.cpp
src/parser/tokens.cpp
src/parser/parser.cpp

src/storage_engine/storage_engine.cpp
src/data_types/data_types.cpp
src/data_types/integer.cpp
src/data_types/real.cpp
src/data_types/varchar.cpp
        include/buffer_pool/lru_k.hpp
        src/buffer_pool/lru_k.cpp
        src/buffer_pool/buffer_pool.cpp
        include/disk/disk_scheduler.hpp
        include/misc/channel.hpp
        src/misc/channel.cpp
        src/disk/disk_scheduler.cpp
        include/disk/disk_manager.hpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)

target_link_libraries(tests PRIVATE Catch2 fmt::fmt)
target_link_libraries(hive PRIVATE fmt::fmt)
