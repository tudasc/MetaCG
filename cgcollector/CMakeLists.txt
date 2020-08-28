cmake_minimum_required(VERSION 3.5)

project(CGCollector VERSION 0.2)


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


include(ExternalProject)

if(DEFINED JSON_INCLUDE_PATH)
  message("JSON_INCLUDE_PATH predefined: ${JSON_INCLUDE_PATH}")
else()
  find_path(JSON_LIBRARY NAMES json)
  if(JSON_LIBRARY)
    set(JSON_INCLUDE_PATH ${JSON_LIBRARY}/json/single_include)
    message("JSON found in ${JSON_INCLUDE_PATH}")
  else()
    message("JSON library not found, download into extern during make")
    ExternalProject_Add(json
      SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/extern/json
      GIT_REPOSITORY      "https://github.com/nlohmann/json.git"
      GIT_TAG             master
      CONFIGURE_COMMAND   ""
      BUILD_COMMAND       ""
      INSTALL_COMMAND     ""
      TEST_COMMAND        ""
      GIT_SHALLOW         true
    )
    set(JSON_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extern/json/single_include)
  endif()
endif()
include_directories(${JSON_INCLUDE_PATH})

if(DEFINED CXXOPTS_INCLUDE_PATH)
  message("CXXOPTS_INCLUDE_PATH predefined: ${CXXOPTS_INCLUDE_PATH}")
else()
  message("CXXOPTS library not found, download into extern during make")
  ExternalProject_Add(cxxopts
    SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/extern/cxxopts
    GIT_REPOSITORY      "https://github.com/jarro2783/cxxopts.git"
    GIT_TAG             v2.2.1
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     ""
    TEST_COMMAND        ""
    GIT_SHALLOW         true
  )
set(CXXOPTS_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extern/cxxopts/include)
endif()
include_directories(${CXXOPTS_INCLUDE_PATH})

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)
