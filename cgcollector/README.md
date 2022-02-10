[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# CGCollector

Clang-based call-graph generation tool of the metacg tool.

## Build CGCollector

CGCollector requires Clang/LLVM version 10.
The software requires the **nlohmann/json** library (here)[github.com/nlohmann/json] and the cube library to read `.cubex` profiles for CG validation (here)[https://www.scalasca.org/software/cube-4.x/download.html].

```{.sh}
$> mkdir build && cd build
$> cmake -DCUBE_INCLUDE=$(cubelib-configPtr --include) -DJSON_LIBRARY=/path/to/json-library/single_include -DCMAKE_INSTALL_PREFIX=/where/it/should/live ..
$> make
$> make install
```

## Using CGCollector

It is easiest to apply CGCollector, when a `compilation_commands.json` database is present.

```{.sh}
$> cgc target.cpp
```

`cgc` is a wrapper script that (tries to) determines the paths to the Clang standard includes.
