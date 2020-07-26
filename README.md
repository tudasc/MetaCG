[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# MetaCG

The goal of MetaCG is to provide a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the json file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The different tools can be built individually - please refer to the individual READMEs for further information.

For a reference on how to build and use the tools, please refer to the .gitlab-ci.yml file.

The license file and an authors file can be found in the individual folders.


## CGCollector

A clang-based tool to collect the whole-program call graph of a C/C++ application


## PGIS

Example tool: standalone analysis tool that heuristically determines low-overhead instrumentation for the PIRA project.

