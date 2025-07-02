set(PROJECT_NAME CGConvert)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(cgconvert CGConvert.cpp)

add_metacg(cgconvert)

install(
  TARGETS cgconvert
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

# TODO: Move to tools Cmakelists?
configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
