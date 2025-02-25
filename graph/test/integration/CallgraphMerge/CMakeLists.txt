set(PROJECT_NAME CallgraphMergeIntegrationTest)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(mergetester MergeTester.cpp)
target_include_directories(mergetester PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(mergetester PUBLIC metacg::metacg)

add_json(mergetester)
add_spdlog_libraries(mergetester)
add_config_include(mergetester)

add_test(NAME mergeTests COMMAND mergetester)
