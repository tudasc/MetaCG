include(AddLLVM)

add_llvm_pass_plugin(cage-plugin CaGe.cpp)
target_include_directories(cage-plugin PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cage/include>)
target_link_libraries(cage-plugin PRIVATE cage)

add_library(cage-plugin-interface INTERFACE)
target_include_directories(
  cage-plugin-interface INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cage/include/cage/interface>
)
target_link_libraries(cage-plugin-interface INTERFACE metacg::metacg)

add_subdirectory(generator)
