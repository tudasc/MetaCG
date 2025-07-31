set(PROJECT_NAME CallgraphMergeIntegrationTest)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_library(MergeMD SHARED MergeCountMD.cpp)
add_metacg(MergeMD)

add_executable(mergetester MergeTester.cpp)
target_include_directories(mergetester PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(mergetester PUBLIC metacg::metacg)

add_config_include(mergetester)

add_test(NAME mergeTests COMMAND mergetester)
