set(PROJECT_NAME collector-tests)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)


add_executable(cgsimpletester
	CGSimpleTester.cpp
)

register_to_clang_tidy(cgsimpletester)

default_compile_options(cgsimpletester)


install(
  TARGETS cgsimpletester
  EXPORT ${TARGETS_EXPORT_NAME}
	RUNTIME DESTINATION bin
)

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  DESTINATION lib/cmake
)
