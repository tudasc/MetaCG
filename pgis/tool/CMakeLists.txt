set(PROJECT_NAME pgoe_estimator)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-targets)
set(CMAKE_VERBOSE_MAKEFILE ON)


add_executable(pgis_pira
	PiraMain.cpp
)

add_executable(pgis_stmt_printer
	StmtPrinterMain.cpp
)

add_pgoe_includes(pgis_pira)
add_pgoe_library(pgis_pira)
add_ipcg_includes(pgis_pira)
add_ipcg_library(pgis_pira)
add_cxxopts_includes(pgis_pira)
add_extrap_libraries(pgis_pira)
add_cube_libraries(pgis_pira)
add_logger_libraries(pgis_pira)

add_pgoe_includes(pgis_stmt_printer)
add_pgoe_library(pgis_stmt_printer)
add_ipcg_includes(pgis_stmt_printer)
add_ipcg_library(pgis_stmt_printer)
add_extrap_libraries(pgis_stmt_printer)
add_cube_libraries(pgis_stmt_printer)
add_logger_libraries(pgis_stmt_printer)



install(
  TARGETS pgis_pira pgis_stmt_printer 
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
