include(json)
include(spdlog)

if(METACG_BUILD_GRAPH_TOOLS OR METACG_BUILD_CGCOLLECTOR OR METACG_BUILD_PGIS)
  include(cxxopts-lib)
endif()

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
