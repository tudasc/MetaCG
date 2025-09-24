add_executable(
    cgdiffTests
    main.cpp
    TestNodeDiff.cpp
    TestNodeSummary.cpp
    TestGlobalMDDiff.cpp
)

target_include_directories(cgdiffTests PRIVATE ../../include)

add_metacg(cgdiffTests)
add_googletest_libraries(cgdiffTests)
