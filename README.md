# MetaCG

The goal of MetaCG is to provide a common whole-program call-graph representation to exchange information between different tools built on top of LLVM/Clang.
It uses the json file format and separates structure from information, i.e., caller/callee relation and *meta* information.

The different tools can be built individually - please refer to the individual READMEs for further information.

For a recipe on how to build and use the tools, you can also use the Gitlab CI yml files.


## CGCollector

A clang-based tool to collect the whole-program call graph of a C/C++ application


## PGIS

Example tool: standalone analysis tool that heuristically determines low-overhead instrumentation for the PIRA project.

