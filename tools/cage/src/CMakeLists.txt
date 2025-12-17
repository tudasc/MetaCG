include(AddLLVM)

add_llvm_pass_plugin(cage-plugin Plugin.cpp)
target_include_directories(cage-plugin PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cage/include>)
target_link_libraries(cage-plugin PRIVATE cage)

add_subdirectory(generator)
