function(add_mcgconfig target)
    cmake_parse_arguments(ARG "" "" "DEFINITIONS" ${ARGN})

    add_executable(${target} MCGConfig.cpp)
    add_config_include(${target})
    add_cxxopts(${target})
    target_compile_definitions(${target} PRIVATE
            INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
            ${ARG_DEFINITIONS})
endfunction()

# Build two versions of metacg-config: one to use inside the build tree (e.g. for tests) and one for the install tree,
# to be used by external tools.
set(MCGCONFIG_BUILD_DEFS "")
set(MCGCONFIG_INSTALL_DEFS "")

if (${BUILD_CAGE})
    list(APPEND MCGCONFIG_BUILD_DEFS HAVE_CAGE CAGE_PLUGIN="$<TARGET_FILE:cage-plugin>")
    list(APPEND MCGCONFIG_INSTALL_DEFS HAVE_CAGE
            CAGE_PLUGIN="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/$<TARGET_FILE_NAME:cage-plugin>")
endif()

add_mcgconfig(metacg-build-config DEFINITIONS ${MCGCONFIG_BUILD_DEFS})
add_mcgconfig(metacg-config DEFINITIONS ${MCGCONFIG_INSTALL_DEFS})

install(TARGETS metacg-config DESTINATION ${CMAKE_INSTALL_BINDIR})
