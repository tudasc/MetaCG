include(AddLLVM)

add_llvm_pass_plugin(cage-plugin CaGe.cpp)
# LLVM < 16 sets BUILD_WITH_INSTALL_RPATH on Linux, which drops the build-tree path to libmetacg
set_target_properties(cage-plugin PROPERTIES BUILD_WITH_INSTALL_RPATH OFF)
target_include_directories(cage-plugin PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cage/include>)
target_link_libraries(cage-plugin PRIVATE cage)

add_subdirectory(generator)
