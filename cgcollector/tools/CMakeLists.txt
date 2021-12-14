set(PROJECT_NAME collector-tools)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)


add_executable(cgcollector CGCollector.cpp)
add_executable(cgmerge CGMerge.cpp)
add_executable(cgvalidate CGValidate.cpp)


# register_to_clang_tidy(cgcollector)
add_collector_include(cgcollector)
add_collector_lib(cgcollector)
add_config_include(cgcollector)
add_json(cgcollector)
default_compile_options(cgcollector)


add_collector_include(cgmerge)
add_collector_lib(cgmerge)
add_config_include(cgmerge)
add_json(cgmerge)


add_collector_include(cgvalidate)
add_cube_includes(cgvalidate)
add_cube_libraries(cgvalidate)
add_config_include(cgvalidate)
add_json(cgvalidate)
add_cxxopts(cgvalidate)
add_clang(cgvalidate)
if (LOGLEVEL)
    target_compile_definitions(cgvalidate PRIVATE LOGLEVEL=${LOGLEVEL})
endif ()

install(
        TARGETS cgcollector cgmerge cgvalidate
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
