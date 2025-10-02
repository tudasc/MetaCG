add_executable(
    cgdifftests
    main.cpp
    TestNodeDiff.cpp
    TestNodeSummary.cpp
    TestGlobalMDDiff.cpp
)

target_include_directories(cgdifftests PRIVATE ../../include)

add_metacg(cgdifftests)
add_googletest_libraries(cgdifftests)

add_test(NAME cgdiff_unittests COMMAND cgdifftests)
