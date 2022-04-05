# Now simply link against gtest or gtest_main as needed. Eg
add_executable(
  pgistests
  CallgraphTest.cpp
  # CallgraphManagerTest.cpp
  CgNodeTest.cpp
  IPCGAnnotation.cpp
  IPCGEstimatorPhaseTest.cpp
  loadImbalance/LIConfigTest.cpp
  loadImbalance/LIEstimatorPhaseTest.cpp
  loadImbalance/LIMetaDataTest.cpp
  loadImbalance/LIMetricTest.cpp
)

target_link_libraries(pgistests gtest_main)
add_pgis(pgistests)

# add_library(ipcg)
target_link_libraries(pgistests mcg)

add_test(NAME cgnode_test COMMAND pgistests)
