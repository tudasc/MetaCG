set(PROJECT_NAME pgoelib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

set(PGOE_LIB_SOURCES
  src/Callgraph.cpp
  src/CallgraphManager.cpp
  src/CgHelper.cpp
  src/CgLocation.cpp
  src/CgNode.cpp
  src/CubeReader.cpp
  src/EdgeBasedOptimumEstimatorPhase.cpp
  src/EstimatorPhase.cpp
  src/IPCGEstimatorPhase.cpp
  src/IPCGReader.cpp
  src/NodeBasedOptimumEstimatorPhase.cpp
  src/ProximityMeasureEstimatorPhase.cpp
  src/SanityCheckEstimatorPhase.cpp
  src/ExtrapConnection.cpp
  src/ExtrapEstimatorPhase.cpp)

add_library(pgoelib SHARED
  ${PGOE_LIB_SOURCES}
)

add_pgoe_includes(pgoelib)
add_cube_includes(pgoelib)
add_cube_libraries(pgoelib)
add_json_parser(pgoelib)
add_extrap_includes(pgoelib)
add_extrap_libraries(pgoelib)

add_logger_libraries(pgoelib)


install(
  TARGETS pgoelib
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
