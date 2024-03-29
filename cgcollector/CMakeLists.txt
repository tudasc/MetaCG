set(PROJECT_NAME CGCollector)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

include(cxxopts)

# include directories
function(add_collector_include target)
  target_include_directories(
    ${target} PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/cgcollector/lib/include>
                     $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )
endfunction()

function(add_collector_lib target)
  target_link_libraries(${target} PUBLIC collector)
endfunction()

# Compile flags
function(default_compile_options target)
  cmake_parse_arguments(
    ARG
    ""
    ""
    "PRIVATE_FLAGS;PUBLIC_FLAGS"
    ${ARGN}
  )

  target_compile_options(
    ${target}
    PRIVATE -Wall
            -Wextra
            -pedantic
            -Wunreachable-code
            -Wwrite-strings
            -Wpointer-arith
            -Wcast-align
            -Wcast-qual
            -fno-rtti
  )

  if(ARG_PRIVATE_FLAGS)
    target_compile_options(${target} PRIVATE "${ARG_PRIVATE_FLAGS}")
  endif()

  if(ARG_PUBLIC_FLAGS)
    target_compile_options(${target} PUBLIC "${ARG_PUBLIC_FLAGS}")
  endif()
endfunction()

option(
  DEBUG_TEST_AA
  "Developer option for testing the new Alias Analysis"
  OFF
)
if(DEBUG_TEST_AA)
  add_compile_definitions(DEBUG_TEST_AA)
endif()

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)
