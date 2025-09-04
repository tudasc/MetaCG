add_executable(metacg-config MCGConfig.cpp)
add_config_include(metacg-config)
add_cxxopts(metacg-config)


target_compile_definitions(metacg-config PRIVATE INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
if (${METACG_BUILD_GRAPH_TOOLS})
    # TODO: Currently only uses build variables
    target_compile_definitions(metacg-config PRIVATE HAVE_GRAPH_TOOLS CAGE_PLUGIN="$<TARGET_FILE:cage-plugin>")
endif()


install(TARGETS metacg-config DESTINATION ${CMAKE_INSTALL_BINDIR})
