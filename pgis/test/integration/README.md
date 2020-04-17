# Integration Testing

Currently, three distinct input folders `inputstatic`, `inputdynamic`, and `inputmodeling` are used to group the available integration tests.
The top-level invocation is handled using the `runner.sh`.
Folder names correspond to the selection mode, i.e., `inputstatic` has the files required for the `static` test suite, etc.

The tests take as input whatever data they require, e.g., `.ipcg` and `.cubex` files for the *dyanmic* tests.
The files are expected to have the same name, i.e., `001.ipcg` and `001.cubex`.
The instrumentation output is generated in the corresponding `outXXX` directory.
Testing is implemented by checking if the expected functions are output in the `instrumented-XXX.txt` file.
Therefore, the user provides a `.afl` file, which holds a list of the function names that are expected to be instrumented, e.g., `001.afl`.

To run the tests for the static, i.e., fully IPCG-based tests, using the *default build folder* (`$PGIS/build`):

```{.sh}
$> ./runner.sh static
```

To run tests with a different build location, run

```{.sh}
$> ./runner.sh static $PWD/relative/path/to/build/location
```

To run dynamic tests, i.e., PIRA I selection based on IPCG and runtime data:

```{.sh}
$> ./runner.sh dynamic
```

To run tests that check the Extra-P modeling-based selection:

```{.sh}
$> ./runner.sh modeling
```
