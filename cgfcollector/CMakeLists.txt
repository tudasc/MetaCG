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

target_precompile_headers(
  ${PROJECT_NAME}
  PRIVATE
  include/headers.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

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

# cgCompare program
add_executable(${PROJECT_NAME}-cgCompare test/cgCompare.cpp)
add_metacg(${PROJECT_NAME}-cgCompare)
add_spdlog_libraries(${PROJECT_NAME}-cgCompare)
install(
  TARGETS ${PROJECT_NAME}-cgCompare
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION bin
)

# generate wrapper script with build dir for easy of use during development
set(FCOLLECTOR_WRAPPER_SCRIPT_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cgfcollector_wrapper.sh.in")
set(FCOLLECTOR_WRAPPER_SCRIPT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cgfcollector_wrapper.sh")

# FCOLLECTOR_WRAPPER_EXPORT_LINE for debuging
option(
  FCOLLECTOR_ADD_BUILD_TO_LIB_PATH
  "Add build dir to LD_LIBRARY_PATH in wrapper script"
  OFF
)
set(FCOLLECTOR_WRAPPER_EXPORT_LINE "")
if(FCOLLECTOR_ADD_BUILD_TO_LIB_PATH)
  set(FCOLLECTOR_WRAPPER_EXPORT_LINE "export LD_LIBRARY_PATH=\"${CMAKE_CURRENT_BINARY_DIR}:\$LD_LIBRARY_PATH\"")
endif()

set(FCOLLECTOR_FILE_NAME "lib${PROJECT_NAME}.so")

configure_file(
  ${FCOLLECTOR_WRAPPER_SCRIPT_TEMPLATE}
  ${FCOLLECTOR_WRAPPER_SCRIPT_OUTPUT}
  @ONLY
)
install(PROGRAMS ${FCOLLECTOR_WRAPPER_SCRIPT_OUTPUT} DESTINATION bin)

set(FCOLLECTOR_TEST_SCRIPT_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/test/test_runner.sh.in")
set(FCOLLECTOR_TEST_SCRIPT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_runner.sh")

set(FCOLLECTOR_TEST_CASES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
set(FCOLLECTOR_WRAPPER "${FCOLLECTOR_WRAPPER_SCRIPT_OUTPUT}")
set(FCOLLECTOR_CG_COMPARE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-cgCompare")

configure_file(
  ${FCOLLECTOR_TEST_SCRIPT_TEMPLATE}
  ${FCOLLECTOR_TEST_SCRIPT_OUTPUT}
  @ONLY
)

install(PROGRAMS ${FCOLLECTOR_TEST_SCRIPT_OUTPUT} DESTINATION bin)
