cmake_minimum_required(VERSION 3.5)

project(pgoe)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


list(APPEND CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/cmake
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
	)

# This also downloads and builds googletest
include(ToolchainOptions)
include(CMakePackageConfigHelpers)

link_directories(
  ${CUBE_LIB}
  ${EXTRAP_LIB}
)

enable_testing()

add_subdirectory(lib)
add_subdirectory(tool)
add_subdirectory(test/unit)
