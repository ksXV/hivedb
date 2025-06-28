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

# TODO: ADD tests/ast.cpp back in here
add_executable(tests
tests/lexer.cpp
tests/parser.cpp
src/parser/lexer.cpp
src/parser/tokens.cpp
src/parser/parser.cpp
)

add_executable(hive
src/main.cpp
src/parser/lexer.cpp
src/parser/tokens.cpp
src/parser/parser.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)

target_link_libraries(tests PRIVATE Catch2 fmt::fmt)
target_link_libraries(hive PRIVATE fmt::fmt)
