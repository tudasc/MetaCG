# CG Fortran collector

## Usage

The plugin is compiled into a dynamic library and can be run with the
Flang compiler like so:

`flang -fc1 -load "build/cgfcollector/libfcollector.so" -plugin "genCG"`

There are three kinds of plugins:

- `genCG`: generates a call graph in the MetaCG json format.
- `genCGwithDot`: like `genCG` but also generate a `.dot` file. Mostly used for debugging.
- `genCGNoRename`: like `genCG` but does not rename the output file.

Additionally these other tools are included:

- `cgfcollector_wrapper.sh`: convenience wrapper to run parse plugin.
- `cgfcollector_comp_wrapper.sh`: acts like a normal Flang compiler but also generates a call graph.
- `cgCompare.cpp`: compares two given call graphs.
- `test_runner.sh`: run tests.

## How to build

To build the cgfcollector the option `METACG_BUILD_CGFCOLLECTOR` must be set to
`ON`.

## Generate a call graph

### from a CMake project

Paste this into your CMakeLists.txt.

```
set(CMAKE_Fortran_COMPILER <path to cgfcollector_wrapper.sh>)
set(CMAKE_Fortran_FLAGS "")
set(CMAKE_Fortran_COMPILE_OBJECT "<CMAKE_Fortran_COMPILER> -dot <DEFINES> <INCLUDES> <FLAGS> <SOURCE> -o <OBJECT>")
set(CMAKE_Fortran_LINK_EXECUTABLE "<path to cgmerge2> <TARGET> <OBJECTS>")
set(CMAKE_EXECUTABLE_SUFFIX .json)
```

This will hook into the cmake build process and generate a callgraph instead of
an executable.

An example can be found in `test/multi/deps`.

### from other projects

Use `cgfcollector_comp_wrapper.sh` or `cgfcollector_wrapper.sh` to hook into the
build process of your favorite tool and use the `cgmerge2` utility to merge the
partial generated call graphs.

An example can be found in `test/multi/fortdepend_deps`. This example uses
fortdepend to generate a module dependency list. If your build system already
generates such a list and executes the compiler on the files in the correct order
you probably don't need this.

## Running test

run `test_runner.sh`

NOTE: The test `test/multi/fortdepend_deps` has a dependency on [fortdepend](https://fortdepend.readthedocs.io/en/latest/)

## Debugging

### print parse tree

```sh
flang-new -fc1 -fdebug-dump-parse-tree file.f90
```
