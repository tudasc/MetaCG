# pymetacg

`pymetacg` are MetaCG's Python bindings that support parsing MetaCG files from Python projects.
Currently, manipulating and writing call graphs is *not* supported.

## Dependencies
 - [nanobind](https://github.com/wjakob/nanobind)
 - [pytest](https://pypi.org/project/pytest/) (for tests only)
 - [pytest-cmake](https://pypi.org/project/pytest-cmake/) (for tests only)

## Building
`pymetacg` can be built as an optional feature during the MetaCG build, producing a dynamic library (e.g., `pymetacg.cpython-313-x86_64-linux-gnu.so`) that can then be loaded from Python.
Pass `-DMETACG_BUILD_PYMETACG=ON` to CMake to enable `pymetacg`.

```bash
$> cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/where/to/install -DMETACG_BUILD_PYMETACG=ON
$> cmake --build build --parallel
$> cmake --install build
```

By default, nanobind is downloaded automatically during configruation.
To use an already installed nanobind, use `-DMETACG_USE_EXTERNAL_NANOBIND=ON` and specify the installation path using `-Dnanobind_ROOT=<your nanobind installation>`, if necessary.

The CMake setup of `pymetacg` uses CMake's [FindPython](https://cmake.org/cmake/help/latest/module/FindPython.html) to find the Python installation for which `pymetacg` will be built.
To specify which Python installation to use, use `-DPython_ROOT_DIR=<python installation>`.

By default, the resulting dynamic library will be installed in `<installation prefix>/lib/<python version>/site-packages`.
To override the installation path, use `-DCMAKE_INSTALL_PYTHON_LIBDIR`.
Finally, make sure that the installation directory is found by Python, e.g., by using the `PYTHONPATH` environment variable.

Test whether `pymetacg` was built successfully:
```bash
$> python -c "import pymetacg; print(pymetacg.info)"
```

## Usage
```python
import pymetacg

# print MetaCG version information
print(pymetacg.info)

# read MetaCG file (.mcg)
cg = pymetacg.Callgraph.from_file("/path/to/callgraph.mcg")

# check whether node exists in graph
main_exists = "main" in cg

# access nodes in graph
main = cg["main"]
foo = cg["foo"]

# iterate over node callees and collect callee function names into list
callees_names = [callee.function_name for callee in main.callees]

# iterate over node callers and collect caller function names into list
callers_names = [caller.function_name for caller in foo.callers]

# check whether function calls itself
foo_is_recursive = foo in foo.callees

# iterate over nodes in graph and print function names
for node in cg:
    print(node.function_name)

# check if node has some meta data
main_has_statement_metadata = "numStatements" in main.meta_data

# access JSON representation of node's meta data using `.data` (independent of meta data type)
metadata = main.meta_data["numStatements"].data
```

## Tests
`pymetacg` comes with a PyTest-based test suite.
Use `-DMETACG_BUILD_PYMETACG_TESTS=ON` and install `pytest` and `pytest-cmake` need to be installed from PyPI to enable building to the tests:

```bash
# create and activate Python virtual environment
$> python -m venv .venv
$> source .venv/bin/activate

# install test dependencies from PyPI
$> pip install -r pymetacg/tests/requirements.txt

# configure and build MetaCG with pymetacg and its test suite enabled
$> cmake -S . -B build -G Ninja -DMETACG_BUILD_PYMETACG=ON -DMETACG_BUILD_PYMETACG_TESTS=ON
$> cmake --build build --parallel

# execute the tests
$> cd build/pymetacg/tets
$> ctest . --verbose
```