set(PROJECT_NAME CGValidate2)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(cgvalidate2 CGValidate2.cpp)

add_config_include(cgvalidate2)
add_metacg(cgvalidate2)
add_cxxopts(cgvalidate2)
add_cube(cgvalidate2)

install(
  TARGETS cgvalidate2
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
