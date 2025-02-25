set(PROJECT_NAME CGMerge2)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_executable(cgmerge2 CGMerge2.cpp)

add_config_include(cgmerge2)
add_json(cgmerge2)
add_metacg(cgmerge2)
add_spdlog_libraries(cgmerge2)
add_clang(cgmerge2)

install(
  TARGETS cgmerge2
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

# TODO: Move to tools Cmakelists?
configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
