cmake_minimum_required(VERSION 3.6)
project(Catch2)

add_library(Catch2)
target_sources(Catch2 PRIVATE "src/catch_amalgamated.cpp")
target_include_directories(Catch2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
