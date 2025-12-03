add_executable(cgdiff CGDiff.cpp)

target_include_directories(cgdiff PRIVATE include)

add_metacg(cgdiff)
add_cxxopts(cgdiff)

if(METACG_BUILD_UNIT_TESTS)
  add_subdirectory(test/unit)
endif()
