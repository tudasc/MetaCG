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





