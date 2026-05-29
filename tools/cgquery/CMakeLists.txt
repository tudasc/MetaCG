set(PROJECT_NAME CGQuery)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(
  cgquery
  src/CGQuery.cpp
  src/commands/DomCommand.cpp
  src/commands/PostDomCommand.cpp
  src/commands/ReachesCommand.cpp
)

target_include_directories(cgquery PRIVATE include/)

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
