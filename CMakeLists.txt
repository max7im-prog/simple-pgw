cmake_minimum_required(VERSION 3.15)
project("simple_pgw" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
