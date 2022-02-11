# This ships with Ubuntu 20.04 and supports C++20
cmake_minimum_required(VERSION 3.16)

### Require out-of-source builds (taken from https://hsf-training.github.io/hsf-training-cmake-webpage/07-commonproblems/index.html)
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with "
            "CMakeLists.txt file). Please make a build subdirectory. Feel free to "
            "remove CMakeCache.txt and CMakeFiles.")
endif()
### End of require out-of-source builds

## Project information
set(METACG_VERSION 0.3.0)
project(MetaCG VERSION ${METACG_VERSION} DESCRIPTION "An annotatable whole-program call-graph toolset")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(METACG_Directory ${CMAKE_CURRENT_SOURCE_DIR})

# This also downloads and builds googletest
include(ToolchainOptions)
# Making packaging easier
include(CMakePackageConfigHelpers)
# Making printing / debugging easier
include(CMakePrintHelpers)


### Component options
# MetaCG graph library will always be built.
# The actual graph implementation
add_subdirectory(graph)

# The collector toolchain: CGCollector, CGMerge, and CGValidate
# Should CGCollector be built
option(WITH_CGCOLLECTOR "On or off" ON)
if (WITH_CGCOLLECTOR)
  add_subdirectory(cgcollector)
endif()

# PIRA analyzer
# Should PGIS be built
option(WITH_PGIS "On or off" ON)
if (WITH_PGIS)
  add_subdirectory(pgis)
endif()
