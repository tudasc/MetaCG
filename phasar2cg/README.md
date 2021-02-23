[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# python2cg

This tool converts a CG in phasar output format to a CG in MetaCG output format.


## Example Usage

```
phasar-llvm --silent --module lulesh2.0.ll --call-graph-analysis=OTF --emit-cg-as-json | python3 phasar2cg.py > cg.json
```

