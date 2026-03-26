# CG Fortran collector

Fortran call graph generation tool for MetaCG. This tool is implemented as a
Flang plugin and generates a call graph from source-level.

## Usage

For single file projects or projects not using modules use:

```sh
cgfcollector_wrapper.sh [options] <source file/s>
```

For any other projects you need a build system. For this we provide another
script that acts as the normal Flang compiler but also produces a call graph.
[More info below](#generate-a-call-graph).

```sh
cgfcollector_comp_wrapper.sh <source file/s>
```

You can also run the plugin directly with Flang:

```sh
flang -fc1 -load "libcgfcollector.so" -plugin "genCG" [options]
```

Available options:

- `--dot`: Additionally generate a DOT file. This is mostly used for debugging.
- `--no-rename`: Do not rename the output file.
- `--verbose`: Print additional information during the generation process.
- `--graph-name`: Set internal graph name

Additionally these other tools are included:

- `cgfcollector_wrapper.sh`: Convenience wrapper to run parse plugin.
- `cgfcollector_comp_wrapper.sh`: Acts like a normal Flang compiler but also generates a call graph.
- `test_runner.sh`: Run tests.

## How to build

To build the cgfcollector the option `METACG_BUILD_CGFCOLLECTOR` must be set to
`ON`.

## Generate a call graph

### from a CMake project

Paste this into your CMakeLists.txt.

```
set(CMAKE_Fortran_COMPILER <path to cgfcollector_wrapper.sh>)
set(CMAKE_Fortran_FLAGS "")
set(CMAKE_Fortran_COMPILE_OBJECT "<CMAKE_Fortran_COMPILER> --dot <DEFINES> <INCLUDES> <FLAGS> <SOURCE> -o <OBJECT>")
set(CMAKE_Fortran_LINK_EXECUTABLE "<path to cgmerge2> <TARGET> <OBJECTS>")
set(CMAKE_EXECUTABLE_SUFFIX .mcg)
```

This will hook into the CMake build process and generate a call graph instead of
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

Run `test_runner.sh`

NOTE: The test `test/multi/fortdepend_deps` has a dependency on [fortdepend](https://fortdepend.readthedocs.io/en/latest/)

## Debugging

### print parse tree

```sh
flang-new -fc1 -fdebug-dump-parse-tree file.f90
```
