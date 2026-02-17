set(PROJECT_NAME CGDiff)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(cgdiff CGDiff.cpp)

target_include_directories(cgdiff PRIVATE include)

add_metacg(cgdiff)
add_cxxopts(cgdiff)

if(METACG_BUILD_UNIT_TESTS)
  add_subdirectory(test/unit)
endif()

install(
  TARGETS cgdiff
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
