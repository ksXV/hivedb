cmake_minimum_required(VERSION 3.11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-pedantic -Wall -Wextra -Werror -Wshadow -g)

project(hive VERSION 1.0)

add_subdirectory(lib/catch2)

add_executable(tests tests/lexer.cpp src/parser/lexer.cpp src/parser/tokens.cpp)
add_executable(hive src/main.cpp src/parser/lexer.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include)

target_link_libraries(tests PRIVATE Catch2)
target_link_libraries(hive PRIVATE)
