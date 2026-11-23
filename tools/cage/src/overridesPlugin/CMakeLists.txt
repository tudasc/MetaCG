include(GNUInstallDirs)

add_library(cage-overrides-plugin MODULE OverridesPlugin.cpp)
# Don't link clang/LLVM: the plugin uses the symbols of the clang that loads it
target_include_directories(cage-overrides-plugin SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_project_compile_options(cage-overrides-plugin)

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(cage-overrides-plugin PRIVATE -fno-rtti)
endif()

install(TARGETS cage-overrides-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
