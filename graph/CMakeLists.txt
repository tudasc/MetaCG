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
)

add_library(metacg SHARED ${MetacgGraphLibSources})

message("Source dir for MetaCG lib: " ${PROJECT_SOURCE_DIR})

target_include_directories(metacg PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)

target_include_directories(metacg PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pgis/lib/include>)

add_json(metacg)
add_spdlog_libraries(metacg)
add_extrap(metacg)

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
