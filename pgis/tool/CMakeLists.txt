set(PROJECT_NAME PGIS_tools)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-targets)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(pgis_pira PGISMain.cpp)

add_pgis(pgis_pira)
add_cxxopts(pgis_pira)
add_cube(pgis_pira)
add_json(pgis_pira)
add_spdlog_libraries(pgis_pira)
add_metacg(pgis_pira)

install(
  TARGETS pgis_pira
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
