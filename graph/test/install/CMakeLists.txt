cmake_minimum_required(VERSION 3.16)

project(
  MetaCG-Test-User
  VERSION 0.1
  DESCRIPTION "An annotatable whole-program call-graph toolset"
)

find_package(metacg REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mainprog main.cpp)

target_link_libraries(mainprog metacg::metacg)
