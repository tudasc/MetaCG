
include(nanobind)
nanobind_build_library(nanobind-static)

# nanobind_add_module(pymetacg NOSTRIP pymetacg.cpp)
# We use nanobind's low-level API, as the high-level API
# sets the default visibility to `hidden` which breaks
# our singleton implementations in MetaCG.
# By using the low-level API and *not* calling
# `nanobind_set_visibility`, we can prevent this.
add_library(pymetacg MODULE pymetacg.cpp)
target_link_libraries(pymetacg PRIVATE nanobind-static)
nanobind_extension(pymetacg)
nanobind_compile_options(pymetacg)
nanobind_link_options(pymetacg)
nanobind_strip(pymetacg)
nanobind_disable_stack_protector(pymetacg)

add_metacg_library(pymetacg)
add_spdlog_libraries(pymetacg)
