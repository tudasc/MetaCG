include(FetchContent)
FetchContent_Declare(
  googletest URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
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
