set(PROJECT_NAME CGFormat)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

include(cxxopts)

add_executable(cgformat CGFormat.cpp)
add_metacg(cgformat)
add_cxxopts(cgformat)

install(
  TARGETS cgformat
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)

add_subdirectory(test)
