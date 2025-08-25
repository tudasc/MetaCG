add_library(cage-plugin MODULE Plugin.cpp)
target_include_directories(cage-plugin PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/cage/include>)
target_include_directories(cage-plugin PRIVATE $<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>)
target_link_libraries(cage-plugin PRIVATE cage)

add_subdirectory(generator)
