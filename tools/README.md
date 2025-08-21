# MetaCG Tools

This folder contains tools for creating and merging call graphs using the MetaCG graph library.

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
