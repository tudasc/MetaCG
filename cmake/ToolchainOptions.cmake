include(json)
include(spdlog)

# Internal dependencies
function(add_metacg target)
  add_metacg_library(${target})
  add_graph_includes(${target})
endfunction()

function(add_pgis target)
  add_pgis_includes(${target})
  add_pgis_library(${target})
endfunction()

function(add_config_include target)
  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)
endfunction()

function(add_pgis_includes target)
  target_include_directories(
    ${target} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pgis/lib/include>
                     $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pgis/lib/include/config>
  )
endfunction()

function(add_graph_includes target)
  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)
endfunction()

function(add_pgis_library target)
  target_link_libraries(${target} PUBLIC pgis)
endfunction()

function(add_metacg_library target)
  target_link_libraries(${target} PUBLIC metacg::metacg)
endfunction()

# Project compile options
function(target_project_compile_options target)
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
            -Wno-unused-parameter
  )

  if(ARG_PRIVATE_FLAGS)
    target_compile_options(${target} PRIVATE "${ARG_PRIVATE_FLAGS}")
  endif()

  if(ARG_PUBLIC_FLAGS)
    target_compile_options(${target} PUBLIC "${ARG_PUBLIC_FLAGS}")
  endif()
endfunction()

# Project compile definitions
function(target_project_compile_definitions target)
  cmake_parse_arguments(
    ARG
    ""
    ""
    "PRIVATE_DEFS;PUBLIC_DEFS"
    ${ARGN}
  )

  target_compile_definitions(${target} PRIVATE LOG_LEVEL=${LOG_LEVEL})

  if(ARG_PRIVATE_DEFS)
    target_compile_definitions(${target} PRIVATE "${ARG_PRIVATE_DEFS}")
  endif()

  if(ARG_PUBLIC_DEFS)
    target_compile_definitions(${target} PUBLIC "${ARG_PUBLIC_DEFS}")
  endif()
endfunction()

if(METACG_BUILD_UNIT_TESTS)
  # Download and unpack googletest at configure time
  configure_file(cmake/GoogleTest.cmake.in googletest-download/CMakeLists.txt)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
  )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
  )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker settings on Windows
  set(gtest_force_shared_crt
      ON
      CACHE BOOL
            ""
            FORCE
  )

  # Add googletest directly to our build. This defines the gtest and gtest_main targets.
  add_subdirectory(
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL
  )

  # The gtest/gtest_main targets carry header search path dependencies automatically when using CMake 2.8.11 or later.
  # Otherwise we have to add them here ourselves.
  if(CMAKE_VERSION
     VERSION_LESS
     2.8.11
  )
    include_directories("${gtest_SOURCE_DIR}/include")
  endif()
endif()
