# Now simply link against gtest or gtest_main as needed. Eg
add_executable(
  basictests
  CallgraphTest.cpp
  # CallgraphManagerTest.cpp
  CgNodeTest.cpp
  IPCGAnnotation.cpp
  IPCGEstimatorPhaseTest.cpp
  loadImbalance/LIConfigTest.cpp
  loadImbalance/LIEstimatorPhaseTest.cpp
  loadImbalance/LIMetaDataTest.cpp
  loadImbalance/LIMetricTest.cpp
  MCGManagerTest.cpp
  MCGReaderTest.cpp
  MCGWriterTest.cpp
)

target_link_libraries(basictests gtest_main)
add_pgis(basictests)

# add_library(ipcg)
target_link_libraries(basictests mcg)

add_test(NAME cgnode_test COMMAND basictests)
