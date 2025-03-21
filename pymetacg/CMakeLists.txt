
include(nanobind)

nanobind_add_module(pymetacg pymetacg.cpp)

add_metacg_library(pymetacg)
