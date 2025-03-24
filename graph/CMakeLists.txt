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

target_include_directories(metacg PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

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
