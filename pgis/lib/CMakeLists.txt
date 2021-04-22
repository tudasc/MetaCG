set(PROJECT_NAME pgislib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

set(PGOE_LIB_SOURCES
  src/Callgraph.cpp
  src/CallgraphManager.cpp
  src/CgHelper.cpp
  src/CgNode.cpp
  src/CubeReader.cpp
  src/EstimatorPhase.cpp
  src/IPCGEstimatorPhase.cpp
  src/ExtrapConnection.cpp
  src/ExtrapEstimatorPhase.cpp

  src/loadImbalance/LIEstimatorPhase.cpp
  src/loadImbalance/metric/AbstractMetric.cpp
  src/loadImbalance/OnlyMainEstimatorPhase.cpp
  include/loadImbalance/metric/ImbalancePercentageMetric.h
  include/loadImbalance/metric/EfficiencyMetric.h
  include/loadImbalance/metric/VariationCoeffMetric.h
  src/loadImbalance/LIMetaData.cpp
  src/loadImbalance/LIRetriever.cpp
  src/loadImbalance/Config.cpp
)

add_library(pgis SHARED
  ${PGOE_LIB_SOURCES}
)

add_library(mcg SHARED
	src/libIPCG/MCGReader.cpp
  src/libIPCG/MetaDataHandler.cpp
)

add_pgoe_includes(pgis)
add_cube_includes(pgis)
add_cube_libraries(pgis)
add_extrap_includes(pgis)
add_extrap_libraries(pgis)
add_logger_libraries(pgis)
add_json_parser(pgis)
target_project_compile_options(pgis)

add_pgoe_includes(mcg)
add_json_parser(mcg)
add_mcg_includes(mcg)
add_extrap_includes(mcg)
add_logger_libraries(mcg)


install(
  TARGETS pgis mcg
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
