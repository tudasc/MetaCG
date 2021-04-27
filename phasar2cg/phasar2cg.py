
import json
import sys

class CgFn:
    def __init__(self):
        self.callees                = []
        self.doesOverride           = False
        self.hasBody                = True
        self.isVirtual              = False
        self.numStatements          = 0
        self.overriddenBy           = []
        self.overriddenFunctions    = []
        self.parents                = []

cg_json = {}
phasar_json = None

for line in sys.stdin:
    phasar_json = json.loads(line)

for fn, callees in phasar_json["CallGraph"].items():
    cgfn = CgFn()
    if callees != None:
        for callee in callees:
            cgfn.callees.append(callee)
    cg_json[fn] = cgfn.__dict__

for fn, callees in phasar_json["CallGraph"].items():
    if callees != None:
        for callee in callees:
            if callee in cg_json:
                if not (fn in (cg_json[callee]["parents"])):
                    cg_json[callee]["parents"].append(fn)

print(json.dumps(cg_json, indent=4))
