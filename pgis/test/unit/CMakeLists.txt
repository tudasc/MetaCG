# Now simply link against gtest or gtest_main as needed. Eg
add_executable(basictests CgNodeTest.cpp
  CallgraphTest.cpp
  CallgraphManagerTest.cpp
  IPCGEstimatorPhaseTest.cpp
  IPCGAnnotation.cpp
  MCGReaderTest.cpp

  loadImbalance/LIMetricTest.cpp
  loadImbalance/LIEstimatorPhaseTest.cpp
  loadImbalance/LIMetaDataTest.cpp
  loadImbalance/LIConfigTest.cpp
  )

target_link_libraries(basictests gtest_main)
add_pgoe_includes(basictests)
add_pgoe_library(basictests)

# add_library(ipcg)
target_link_libraries(basictests mcg)

add_test(NAME cgnode_test COMMAND basictests)
