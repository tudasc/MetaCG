# CGCollector

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


