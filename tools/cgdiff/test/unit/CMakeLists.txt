add_executable(
  cgdifftests
  main.cpp
  TestGlobalMDDiff.cpp
  TestNodeDiff.cpp
  TestNodeSummary.cpp
)

target_include_directories(cgdifftests PRIVATE ../../include)

add_metacg(cgdifftests)
add_googletest_libraries(cgdifftests)

add_test(NAME cgdiff_unittests COMMAND cgdifftests)
