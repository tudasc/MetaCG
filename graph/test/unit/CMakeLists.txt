# Now simply link against gtest or gtest_main as needed.Eg
add_executable(
  libtests
  CGNodeTests.cpp
  DotIOTest.cpp
  LoggingTest.cpp
  MCGManagerTest.cpp
  ReachabilityAnalysisTest.cpp
  ReaderFactoryTest.cpp
  UtilTest.cpp
  VersionThreeMCGReaderTest.cpp
  VersionThreeMCGWriterTest.cpp
  VersionThreeReaderWriterRoundtripTest.cpp
  VersionTwoMCGReaderTest.cpp
  VersionTwoMCGWriterTest.cpp
  VersionTwoReaderWriterRoundtripTest.cpp
)

target_include_directories(libtests PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
add_googletest_libraries(libtests)
target_link_libraries(libtests PUBLIC metacg::metacg)

add_json(libtests)
add_spdlog_libraries(libtests)
add_config_include(libtests)

add_test(NAME libraryTests COMMAND libtests)
