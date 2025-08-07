include(FetchContent)
FetchContent_Declare(
  googletest URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

set(gtest_force_shared_crt
    ON
    CACHE BOOL
          ""
          FORCE
)

function(add_googletest_libraries target)
  target_link_libraries(${target} PUBLIC GTest::gtest_main)
endfunction()
