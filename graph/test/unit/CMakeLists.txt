# Now simply link against gtest or gtest_main as needed.Eg
add_executable(
  libtests
  DotIOTest.cpp
  MCGManagerTest.cpp
  ReachabilityAnalysisTest.cpp
  UtilTest.cpp
  VersionThreeMCGReaderTest.cpp
  VersionThreeMCGWriterTest.cpp
  VersionThreeReaderWriterRoundtripTest.cpp
  VersionTwoMCGReaderTest.cpp
  VersionTwoMCGWriterTest.cpp
  VersionTwoReaderWriterRoundtripTest.cpp
)

target_include_directories(libtests PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(libtests PUBLIC gtest_main)
target_link_libraries(libtests PUBLIC metacg::metacg)

add_json(libtests)
add_spdlog_libraries(libtests)
add_config_include(libtests)

add_test(NAME libraryTests COMMAND libtests)
