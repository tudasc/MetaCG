set(PROJECT_NAME CGQuery)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(cgquery CGQuery.cpp)

add_metacg(cgquery)
add_cxxopts(cgquery)

install(
  TARGETS cgquery
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
