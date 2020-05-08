[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# Profile Guided Instrumentation Selection

This tool is part of the PIRA toolchain.
It is used as analysis engine and constructs instrumentation selections guided by both static and dynamic information from a target application.


## Building PGIS

### Requirements

The tool builds using CMake and comes with multiple dependencies, which are downloaded and built automatically, when using the `build_submodules.sh` script.

- cxxopts
- json
- Extra-P
- CUBE library
- PyQt5
- matplotlib

### Building w/ Provided Script

To build PGIS using the provided script, follow

```{.sh}
$> ./build_submodules.sh
$> mkdir build && cd build
```

This will build and install the required software into `./deps/src` and `./deps/install`, respectively.
To use PGIS, please add the locations to `LD_LIBRARY_PATH` as needed.

### Building Manually

```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/pgis-test-install -DCXXOPTS_INCLUDE=../deps/src/cxxopts -DJSON_INCLUDE=../deps/src/json/single_include -DCUBE_LIB=$(dirname $(which cube_info))/../lib -DCUBE_INCLUDE=$(dirname $(which cube_info))/../include/cubelib -DEXTRAP_INCLUDE=../deps/src/extrap/extrap-3.0/include -DEXTRAP_LIB=../deps/install/extrap/lib -DSPDLOG_BUILD_SHARED=ON ..
```

## Using PGIS

For examples how to use PGIS, see the `test/integration` folder.
The static call-graph input data is constructed using the tool *CGCollector*.
The dynamic profiling information is passed to PGIS in CUBE files, which are typically constructed with the measurement system Score-P.

