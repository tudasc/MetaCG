add_library(FileInfoPlugin SHARED FileInfoMetadataPlugin.cpp FileInfoMetadata.cpp)
add_clang(FileInfoPlugin)
add_graph_includes(FileInfoPlugin)
add_spdlog_libraries(FileInfoPlugin)
add_json(FileInfoPlugin)
target_include_directories(FileInfoPlugin PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cgcollector2/include> )
set_target_properties(FileInfoPlugin PROPERTIES PREFIX ""
        SUFFIX ".collector"
        )
