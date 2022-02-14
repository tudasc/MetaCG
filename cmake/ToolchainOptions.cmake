# LLVM related stuff (XXX Do we actually need LLVM?)
find_package(
  LLVM
  REQUIRED
  CONFIG
)

# Clang related stuff
find_package(
  Clang
  REQUIRED
  CONFIG
)

# Cube library installation
find_library(CUBE_LIB cube4)
find_path(CUBE_INCLUDE cube)

# Extra-P installation directory for library
find_library(EXTRAP_LIB extrap)
# Extra-P source tree for includes (they are not installed with extra-p)
find_path(EXTRAP_INCLUDE extrap)

include(json)
include(cxxopts)

# External dependencies
function(add_clang target)
  # clang specified as system lib to suppress all warnings from clang headers
  target_include_directories(${target} SYSTEM PUBLIC ${CLANG_INCLUDE_DIRS})

  target_link_libraries(${target} clangTooling)
endfunction()

function(add_extrap target)
  target_include_directories(${target} SYSTEM PUBLIC ${EXTRAP_INCLUDE})
  target_link_directories(
    ${target}
    PUBLIC
    ${EXTRAP_LIB}
  )
  target_link_libraries(${target} extrap)
endfunction()

function(add_spdlog_libraries target)
  target_link_libraries(${target} spdlog::spdlog)
endfunction()

function(add_cube target)
  target_include_directories(${target} SYSTEM PUBLIC ${CUBE_INCLUDE})
  target_link_directories(
    ${target}
    PUBLIC
    ${CUBE_LIB}
  )
  target_link_libraries(${target} cube4)
endfunction()

# Internal dependencies
function(add_metacg target)
  add_metacg_library(${target})
  add_graph_includes(${target})
endfunction()

function(add_pgis target)
  add_pgis_includes(${target})
  add_pgis_library(${target})
endfunction()

function(add_mcg target)
  add_mcg_includes(${target})
  add_mcg_library(${target})
endfunction()

function(add_pgis_includes target)
  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pgis/lib/include>)
endfunction()

function(add_graph_includes target)
  message("Project source dir within add_graph_include: " ${PROJECT_SOURCE_DIR})

  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)
endfunction()

function(add_mcg_includes target)
  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pgis/lib/include/libIPCG>)
endfunction()

function(add_pgis_library target)
  target_link_libraries(${target} pgis)
endfunction()

function(add_mcg_library target)
  target_link_libraries(${target} mcg)
endfunction()

function(add_metacg_library target)
  target_link_libraries(${target} metacg)
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

# Download and unpack spdlog at configure time
configure_file(cmake/spdlog.cmake.in spdlog-download/CMakeLists.txt)
execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE spdresult
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spdlog-download
)
if(spdresult)
  message(FATAL_ERROR "CMake step for spdlog failed: ${spdresult}")
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE spdresult
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spdlog-download
)
if(spdresult)
  message(FATAL_ERROR "Build step for spdlog failed: ${spdresult}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(spdlog_force_shared_crt
    ON
    CACHE BOOL
          ""
          FORCE
)

# Add spdlog directly to our build.
add_subdirectory(
  ${CMAKE_CURRENT_BINARY_DIR}/spdlog-src
  ${CMAKE_CURRENT_BINARY_DIR}/spdlog-build
  EXCLUDE_FROM_ALL
)

install(TARGETS spdlog DESTINATION lib)
