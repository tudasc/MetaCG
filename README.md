[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# MetaCG

MetaCG provides a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the JSON file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The MetaCG graph library is the fundamental component, together with, e.g., I/O facilities.
The repository also contains an experimental Clang-based tool for call-graph construction at the source-code level.

The current default file format is MetaCG format version 4.
More info on the different formats can be found in the [graph README](graph/README.md).

The MetaCG project includes `pymetacg`, a set of bindings that allows to parse MetaCG files from Python.
Refer to the [pymetacg README](pymetacg/README.md) for more information how to build and use `pymetacg`.

MetaCG originated from PGIS, a tool to generate Score-P instrumentation configurations from static call-graph data and dynamic profile information, that is part of the [PIRA](https://github.com/tudasc/PIRA) toolset.
PGIS is no longer maintained and was thus removed from the repository.
The last MetaCG release that includes PGIS is v1.00.0.

## Requirements and Building

MetaCG consists of the graph library, CG construction tools, analysis tools, and Python bindings.
The graph library is always built, while the other components can be selectively built.
In the GitHub testing, MetaCG is built using GCC 12 and tests against LLVM versions 18, 20 and 22.
Other version combinations *may* work.

**Build Requirements (for graph lib)**
- nlohmann/json library [github](https://github.com/nlohmann/json)
- spdlog [github](https://github.com/gabime/spdlog)

### Building

#### Graph Library Only

The default is to build only the graph library.
The build requirements are downloaded at configure time.
While CMake looks for `nlohmann-json` version 3.12., MetaCG should work starting from version 3.9.2.
For spdlog, we rely on version 1.15.3 -- other versions *may* work.
If you do not want to download at configure time, please use the respective CMake options listed below.

```{.sh}
# To build the MetaCG library only
$> cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/where/to/install
$> cmake --build build --parallel
$> cmake --install build
```

#### Build Graph Library and Tools (CGCollector, CGFCollector, ...)

You can configure MetaCG to also build CGCollector and other tools based on the graphlib (cgconvert, cgformat, cgmerge2).
This requires additional dependencies.

**Additional Build Requirements (for full build)**
- Clang/LLVM version 15 (and above)
- Flang/LLVM version 18 (and above) for the Fortran call graph collector CGFCollector
- Cube 4.5 [scalasca.org](https://www.scalasca.org/software/cube-4.x/download.html)
- cxxopts [github](https://github.com/jarro2783/cxxopts)


Clang/LLVM (in a supported version) are assumed to be available on the system and is required for the CG construction tools.

The Cube library (required for CGValidate and CGValidate2) can be built using the `build_submodules.sh` script provided in the repository, though the script is not tested outside of our CI system.
It builds and installs the dependencies into `./extern`.

```{.sh}
$> ./build_submodules.sh
```

Thereafter, the package can be configured and built from the top-level CMake.
Change the `CMAKE_INSTALL_PREFIX` to where you want your MetaCG installation to live.

```{.sh}
# To build the MetaCG library w/ CGCollector and graph tools
$> extinstalldir=./extern/install
$> cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=/where/to/install \
  -DCUBE_DIR="$extinstalldir/cubelib" \
  -DMETACG_BUILD_CGCOLLECTOR=ON \
  -DMETACG_BUILD_GRAPH_TOOLS=ON
$> cmake --build build --parallel
# Installation installs CGCollector, CGMerge, CGValidate and the graph tools
$> cmake --install build
```

#### General CMake Options

These options are common for the MetaCG package.

- Bool `METACG_BUILD_CGCOLLECTOR`: Whether to build call-graph construction tool <default=OFF>
- Bool `METACG_USE_EXTERNAL_JSON`: Search for installed version of nlohmann-json <default=OFF>

#### Cube CMake Options

These options are required when building with `METACG_BUILD_CGCOLLECTOR=ON` or `METACG_BUILD_CGVALIDATE2=ON`.

- Path `CUBE_DIR`: Path to the libcube installation

## Usage

### Graph Library

Provides the basic data structures and its means to read and write the call graph with the metadata to a JSON file.
To include MetaCG as library in your project, you can simply install, find the package and link the library.
This pulls in all required dependencies and compile flags.

```
# In your project's CMake
find_package(MetaCG <VERSION_STR> REQUIRED)
# Assuming you have a target MainApp
target_link_library(MainApp metacg::metacg)
```

### CGCollector
Clang-based call-graph generation tool for MetaCG.
It has the components CGCollector, CGMerge and CGValidate to construct the partial MCG per translation unit, merge the partial MCGs into the final whole-program MCG and validate edges against a full Score-P profile, respectively.

See its [README](cgcollector/README.md) for details.
