include(GNUInstallDirs)

add_library(cage-overrides-plugin SHARED OverridesPlugin.cpp)
add_clang(cage-overrides-plugin)
target_project_compile_options(cage-overrides-plugin)

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(cage-overrides-plugin PRIVATE -fno-rtti)
endif()

install(TARGETS cage-overrides-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
