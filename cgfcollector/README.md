# CG Fortran Collector

Fortran call graph generation tool for MetaCG. This tool is implemented as a
Flang plugin and generates a call graph from source-level.

## Usage

For single file projects or projects not using modules use:

```sh
cgfcollector_comp_wrapper.sh [flang options, source file/s, plugin options]
```

For any other projects you need a build system. That calls `cgfcollector_comp_wrapper.sh`
and `cgfcollector_link_wrapper.sh` accordingly. For examples [see](#generate-a-call-graph).

You can also run the plugin directly with Flang:

```sh
flang -fc1 -load "libcgfcollector.so" -plugin "genCG" [options]
```

Available options:

- `--dot`: Additionally generate a DOT file. This is mostly used for debugging.
- `--verbose`: Print additional information during the generation process.
- `--graph-name`: Set internal graph name.
- `--include-intrinsics`: Include intrinsic procedures in the call graph.

Additionally these other tools are included:

- `cgfcollector_comp_wrapper.sh`: Acts like a normal Flang compiler but also generates a call graph.
- `cgfcollector_link_wrapper`: Acts like a normal Flang linker but also merges
  the generated call graphs.
- `test_runner.sh`: Run tests.

## How to build

To build the cgfcollector the option `METACG_BUILD_CGFCOLLECTOR` must be set to
`ON`.

## Generate a call graph

### from a CMake project

Paste this into your CMakeLists.txt.

```
set(CMAKE_Fortran_COMPILER "flang-new")
set(CMAKE_Fortran_COMPILER_LAUNCHER <path to cgfcollector_comp_wrapper.sh>)
set(CMAKE_Fortran_LINKER_LAUNCHER "<path to cgfcollector_link_wrapper.sh>")
```

This will hook into the CMake build process and generate a call graph.

An example can be found in `test/multi/deps`.

### from other projects

Use `cgfcollector_comp_wrapper.sh` and `cgfcollector_link_wrapper.sh` to hook into the
build process of your favorite tool.

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
