[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# MetaCG

MetaCG provides a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the JSON file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The MetaCG graph library is the fundamental component, together with, e.g., I/O facilities.
The repository also contains an experimental Clang-based tool for call-graph construction at the source-code level.

The current default file format is MetaCG format version 3.
More info on the different formats can be found in the [graph README](graph/README.md).

The MetaCG project includes `pymetacg`, a set of bindings that allows to parse MetaCG files from Python.
Refer to the [pymetacg README](pymetacg/README.md) for more information how to build and use `pymetacg`.

## Requirements and Building

MetaCG consists of the graph library, a CG construction tool, and an example analysis tool.
The graph library is always built, while the CGCollector and other graphlib-based tools can be disabled at configure time.

We test MetaCG internally using GCC 11 for Clang/LLVM 13, 14, 15, 16, 17 and 18.
Other version combinations *may* work.

**Build Requirements (for graph lib)**
- nlohmann/json library [github](https://github.com/nlohmann/json)
- spdlog [github](https://github.com/gabime/spdlog)

**Additional Build Requirements (for full build)**
- Clang/LLVM version 10 (and above)
- Cube 4.5 [scalasca.org](https://www.scalasca.org/software/cube-4.x/download.html)
- cxxopts [github](https://github.com/jarro2783/cxxopts)

### Building

#### Graph Library Only

The default is to build only the graph library.
The build requirements are downloaded at configure time.
While CMake looks for `nlohmann-json` version 3.12., MetaCG should work starting from version 3.9.2.
For spdlog, we rely on version 1.8.2 -- other versions *may* work.
If you do not want to download at configure time, please use the respective CMake options listed below.

```{.sh}
# To build the MetaCG library w/o CGCollector
$> cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/where/to/install
$> cmake --build build --parallel
$> cmake --install build
```

#### Build Graph Library and Tools (CGCollector)

You can configure MetaCG to also build CGCollector.
This requires additional dependencies.
Clang/LLVM (in a supported version) are assumed to be available on the system.
The Cube library can be built using the `build_submodules.sh` script provided in the repository, though the script is not tested outside of our CI system.
It builds and installs the dependencies into `./extern`.

```{.sh}
$> ./build_submodules.sh
```

Thereafter, the package can be configured and built from the top-level CMake.
Change the `CMAKE_INSTALL_PREFIX` to where you want your MetaCG installation to live.

```{.sh}
# To build the MetaCG library w/ CGCollector
$> extinstalldir=./extern/install
$> cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=/where/to/install \
  -DCUBE_DIR="$extinstalldir/cubelib" \
  -DMETACG_BUILD_CGCOLLECTOR=ON \
$> cmake --build build --parallel
# Installation installs CGCollector, CGMerge, CGValidate
$> cmake --install build
```

#### General CMake Options

These options are common for the MetaCG package.

- Bool `METACG_BUILD_CGCOLLECTOR`: Whether to build call-graph construction tool <default=OFF>
- Bool `METACG_USE_EXTERNAL_JSON`: Search for installed version of nlohmann-json <default=OFF>

#### CGCollector CMake Options

These options are required when building with `METACG_BUILD_CGCOLLECTOR=ON`.

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
