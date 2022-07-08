set(PROJECT_NAME graphlib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MetacgGraphLibSources
    src/CgNode.cpp
    src/Callgraph.cpp
    src/MCGReader.cpp
    include/MCGReader.h
    include/CgNode.h
    include/Callgraph.h
    include/CgNodePtr.h
    include/MetaData.h
    include/MCGManager.h
    include/MCGWriter.h
    src/MCGWriter.cpp
    include/Util.h
    include/MCGBaseInfo.h
    include/LoggerUtil.h
    include/DotIO.h
    src/DotIO.cpp
    include/Timing.h
)

add_library(metacg SHARED ${MetacgGraphLibSources})

message("Source dir for MetaCG lib: " ${PROJECT_SOURCE_DIR})

target_include_directories(metacg PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)

add_config_include(metacg)

add_json(metacg)
add_spdlog_libraries(metacg)
# add_extrap(metacg)

install(
  TARGETS metacg
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)

# Add the directory containing unit tests
add_subdirectory(test/unit)
