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
)

target_link_libraries(pgistests PUBLIC gtest_main)
add_pgis(pgistests)
add_metacg(pgistests)
add_json(pgistests)
add_spdlog_libraries(pgistests)

# add_library(ipcg) target_link_libraries(pgistests)

add_test(NAME cgnode_test COMMAND pgistests)
