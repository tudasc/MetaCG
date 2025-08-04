add_executable(main main.cpp)

target_include_directories(main PRIVATE ../../include)


add_metacg(main)
add_googletest_libraries(main)
