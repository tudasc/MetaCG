# Profile Guided Instrumentation Selection

This tool is part of the [PIRA](https://github.com/tudasc/pira) toolchain.
It is used as analysis engine and constructs instrumentation selections guided by both static and dynamic information from a target application.

#### Using PGIS

The PGIS tool offers a variety of modes to operate.
A list of all modes and options can be found with `pgis_pira --help`.
Currently, the user needs to provide any `parameter-file`, as required by the performance model guided instrumentation or the load imbalance detection.
Examples of such files can be found in the heuristics respective integration test directory.


1. Construct overview instrumentation configurations for Score-P from a MetaCG file.

```{.sh}
$> pgis_pira --metacg-format 2 --static mcg-file
```

2. Construct hot-spot instrumentation using raw runtime values.

```{.sh}
$> pgis_pira --metacg-format 2 --cube cube-file mcg-file
```

3. Construct performance model guided instrumentation configurations for Score-P using Extra-P.
The Extra-P configuration lists where to find the experiment series.
Its content follows what is expected by Extra-P.

```{.json}
{
 "dir": "./002",
 "prefix": "t",
 "postfix": "postfix",
 "reps": 5,
 "iter": 1,
 "params" : {
  "X": ["3", "5", "7", "9", "11"]
 }
}
```

```{.sh}
$> pgis_pira --metacg-format 2 --parameter-file <parameter-file> --extrap extrap-file mcg-file
```

4. Evaluate and construct load-imbalance instrumentation configuration.

```{.sh}
$> pgis_pira --metacg-format 2 --parameter-file <parameter-file> --lide 1 --cube cube-file mcg-file
```
