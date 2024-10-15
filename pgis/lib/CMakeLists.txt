set(PROJECT_NAME pgislib)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(PGIS_LIB_SOURCES
    src/PiraMCGProcessor.cpp
    src/CgHelper.cpp
    src/CubeReader.cpp
    src/EstimatorPhase.cpp
    src/IPCGEstimatorPhase.cpp
    src/ExtrapConnection.cpp
    src/ExtrapEstimatorPhase.cpp
    src/config/ParameterConfig.cpp
    src/loadImbalance/LIEstimatorPhase.cpp
    src/loadImbalance/metric/AbstractMetric.cpp
    src/loadImbalance/OnlyMainEstimatorPhase.cpp
    include/loadImbalance/metric/ImbalancePercentageMetric.h
    include/loadImbalance/metric/EfficiencyMetric.h
    include/loadImbalance/metric/VariationCoeffMetric.h
    include/config/ParameterConfig.h
    include/config/GlobalConfig.h
    include/config/PiraIIConfig.h
    include/LegacyMCGReader.h
    src/loadImbalance/LIMetaData.cpp
    src/LegacyMCGReader.cpp
    include/MetaData/PGISMetaData.h
    src/LegacyEstimatorPhases.cpp
    include/ErrorCodes.h
)

add_library(pgis SHARED ${PGIS_LIB_SOURCES})

add_pgis_includes(pgis)
add_graph_includes(pgis)
add_metacg_library(pgis)
add_cube(pgis)
add_extrap(pgis)
add_spdlog_libraries(pgis)
add_json(pgis)
target_project_compile_options(pgis)

install(
  TARGETS pgis
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
