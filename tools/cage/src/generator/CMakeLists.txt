add_library(cage STATIC CallGraphGenerator.cpp FileExporter.cpp)
set_target_properties(cage PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(cage PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cage/include>)
target_include_directories(cage PUBLIC $<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>)

if(${CAGE_USE_METAVIRT})
  target_link_libraries(cage PRIVATE metavirt::Types)
  target_compile_definitions(cage PRIVATE "HAVE_METAVIRT")
endif()

target_link_libraries(cage PUBLIC metacg::metacg)
add_config_include(cage)
