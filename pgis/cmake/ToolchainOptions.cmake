# Cube library installation
find_library(CUBE_LIB cube4)
find_path(CUBE_INCLUDE cube)
# JSON library source tree
find_path(JSON_INCLUDE json)
# Extra-P installation directory for library
find_library(EXTRAP_LIB extrap)
# Extra-P source tree for includes (they are not installed with extra-p)
find_path(EXTRAP_INCLUDE extrap)
# This is the command line parser library
find_path(CXXOPTS_INCLUDE cxxopts)


function(add_extrap_includes target)
  target_include_directories(${target} PUBLIC
    ${EXTRAP_INCLUDE}
  )
endfunction()

function(add_extrap_libraries target)
  target_link_libraries(${target}
    extrap
  )
endfunction()

function(add_logger_libraries target)
	target_link_libraries(${target} 
		spdlog::spdlog
	)
endfunction()

function(add_cube_includes target)
	target_include_directories(${target} PUBLIC
		${CUBE_INCLUDE}
	)
endfunction()

function(add_cxxopts_includes target)
    target_include_directories(${target} PUBLIC
        ${CXXOPTS_INCLUDE}/include
    )
endfunction()

function(add_cube_libraries target)
	target_link_libraries(${target}
		cube4
	)
endfunction()

function(add_json_parser target)
	target_include_directories(${target} PUBLIC
		${JSON_INCLUDE}
	)
endfunction()

function(target_project_compile_options target)
  cmake_parse_arguments(ARG "" "" "PRIVATE_FLAGS;PUBLIC_FLAGS" ${ARGN})

  target_compile_options(${target} PRIVATE
    -Wall -Wextra -pedantic
    -Wunreachable-code -Wwrite-strings
    -Wpointer-arith -Wcast-align
    -Wcast-qual -Wno-unused-parameter
    -Werror
  )

  if(ARG_PRIVATE_FLAGS)
    target_compile_options(${target} PRIVATE
      "${ARG_PRIVATE_FLAGS}"
    )
  endif()

  if(ARG_PUBLIC_FLAGS)
    target_compile_options(${target} PUBLIC
      "${ARG_PUBLIC_FLAGS}"
    )
  endif()
endfunction()

function(target_project_compile_definitions target)
  cmake_parse_arguments(ARG "" "" "PRIVATE_DEFS;PUBLIC_DEFS" ${ARGN})

  target_compile_definitions(${target} PRIVATE
    LOG_LEVEL=${LOG_LEVEL}
  )

  if(ARG_PRIVATE_DEFS)
    target_compile_definitions(${target} PRIVATE
      "${ARG_PRIVATE_DEFS}"
    )
  endif()

  if(ARG_PUBLIC_DEFS)
    target_compile_definitions(${target} PUBLIC
      "${ARG_PUBLIC_DEFS}"
    )
  endif()
endfunction()


function(add_pgoe_includes target)
	target_include_directories(${target}
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/include>
	)
endfunction()

function(add_ipcg_includes target)
	target_include_directories(${target}
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/include/libIPCG>
	)
endfunction()


function(add_pgoe_library target)
  target_link_libraries(${target}
    pgoelib
  )
endfunction()

function(add_ipcg_library target)
  target_link_libraries(${target}
		ipcg
  )
endfunction()

## Package cxxopts (from https://github.com/jarro2783/cxxopts.git)
configure_file(${CXXOPTS_INCLUDE}/cxxopts-config.cmake.in cxxopts-ext/CMakeLists.txt)


# Download and unpack googletest at configure time
configure_file(cmake/GoogleTest.cmake.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()



# Download and unpack googletest at configure time
configure_file(cmake/spdlog.cmake.in spdlog-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE spdresult
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spdlog-download )
if(spdresult)
  message(FATAL_ERROR "CMake step for spdlog failed: ${spdresult}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE spdresult
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spdlog-download)
if(spdresult)
  message(FATAL_ERROR "Build step for spdlog failed: ${spdresult}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(spdlog_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/spdlog-src
                 ${CMAKE_CURRENT_BINARY_DIR}/spdlog-build
                 EXCLUDE_FROM_ALL)

install(
	TARGETS spdlog
	DESTINATION lib
	)


