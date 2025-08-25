add_executable(metacg-config MCGConfig.cpp)
add_config_include(metacg-config)
target_compile_definitions(metacg-config PRIVATE INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
add_cxxopts(metacg-config)

install(TARGETS metacg-config DESTINATION ${CMAKE_INSTALL_BINDIR})
