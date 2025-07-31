include(nanobind)
include(spdlog)
include(json)

nanobind_build_library(nanobind-static)

# We use nanobind's low-level API, as the high-level API sets the default visibility to `hidden` which breaks our
# singleton implementations in MetaCG. By using the low-level API and *not* calling `nanobind_set_visibility`, we can
# prevent this.
add_library(pymetacg MODULE pymetacg.cpp)
target_link_libraries(pymetacg PRIVATE nanobind-static)
nanobind_extension(pymetacg)
nanobind_compile_options(pymetacg)
nanobind_link_options(pymetacg)
nanobind_strip(pymetacg)
nanobind_disable_stack_protector(pymetacg)

add_metacg_library(pymetacg)

if(NOT CMAKE_INSTALL_PYTHON_LIBDIR)
  set(CMAKE_INSTALL_PYTHON_LIBDIR
      "${CMAKE_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages"
  )
endif()

install(TARGETS pymetacg DESTINATION ${CMAKE_INSTALL_PYTHON_LIBDIR})

# Tests
if(METACG_BUILD_PYMETACG_TESTS)
  add_subdirectory(tests)
endif()
