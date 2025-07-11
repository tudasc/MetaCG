set(PROJECT_NAME PerfTest)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(perftest PerfTester.cpp)
add_metacg(perftest)

add_config_include(perftest)
