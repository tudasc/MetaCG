# CG fortran collector

## Usage

`cgfcollector_wrapper.sh` convenience wrapper to run parse plugin.

### Generate a callgraph from a CMake project

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

## Running test

run `test_runner.sh`

## Debug

### print parse tree

```sh
flang-new -fc1 -fdebug-dump-parse-tree file.f90
```

### Grammar

[Grammar](https://flang.llvm.org/docs/f2018-grammar.html)
