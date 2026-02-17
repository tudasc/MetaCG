set(PROJECT_NAME cgfcollector)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

file(
  GLOB
  CGFCOLLECTOR_SOURCES
  src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${CGFCOLLECTOR_SOURCES})
add_flang(${PROJECT_NAME})
add_metacg(${PROJECT_NAME})
add_spdlog_libraries(${PROJECT_NAME})

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

# visuel program to generate dot file from graph
add_executable(${PROJECT_NAME}-visuel ${CMAKE_CURRENT_SOURCE_DIR}/tools/visuel.cpp)
add_metacg(${PROJECT_NAME}-visuel)
add_spdlog_libraries(${PROJECT_NAME}-visuel)
install(
  TARGETS ${PROJECT_NAME}-visuel
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION bin
)

# generate wrapper scripts

function(
  configure_file_and_install
  TEMPLATE
  OUTPUT_BASENAME
  DESTINATION
)
  set(BUILD_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_BASENAME}")
  set(INSTALL_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_BASENAME}.install")

  # build-time values for development
  set(CGFCOLLECTOR_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.so")
  set(CGFCOLLECTOR_CGDIFF "${CMAKE_BINARY_DIR}/tools/cgdiff/cgdiff")
  set(CGFCOLLECTOR_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/cgfcollector_wrapper.sh")
  set(CGFCOLLECTOR_TEST_CASES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
  set(CGMERGE2_EXECUTABLE "${CMAKE_BINARY_DIR}/tools/cgmerge2/cgmerge2")

  configure_file(
    "${TEMPLATE}"
    "${BUILD_OUTPUT}"
    @ONLY
  )

  if(DESTINATION
     STREQUAL
     ""
  )
    return()
  endif()

  # install-time values
  set(CGFCOLLECTOR_FILE_NAME "${CMAKE_INSTALL_PREFIX}/lib/lib${PROJECT_NAME}.so")

  configure_file(
    "${TEMPLATE}"
    "${INSTALL_OUTPUT}"
    @ONLY
  )

  install(
    PROGRAMS "${INSTALL_OUTPUT}"
    DESTINATION "${DESTINATION}"
    RENAME "${OUTPUT_BASENAME}"
  )
endfunction()

configure_file_and_install(
  "${CMAKE_CURRENT_SOURCE_DIR}/tools/cgfcollector_wrapper.sh.in"
  "cgfcollector_wrapper.sh"
  "bin"
)

configure_file_and_install(
  "${CMAKE_CURRENT_SOURCE_DIR}/tools/cgfcollector_comp_wrapper.sh.in"
  "cgfcollector_comp_wrapper.sh"
  "bin"
)

configure_file_and_install(
  "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_runner.sh.in"
  "test_runner.sh"
  ""
)

configure_file_and_install(
  "${CMAKE_CURRENT_SOURCE_DIR}/test/multi/cmake_base.txt.in"
  "cmake_base.txt"
  ""
)
file(
  RENAME
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_base.txt"
  "${CMAKE_CURRENT_SOURCE_DIR}/test/multi/cmake_base.txt"
)

configure_file_and_install(
  "${CMAKE_CURRENT_SOURCE_DIR}/test/multi/make_base.in"
  "make_base"
  ""
)
file(
  RENAME
  "${CMAKE_CURRENT_BINARY_DIR}/make_base"
  "${CMAKE_CURRENT_SOURCE_DIR}/test/multi/make_base"
)

# tests
add_test(NAME cgfcollector_tests COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_runner.sh)
