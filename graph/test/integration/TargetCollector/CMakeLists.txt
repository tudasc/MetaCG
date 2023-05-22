cmake_minimum_required(VERSION 3.16)

project(TargetCollectorIntegrationTest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(testExe src/foo.cpp src/main.cpp)

target_include_directories(testExe PRIVATE include)
