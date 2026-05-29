# CGDiff

CGDiff compares to MetaCG call-graphs. The comparison is based on features such as name, hasBody, callees, metadata.

**NOTE:** Since nodeIDs are not unique across call graphs, the node name is used as the unique identifier.

### Usage

```

./cgdiff <file1.mcg> <file2.mcg> [options]

```

**Example:**  `./cgdiff ./input/cgA_basic.mcg ./input/cgC_basic.mcg --ignore-edges`
