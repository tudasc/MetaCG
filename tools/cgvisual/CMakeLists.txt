set(PROJECT_NAME CGVisual)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)
set(PROJECT_BINARY_NAME cgvisual)

add_executable(${PROJECT_BINARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/visual.cpp)

add_metacg(${PROJECT_BINARY_NAME})
add_spdlog_libraries(${PROJECT_BINARY_NAME})

install(
  TARGETS ${PROJECT_BINARY_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
