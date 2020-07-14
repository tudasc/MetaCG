set(PROJECT_NAME collector-tools)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)


add_executable(cgcollector
  CGCollector.cpp
)

add_executable(cgmerge
  CGMerge.cpp
)

register_to_clang_tidy(cgcollector)

add_collector_include(cgcollector)
add_collector_lib(cgcollector)

default_compile_options(cgcollector)


install(
  TARGETS cgcollector cgmerge
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
