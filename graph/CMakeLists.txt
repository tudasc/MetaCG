set(PROJECT_NAME graphlib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MetacgGraphLibSources
    src/CgNode.cpp
    src/Callgraph.cpp
    src/io/MCGReader.cpp
    src/io/MCGWriter.cpp
    src/io/VersionTwoMCGReader.cpp
    src/io/VersionThreeMCGReader.cpp
    src/MCGManager.cpp
    src/io/VersionTwoMCGWriter.cpp
    src/io/VersionThreeMCGWriter.cpp
    src/DotIO.cpp
    src/MCGBaseInfo.cpp
    src/ReachabilityAnalysis.cpp
    src/LoggerUtil.cpp
    include/io/MCGReader.h
    include/CgNode.h
    include/Callgraph.h
    include/CgNodePtr.h
    include/metadata/MetaData.h
    include/MCGManager.h
    include/io/MCGWriter.h
    include/Util.h
    include/MCGBaseInfo.h
    include/LoggerUtil.h
    include/DotIO.h
    include/Timing.h
    include/ReachabilityAnalysis.h
)

# Custom in-tree metadata
file(
  GLOB
  CUSTOM_METADATA
  "include/metadata/custom/*.h"
)
if(NOT
   CUSTOM_METADATA
   STREQUAL
   ""
)
  message(STATUS "Found custom metadata: ${CUSTOM_METADATA}")
endif()
# Prepare a string to store the new includes
set(INCLUDE_DIRECTIVES "")
# Loop through all collected files and create #include lines
foreach(file ${CUSTOM_METADATA})
  get_filename_component(
    filename
    ${file}
    NAME
  )
  # Append the #include directive to the INCLUDE_DIRECTIVES variable
  list(
    APPEND
    INCLUDE_DIRECTIVES
    "#include \"${filename}\""
  )
endforeach()
# Join the list of #include directives into a single string
string(
  REPLACE ";"
          "\n"
          INCLUDE_DIRECTIVES
          "${INCLUDE_DIRECTIVES}"
)

configure_file(
  include/metadata/CustomMD.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/metadata/CustomMD.h
  @ONLY
)

add_library(metacg SHARED ${MetacgGraphLibSources})
add_library(
  metacg::metacg
  ALIAS
  metacg
)

include(GenerateExportHeader)
generate_export_header(
  metacg
  BASE_NAME
  metacg
  EXPORT_FILE_NAME
  export/metacg/metacg_export.hpp
)

set_target_properties(
  metacg
  # XXX Is default visibility sensible?
  PROPERTIES CXX_VISIBILITY_PRESET default
             VISIBILITY_INLINES_HIDDEN YES
             VERSION "${PROJECT_VERSION}"
             SOVERSION "${PROJECT_VERSION_MAJOR}"
             EXPORT_NAME metacg
             OUTPUT_NAME metacg
)

target_include_directories(
  metacg PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(installRules)
endif()

add_config_include(metacg)

add_json(metacg)
add_spdlog_libraries(metacg)

if(METACG_BUILD_UNIT_TESTS)
  add_subdirectory(test/unit)
endif()

add_subdirectory(test/integration/CallgraphMerge)
