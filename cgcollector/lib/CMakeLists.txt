set(PROJECT_NAME collectorlib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

set(COLLECTORLIB_SOURCES
    # cmake-format: sortable
    src/AAReferenceVisitor.cpp
    src/AAUSR.cpp
    src/AliasAnalysis.cpp
    src/CallGraph.cpp
    src/CallgraphToJSON.cpp
    src/GlobalCallDepth.cpp
    src/helper/ASTHelper.cpp
    src/helper/common.cpp
    src/JSONManager.cpp
)

add_library(collector SHARED ${COLLECTORLIB_SOURCES})

add_collector_include(collector)

add_clang(collector)
add_json(collector)
add_config_include(collector)

default_compile_options(collector)

install(
  TARGETS collector
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
