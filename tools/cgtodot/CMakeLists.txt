set(PROJECT_NAME CGToDot)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)
set(PROJECT_BINARY_NAME cgtodot)

add_executable(${PROJECT_BINARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/CGToDot.cpp)

add_metacg(${PROJECT_BINARY_NAME})
add_cxxopts(${PROJECT_BINARY_NAME})
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

# tests

set(CGTODOT_BIN ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_BINARY_NAME})
set(CGTODOT_TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/data)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test/test_runner.sh.in" "test_runner.sh")

add_test(NAME cgtodot_tests COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_runner.sh)
