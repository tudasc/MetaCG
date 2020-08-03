set(PROJECT_NAME collector-tools)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)


add_executable(cgcollector
  CGCollector.cpp
)

add_executable(cgmerge
  CGMerge.cpp
)

add_executable(cgverify
  CGVerify.cpp
)

add_cube_includes(cgverify)
add_cube_libraries(cgverify)

# register_to_clang_tidy(cgcollector)

add_collector_include(cgcollector)
add_collector_lib(cgcollector)

default_compile_options(cgcollector)


install(
  TARGETS cgcollector cgmerge cgverify
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

install(PROGRAMS
	${CMAKE_CURRENT_SOURCE_DIR}/cgc
	DESTINATION bin
)
