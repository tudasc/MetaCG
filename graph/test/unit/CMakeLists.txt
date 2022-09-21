# Now simply link against gtest or gtest_main as needed.Eg
add_executable(
  libtests
  CgNodeTest.cpp
  DotIOTest.cpp
  MCGManagerTest.cpp
  MCGReaderTest.cpp
  MCGWriterTest.cpp
  ReachabilityAnalysisTest.cpp
  UtilTest.cpp
)

target_include_directories(libtests PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)
target_link_libraries(libtests gtest_main)

# add_pgis(libtests)
add_json(libtests)
add_spdlog_libraries(libtests)
add_config_include(libtests)

# add_library(ipcg)
target_link_libraries(libtests metacg)

add_test(NAME libraryTests COMMAND libtests)
