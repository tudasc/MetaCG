# Now simply link against gtest or gtest_main as needed. Eg
add_executable(
  pgistests
  CallgraphTest.cpp
  # CallgraphManagerTest.cpp
  IPCGEstimatorPhaseTest.cpp
  LegacyMCGReaderTest.cpp
  loadImbalance/LIConfigTest.cpp
  loadImbalance/LIEstimatorPhaseTest.cpp
  loadImbalance/LIMetricTest.cpp
  PIRAPGISMetadataTest.cpp
)

add_googletest_libraries(pgistests)
add_pgis(pgistests)
add_metacg(pgistests)

# add_library(ipcg) target_link_libraries(pgistests)

add_test(NAME cgnode_test COMMAND pgistests)
