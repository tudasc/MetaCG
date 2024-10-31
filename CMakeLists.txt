# This ships with Ubuntu 20.04 and supports C++20
cmake_minimum_required(VERSION 3.16)

list(
  APPEND
  CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(Policies)

set_policies()

# Require out-of-source builds (taken from
# https://hsf-training.github.io/hsf-training-cmake-webpage/07-commonproblems/index.html)
file(
  TO_CMAKE_PATH
  "${PROJECT_BINARY_DIR}/CMakeLists.txt"
  LOC_PATH
)
if(EXISTS "${LOC_PATH}")
  message(
    FATAL_ERROR
      "You cannot build in a source directory (or any directory with "
      "CMakeLists.txt file). Please make a build subdirectory. Feel free to "
      "remove CMakeCache.txt and CMakeFiles."
  )
endif()
# End of require out-of-source builds

# Project information
set(METACG_VERSION 0.7.1)
project(
  MetaCG
  VERSION ${METACG_VERSION}
  DESCRIPTION "An annotatable whole-program call-graph toolset"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(METACG_Directory ${CMAKE_CURRENT_SOURCE_DIR})

# Whether the GoogleTest-based unit tests should be built and GoogleTest is downloaded as dependency This needs to be
# here, so ToolchainOptions already "sees" the option's value
option(
  METACG_BUILD_UNIT_TESTS
  "On or Off"
  ON
)

include(ToolchainOptions)
# Making packaging easier
include(CMakePackageConfigHelpers)
# Making printing / debugging easier
include(CMakePrintHelpers)

# Obtain ghe git revision of this MetaCG repository
include(GetGitRevisionDescription)

# Stream the verison info in the project's config.h
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
if(NOT
   DEFINED
   GIT_SHA1
)
  set(GIT_SHA1 "NO_GIT_ROOT")
endif()

# Configure the file that holds version information
configure_file(Config.h.in config.h)

# Provide googletest library
include(GoogleTest)

# Component options MetaCG graph library will always be built. The actual graph implementation
add_subdirectory(graph)

# Build the collector toolchain: CGCollector, CGMerge, and CGValidate
option(
  METACG_BUILD_CGCOLLECTOR
  "On or off"
  OFF
)
if(METACG_BUILD_CGCOLLECTOR)
  include(ClangLLVM)
  include(CubeLib)
  add_subdirectory(cgcollector)
endif()

# PIRA analyzer Should PGIS be built
option(
  METACG_BUILD_PGIS
  "On or off"
  OFF
)
if(METACG_BUILD_PGIS)
  include(ExtraP)
  include(CubeLib)
  add_subdirectory(pgis)
endif()

# If set to on, CMake looks for installed nlohmann::json
option(
  METACG_USE_EXTERNAL_JSON
  "On or off"
  OFF
)

# If set to on, CMake looks for installed cxxopts
option(
  METACG_USE_EXTERNAL_CXXOPTS
  "On or Off"
  OFF
)
