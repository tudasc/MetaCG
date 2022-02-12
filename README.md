[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# MetaCG

MetaCG provides a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the json file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The package contains an experimental Clang-based tool for call-graph construction, and a converter for the output files of [Phasar](https://github.com/secure-software-engineering/phasar).
Also, the package contains the PGIS analysis tool, which is used as the analysis backend in [PIRA](https://github.com/tudasc/pira).

Once constructed, the graph can be serialized into a JSON format, which contains the following information:

```{.json}
{
 "_MetaCG: {
   "version": "2.0",
   "generator": {
    "name": "ToolName",
    "version": "ToolVersion"
    }
  },
  "_CG": {
   "main": {
    "callers": [],
    "callees": ["foo"],
    "hasBody": true,
    "isVirtual": false,
    "doesOverride": false,
    "overrides": [],
    "overriddenBy": [],
    "meta": {
     "MetaTool": {}
    }
  }
```

- *_MetaCG* contains information about the file itself, such as the file format version.
- *_CG* contains the actual serialized call graph. For each function, it lists
  - *callers*: A set of functions that this function may be called from.
  - *callees*: A set of functions that this function may call.
  - *hasBody*: Whether a definition of the function was found in the processed source files.
  - *isVirtual*: Whether the function is marked as virtual.
  - *doesOverride*: Whether this function overrides another function.
  - *overrides*: A set of functions that this function overrides.
  - *overriddenBy*: A set of functions that this function is overridden by.
  - *meta*: A special field into which a tool can export its (intermediate) results.

This allows, e.g., the PIRA profiler to export various additional information about the program's functions into the MetaCG file.
Examples are empirically determined performance models, runtime measurements, or loop nesting depth per function.

## Requirements / Building

MetaCG consists of the graph library, a CG construction tool, and an example analysis tool.
Currently, all components are built.

**Build Requirements**
- Clang/LLVM version 10.
- nlohmann/json library [github](https://github.com/nlohmann/json)
- Cube 4.5 [scalasca.org](https://www.scalasca.org/software/cube-4.x/download.html)
- spdlog [github](https://github.com/gabime/spdlog)
- cxxopts [github](https://github.com/jarro2783/cxxopts)
- Extra-P 3.0 [.tar.gz](http://apps.fz-juelich.de/scalasca/releases/extra-p/extrap-3.0.tar.gz)
- PyQt5
- matplotlib

### Building the package

Currently, one first needs to build the dependencies for PGIS.
This can be done by running the provided convenience script, which we use in a Ubuntu 20.04 container and on a RedHat 8.5 (with certain LMod modules).
This will build and install the required software into `./deps/src` and `./deps/install`, respectively.
No guarantees that the script works on your machine.

```{.sh}
?> cd pgis
?> ./build_submodules.sh
```

Thereafter, the package can be configured and built from the top-level CMake.

```{.sh}
?> mkdir build && build
?> cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/tmp/metacg -DCUBE_LIB=$(dirname $(which cube_info))/../lib -DCUBE_INCLUDE=$(dirname $(which cube_info))/../include/cubelib -DEXTRAP_INCLUDE=../pgis/deps/src/extrap/extrap-3.0/include -DEXTRAP_LIB=../pgis/deps/install/extrap/lib -DSPDLOG_BUILD_SHARED=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ..
?> make 
# Installation installs CGCollector, CGMerge, CGValidate, PGIS
?> make install
```

## Usage

### Graph Library
Provides the basic data structure and its means to read and write the data structure with the metadata to a JSON file.

### CGCollector
Clang-based call-graph generation tool for MetaCG.
It has the components CGCollector, CGMerge and CGValidate to construct the partial MCG per translation unit, merge the partial MCGs into the final whole-program MCG and validate edges against a full Score-P profile, respectively.


## Using CGCollector

It is easiest to apply CGCollector, when a compilation database (`compile_commands.json`) is present.
Then, CGCollector can be applied to a single source file using

```{.sh}
$> cgc target.cpp
```

`cgc` is a wrapper script that (tries to) determines the paths to the Clang standard includes.

Subsequently, the resulting partial MCGs are merged using `CGMerge` to create the final, whole-program call-graph of the application.

```{.sh}
?> echo "null" > $IPCG_FILENAME
?> find ./src -name "*.mcg" -exec cgmerge $IPCG_FILENAME $IPCG_FILENAME {} +
```

Optionally, you can test the call graph for missing edges, by providing an *unfiltered* application profile that was recorded using [Score-P](https://www.vi-hps.org/projects/score-p) in the [Cube](https://www.scalasca.org/scalasca/software/cube-4.x/download.html) library format.
This is done using the CGValidate tool, which also allows to patch all missing edges and nodes.

### PGIS (The PIRA Analyzer)

This tool is part of the [PIRA](https://github.com/tudasc/pira) toolchain.
It is used as analysis engine and constructs instrumentation selections guided by both static and dynamic information from a target application.

### Using PGIS

The PGIS tool offers a variety of modes to operate.
A list of all modes and options can be found with `pgis_pira --help`.
Currently, the user needs to provide any `parameter-file`, as required by the performance model guided instrumentation or the load imbalance detection.
Examples of such files can be found in the heuristics respective integration test directory.


1. Construct overview instrumentation configurations for Score-P from a MetaCG file.

```{.sh}
?> pgis_pira --metacg-format 2 --static mcg-file
```

2. Construct hot-spot instrumentation using raw runtime values.

```{.sh}
?> pgis_pira --metacg-format 2 --cube cube-file mcg-file
```

3. Construct performance model guided instrumentation configurations for Score-P using Extra-P.
The Extra-P configuration lists where to find the experiment series.
Its content follows what is expected by Extra-P.

```{.json}
{
 "dir": "./002",
 "prefix": "t",
 "postfix": "postfix",
 "reps": 5,
 "iter": 1,
 "params" : {
  "X": ["3", "5", "7", "9", "11"]
 }
}
```

```{.sh}
?> pgis_pira --metacg-format 2 --parameter-file <parameter-file> --extrap extrap-file mcg-file
```

4. Evaluate and construct load-imbalance instrumentation configuration.

```{.sh}
?> pgis_pira --metacg-format 2 --parameter-file <parameter-file> --lide 1 --cube cube-file mcg-file
```
