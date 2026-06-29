# CaGe - Call Graph Generator

CaGe is MetaCG's call graph generator for LLVM IR, implemented as an LLVM pass plugin.\
It is intended to extract a whole-program call graph from compiled LLVM IR at linktime and writes the result to a MetaCG JSON file (`.mcg`).\
It handles all calls found by LLVM's call graph analysis and augments it with indirect calls via function pointers and C++ virtual calls using different analysis backends.\
CaGe itself can be extended with CaGe-Plugins to modify the graph or add custom MetaCG metadata. 

CaGe can be invoked in two modes depending on how it is loaded into the LLVM pipeline:

- **LTO mode** (recommended): the pass runs at the end of the full LTO pipeline during linking, where all translation units are visible simultaneously.
- **opt mode**: the pass is loaded into the standard per-TU optimizer pipeline via the `opt` tool. For a whole program call graph, these then need to be merged with a tool like `cgmerge`.

## LTO mode

Compile each translation unit with `-flto`, then link with LLD and the CaGe plugin.
The `metacg-config` helper generates all required flags:

```sh
# Compile step
clang++ $(metacg-config --cage-cxxflags) example_a.cpp -c -o example_a.o
clang++ $(metacg-config --cage-cxxflags) example_b.cpp -c -o example_b.o

# Link step — CaGe runs at the end of the LTO pipeline
clang++ $(metacg-config --cage-ldflags --cage-pass-option -cg-file=example.mcg) \
    example_a.o example_b.o -o example
```

The call graph is written to `example.mcg` after the link step completes.

## Opt mode

In opt mode, a partial call graph is generated for each translation unit.
For CaGe to run, an optimization level has to be enabled, or CaGe needs to be invoked explicitly:

```sh
#Invoke via optlevel
clang++ -fpass-plugin=$(metacg-config --prefix)/lib/CaGe.so -O2 example.cpp

#Or via explicit pass control
# Compile to IR
clang++ -emit-llvm -S example.cpp 

# Run the CaGe pass
opt --load-pass-plugin=$(metacg-config --prefix)/lib/CaGe.so --passes=CaGe \
    -cg-file=example.mcg 
```
There is no way to have pass-granular control from the clang-driver invocation, making this two-step approach necessary.

## Pass options

Pass options are forwarded to CaGe depending on the invocation type.

### LTO mode
For LTO mode pass `-Wl,-mllvm="<option>"` or use the more expressive `$(metacg-config --cage-pass-option <option>)` 

### Opt mode
If you invoke opt via the `clang` compiler driver use `-fplugin-arg-CaGe-<option>`.\
If you invoke opt directly, you should be able to use the options directly.


### List of options
| Option | Description                                       | Default              |
|---|---------------------------------------------------|----------------------|
| `-cg-file=<path>` | Output file for the generated call graph          | `cage_callgraph.mcg` |
| `-pta=no/signature` | Indirect-call resolution strategy                 | `no`                 |
| `-cage-verbose` | Print debug output                                | off                  |
| `-plugin-paths=<path>` | List of paths to load CaGe plugins from           | empty                |

The `-pta=signature` option approximates indirect calls by adding all functions whose signature matches the call site as potential targets.
There is an additional metavirt backend implemented for indirect call resolving, for which there currently is no runtime control.
It is currently enabled or disabled at the time of CaGe's compilation.

The output file can also be set via the `CAGE_CG` environment variable; an explicit `-cg-file` takes precedence.

## Plugin system

CaGe supports dynamically loaded plugins that implement the `cage::Plugin` interface.
A plugin can augment the call graph (add nodes, edges, or metadata from the LLVM module) or consume it (convert to another format, trigger downstream analysis).
See `CaGeDemoPlugin/` for a minimal example.

Plugins are loaded by passing `-plugin-paths=<path>` as a pass option.

### Build system integration
Integrating CaGe into build systems, e.g. Make and CMake, is simple.
Many Make projects already define `CXXFLAGS` and `LDFLAGS` variables, which can be extended with the respective
`metacg-config` output.
For CMake projects, the relevant options are `CMAKE_CXX_FLAGS`, `CMAKE_EXE_LINKER_FLAGS` and `CMAKE_SHARED_LINKER_FLAGS`.
