set(PROJECT_NAME fcollector)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

file(
  GLOB
  FCOLLECTOR_SOURCES
  src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${FCOLLECTOR_SOURCES})
add_flang(${PROJECT_NAME})
add_metacg(${PROJECT_NAME})
add_spdlog_libraries(${PROJECT_NAME})
# add_json(${PROJECT_NAME})

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

configure_package_config_file(
  ${METACG_Directory}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake)
