include(CMakeFindDependencyMacro)

find_dependency(Threads)
find_dependency(nlohmann_json)

include("${CMAKE_CURRENT_LIST_DIR}/metacgTargets.cmake")
