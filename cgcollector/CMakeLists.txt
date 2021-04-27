cmake_minimum_required(VERSION 3.5)

project(CGCollector VERSION 0.2.0)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

include(ToolchainOptions)
include(CMakePackageConfigHelpers)
include(GetGitRevisionDescription)

get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message(${GIT_SHA1})

configure_file(Config.h.in config.h)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)
