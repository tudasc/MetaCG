[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# MetaCG

The goal of MetaCG is to provide a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the json file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The package contains an experimental Clang-based tool for its construction, and a converter for the output files of [Phasar](https://github.com/secure-software-engineering/phasar).
Also, the package contains the PGIS analysis tool, which is used as the backend in [PIRA](https://github.com/tudasc/pira).

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
Examples are empirically determined perfomance models, runtime measurements, or loop nesting depth per function.




## Repository Organization

The different tools can be built individually - please refer to the individual READMEs for further information.

For a reference on how to build and use the tools, please refer to the .gitlab-ci.yml file.

The license file and an authors file can be found in the individual folders.


### CGCollector

Clang-based tool to collect the whole-program call graph of a C/C++ application.


### PGIS

Analysis tool that heuristically determines low-overhead instrumentation for the PIRA project.

