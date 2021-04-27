set(PROJECT_NAME collectorlib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

set(COLLECTORLIB_SOURCES
  src/helper/ASTHelper.cpp
  src/helper/common.cpp
  src/JSONManager.cpp
  src/CallgraphToJSON.cpp
  src/CallGraph.cpp
)

add_library(collectorlib SHARED
  ${COLLECTORLIB_SOURCES}
)

add_collector_include(collectorlib)
register_to_clang_tidy(collectorlib)

add_clang(collectorlib)
add_json(collectorlib)
add_config_include(collectorlib)

default_compile_options(collectorlib)

install(
  TARGETS collectorlib
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  DESTINATION lib/cmake
)
