# MetaCG Tools

This folder contains tools for creating and merging call graphs using the MetaCG graph library.

## CaGe

CaGe is MetaCG's call graph generator from LLVM-IR.
Please refere to the separate [Readme](cage/README.md) for additional information.

## CGMerge2

CGMerge2 is a front-end for MetaCG's call graph merging functionality. 
It allows reading multiple call graphs from file and merges them into a single whole-program call graph.
It is a stand-in replacement for the original CGMerge tool.
Compared to the old JSON-based implementation, it operates directly on the in-memory graph representation, therefore making it agnostic w.r.t. the used file format.

### Basic usage

The tool interface is identical to the old CGMerge implementation: 

```
cgmerge2 <outfile> <infile1> <infile2> ..."
```

Note that `outfile` will be overridden, if it already exists.

### Custom metadata

CGMerge2 will automatically detect and merge all metadata types available in the graph library. 
Custom metadata handling can be added dynamically by putting the metadata implementation into a shared library.
Loading this library with `LD_PRELOAD` will automatically register the metadata type.

Example:

```
LD_PRELOAD="MyCustomMD.so" cgmerge2 merged.mcg in_A.mcg in_B.mcg
```

### Merge policy plugins

The built-in merge policies can be selected with `--merge-policy name|name-and-origin`.
A user-defined merge policy can be loaded from a shared library with `--plugin-path`.
Only one plugin can be loaded. If a plugin is loaded, `--merge-policy` is ignored and a warning is printed if it was set.

A plugin inherits from `cgmerge2::Plugin` (`cgmerge2/interface/CGMerge2Plugin.h`, CMake target `cgmerge2-plugin-interface`),
implements `findMatchingNode()` to return the `MergeAction` for each source node, and exports `getCGMerge2Plugin()`:

```cpp
#include "cgmerge2/interface/CGMerge2Plugin.h"

struct MyPolicy final : cgmerge2::Plugin {
  std::optional<metacg::MergeAction> findMatchingNode(const metacg::Callgraph& targetCG,
                                                      const metacg::CgNode& sourceNode) const final {
    // Return {targetNodeId, replace} to merge with an existing node, or std::nullopt to copy sourceNode
    return std::nullopt;
  }
  std::string getPluginName() const final { return "My Policy"; }
};

extern "C" cgmerge2::Plugin* getCGMerge2Plugin() { return new MyPolicy(); }
```

```
cgmerge2 --plugin-path ./libMyPolicy.so merged.mcg in_A.mcg in_B.mcg
```

cgcollector2 ships the plugin `libcgcollector2-merge-policy.so` (policy `MergeByNameAndOriginAndSignature`).
It matches nodes like `name-and-origin`, but additionally requires identical function signatures, taken from the
`FunctionSignatureMetadata` that cgcollector2 emits when run without `--whole-program`.
This keeps apart e.g. template instantiations with translation-unit-local types, which share mangled name and origin.
Nodes without signature metadata are matched by name and origin only, and a warning is printed.

```
cgmerge2 --plugin-path libcgcollector2-merge-policy.so merged.mcg in_A.mcg in_B.mcg
```

If CGMerge2 is built with OpenMP, `findMatchingNode()` is called concurrently from multiple threads.

## CGFormat

CGFormat is a utility tool created to help check and automatically correct the formatting of MetaCG call graphs.

Basic usage: `cgformat <input_file> [--apply -o <output_file>]`
See `cgformat --help` for a full documentation of input parameters.
Without `--apply`, `cgformat` checks the graph according to the given rules and returns a status code.
With `--apply`, a modified version of the graph is stored in the given output file.
If no output file is specified, the input file is overwritten.

There are some safety mechanism in place, to prevent the loss of metadata in the case that some metadata entries cannot 
be parsed.
By default, `cgformat` will exit with an error if any metadata would be lost during the export with `--apply`.
This behavior can be overridden with `--discard_failed_metadata`.

### Example use cases
- Checking that origin paths start with a specific prefix: `cgformat input.mcg --origin_prefix "/opt/metacg"`
- Replacing origin path prefixes: `cgformat input.mcg --origin_prefix "/opt/metacg" --origin_prefix_to_replace "oldpath" --apply -o output.mcg`
- Changing indentation to 4: `cgformat input.mcg --apply --indent 4`
- Removing all indentation and new lines: `cgformat input.mcg --apply --indent '-1'`
