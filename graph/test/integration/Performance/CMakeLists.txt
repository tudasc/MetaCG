set(PROJECT_NAME PerfTest)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(perftest PerfTester.cpp)
target_link_libraries(perftest PUBLIC metacg::metacg)

add_json(perftest)
add_spdlog_libraries(perftest)
add_config_include(perftest)
