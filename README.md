[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# MetaCG

MetaCG provides a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the json file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The package contains an experimental Clang-based tool for call-graph construction, and a converter for the output files of [Phasar](https://github.com/secure-software-engineering/phasar).
Also, the package contains the PGIS analysis tool, which is used as the analysis backend in [PIRA](https://github.com/tudasc/pira).

Once constructed, the graph can be serialized into JSON.

The JSON structure follows either the version two (MetaCGV2) or version three (MetaCGV3) specification.  
MetaCGV3 is more suitable for a wider range of applications due to having less necessary attributes. 
For any given function it requires the name and origin of the function and whether its definition is available.
It is also usually more space efficient compared to MetaCGV2 due to hashed function names and allows to export metadata not only to nodes but also to edges  
The MetaCGV3 specification also includes a more human-readable representation for debugging purposes.  
It forgoes the hashing of names and explicitly lists caller and callees in exchange for increased filesize.  
<table>
<tr>
<td>
MetaCGV3 format
<td>
MetaCGV3 debug format
</td>
</tr>
<tr>
<td>
<pre>
{
 "_MetaCG":{
    "version":"3.0",
    "generator":{```
      "name":"ToolName",
      "sha":"GitSHA",
      "version":"ToolVersion"
    }
  },
  "_CG":{
    "edges":[
      [
        [11868120863286193964,9631199822919835226],
        {"EdgeMetadata":{"isHotEdge":true}}"
      ]
    ],
    "nodes":[
      [ 9631199822919835226,{
        <br>
        "functionName":" foo",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":5}
        },
        "origin":"main.cpp"
        }
      ],
      [11868120863286193964,{
        <br>
        "functionName":"main",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":8}
        },
        "origin":"main.cpp"
        }
      ]
    ]
  }
}
</pre>
</td>
<td>
<pre>
{
  "_MetaCG":{
    "version":"3.0",
    "generator":{
      "name":"ToolName",
      "sha":"GitSHA",
      "version":"ToolVersion"
    }
  },
  "_CG":{
    "edges":[
      [
        [11868120863286193964,9631199822919835226],
        {"EdgeMetadata":{"isHotEdge":true}}"
      ]
    ],
    "nodes":[
      [ "foo",{
        "callees":[],
        "callers":["main"],
        "functionName":" foo",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":5}
        },
        "origin":"main.cpp"
        }
      ],
      [ "main",{
        "callees":["foo"],
        "callers":[],
        "functionName":"main",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":8}
        },
        "origin":"main.cpp"
        }
      ]
    ]
  }
}
</pre>
</td>
</tr>

</table>

The version-two specification (MetaCGV2) contains the following information:
```{.json}
{
 "_MetaCG": {
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

The version two specification is mainly used for the PIRA profiler, to export various additional information about the program's functions into the MetaCG file.
It uses the meta field to export e.g. empirically determined performance models, runtime measurements, or loop nesting depth per function.

## Requirements and Building

MetaCG consists of the graph library, a CG construction tool, and an example analysis tool.
The graph library is always built, while the CGCollector and the PGIS tool can be disabled at configure time.

We test MetaCG internally using GCC 10 and 11 for Clang 10 and 14 (for GCC 10) and 13 and 14 (for GCC 11), respectively.
Other version combinations *may* work.

**Build Requirements (for graph lib)**
- nlohmann/json library [github](https://github.com/nlohmann/json)
- spdlog [github](https://github.com/gabime/spdlog)

**Additional Build Requirements (for full build)**
- Clang/LLVM version 10 (and above)
- Cube 4.5 [scalasca.org](https://www.scalasca.org/software/cube-4.x/download.html)
- Extra-P 3.0 [.tar.gz](http://apps.fz-juelich.de/scalasca/releases/extra-p/extrap-3.0.tar.gz)
- cxxopts [github](https://github.com/jarro2783/cxxopts)
- PyQt5
- matplotlib

### Building

#### Graph Library Only

The default is to build only the graph library.
The build requirements are downloaded at configure time.
While CMake looks for `nlohmann-json` version 3.10., MetaCG should work starting from version 3.9.2.
For spdlog, we rely on version 1.8.2 -- other versions *may* work.
If you do not want to download at configure time, please use the respective CMake options listed below.

```{.sh}
# To build the MetaCG library w/o PGIS or CGCollector
$> cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/where/to/install
$> cmake --build build --parallel
$> cmake --install build
```

#### Full Package Build

You can configure MetaCG to also build CGCollector and PGIS.
This requires additional dependencies.
Clang/LLVM (in a supported version) and the Cube library are assumed to be available on the system.
Extra-P can be built using the `build_submodules.sh` script provided in the repository, though the script is not tested outside of our CI system.
It builds and installs Extra-P into `./deps/src` and `./deps/install`, respectively.

```{.sh}
$> ./build_submodules.sh
```

Thereafter, the package can be configured and built from the top-level CMake.
Change the `CMAKE_INSTALL_PREFIX` to where you want your MetaCG installation to live.

```{.sh}
# To build the MetaCG library w/ PGIS and CGCollector
$> cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/where/to/install -DCUBE_LIB=$(dirname $(which cube_info))/../lib -DCUBE_INCLUDE=$(dirname $(which cube_info))/../include/cubelib -DEXTRAP_INCLUDE=./extern/src/extrap/extrap-3.0/include -DEXTRAP_LIB=./extern/install/extrap/lib
$> cmake --build build --parallel
# Installation installs CGCollector, CGMerge, CGValidate, PGIS
$> cmake --install build
```

#### General CMake Options

These options are common for the MetaCG package.

- Bool `METACG_BUILD_CGCOLLECTOR`: Whether to build call-graph construction tool <default=OFF>
- Bool `METACG_BUILD_PGIS`: Whether to build demo-analysis tool <default=OFF>
- Bool `METACG_USE_EXTERNAL_JSON`: Search for installed version of nlohmann-json <default=OFF>

#### PGIS CMake Options

These options are required when building with `METACG_BUILD_PGIS=ON`.

- Path `CUBE_LIB`: Path to the libcube library directory
- Path `CUBE_INCLUDE`: Path to the libcube include directory
- Path `EXTRAP_LIB`: Path to the Extra-P library directory
- Path `EXTRAP_INCLUDE`: Path to the Extra-P include directory

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


#### Using CGCollector

It is easiest to apply CGCollector, when a compilation database (`compile_commands.json`) is present.
Then, CGCollector can be applied to a single source file using

```{.sh}
$> cgc target.cpp
```

`cgc` is a wrapper script that (tries to) determines the paths to the Clang standard includes.

Subsequently, the resulting partial MCGs are merged using `CGMerge` to create the final, whole-program call-graph of the application.

```{.sh}
$> echo "null" > $IPCG_FILENAME
$> find ./src -name "*.mcg" -exec cgmerge $IPCG_FILENAME $IPCG_FILENAME {} +
```

##### CGCollector / CGMerge on Multi-File Projects

The easiest approch to apply the CGCollector / CGMerge toolchain to a multi-file project is using the `TargetCollector.py` tool.
It is a convenience tool around CMake's file API that allows to configure the target project and apply the CGCollector / CGMerge to only the source files required for a given CMake target.
Check out the `graph/test/integration/TargetCollector/TestRunner.sh` script for an example invocation.

In case you want to apply the CGCollector / CGMerge toolchain to a non-CMake project, you need to resort to manually finding the files that need to be processed and merged for the given use case.

#### Validation of Generated Callgraph

Optionally, you can test the call graph for missing edges, by providing an *unfiltered* application profile that was recorded using [Score-P](https://www.vi-hps.org/projects/score-p) in the [Cube](https://www.scalasca.org/scalasca/software/cube-4.x/download.html) library format.
This is done using the CGValidate tool, which also allows to patch all missing edges and nodes.


### PGIS (The PIRA Analyzer)

This tool is part of the [PIRA](https://github.com/tudasc/pira) toolchain.
It is used as analysis engine and constructs instrumentation selections guided by both static and dynamic information from a target application.

#### Using PGIS

The PGIS tool offers a variety of modes to operate.
A list of all modes and options can be found with `pgis_pira --help`.
Currently, the user needs to provide any `parameter-file`, as required by the performance model guided instrumentation or the load imbalance detection.
Examples of such files can be found in the heuristics respective integration test directory.


1. Construct overview instrumentation configurations for Score-P from a MetaCG file.

```{.sh}
$> pgis_pira --metacg-format 2 --static mcg-file
```

2. Construct hot-spot instrumentation using raw runtime values.

```{.sh}
$> pgis_pira --metacg-format 2 --cube cube-file mcg-file
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
$> pgis_pira --metacg-format 2 --parameter-file <parameter-file> --extrap extrap-file mcg-file
```

4. Evaluate and construct load-imbalance instrumentation configuration.

```{.sh}
$> pgis_pira --metacg-format 2 --parameter-file <parameter-file> --lide 1 --cube cube-file mcg-file
```
