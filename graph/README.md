# MetaCG Graph Library

## File Format

The MetaCG graph library uses a JSON format for serializing call graphs.
The newest version is 4, with version 2 and 4 being actively supported in the current release version.  

To illustrate the two formats and highlight differences, consider the following code snippet:

```C++
 struct A {
     virtual void foo() = 0;
 };
 
 struct B: A {
     void foo() override {}
 };
 
 void bar(A* a) {
     a->foo();
 }
```

The table below shows a comparison of the corresponding call graphs in format version 2 and 4.

<table>
<tr>
<td>
Version 2
<td>
Version 4
</td>
</tr>
<tr>
<td>
<pre>
{
    "_CG": {
        "_Z3barP1A": {
            "callees": [
                "_ZN1A3fooEv"
            ],
            "callers": [],
            "doesOverride": false,
            "hasBody": true,
            "isVirtual": false,
            "meta": {
                "fileProperties": {
                    "origin": "virtual_calls.cpp",
                    "systemInclude": false
                }
            },
            "overriddenBy": [],
            "overrides": []
        },
        "_ZN1A3fooEv": {
            "callees": [],
            "callers": [
                "_Z3barP1A"
            ],
            "doesOverride": false,
            "hasBody": false,
            "isVirtual": true,
            "meta": {
                "fileProperties": {
                    "origin": "virtual_calls.cpp",
                    "systemInclude": false
                }
            },
            "overriddenBy": [
                "_ZN1B3fooEv"
            ],
            "overrides": []
        },
        "_ZN1B3fooEv": {
            "callees": [],
            "callers": [],
            "doesOverride": true,
            "hasBody": true,
            "isVirtual": true,
            "meta": {
                "fileProperties": {
                    "origin": "virtual_calls.cpp",
                    "systemInclude": false
                }
            },
            "overriddenBy": [],
            "overrides": [
                "_ZN1A3fooEv"
            ]
        }
    },
    "_MetaCG": {
        "generator": {
            "name": "CGCollector",
            "sha": "ce27d337f62288d0819ef3e0c837620e6f624b1f",
            "version": "0.7"
        },
        "version": "2.0"
    }
}
</pre>
</td>
<td>
<pre>
{
    "_CG": {
        "0": {
            "callees": {
                "1": {}
            },
            "functionName": "_Z3barP1A",
            "hasBody": true,
            "meta": {
                "fileProperties": {
                    "systemInclude": false
                }
            },
            "origin": "virtual_calls.cpp"
        },
        "1": {
            "callees": {},
            "functionName": "_ZN1A3fooEv",
            "hasBody": false,
            "meta": {
                "fileProperties": {
                    "systemInclude": false
                },
                "overrideMD": {
                    "overriddenBy": [
                        "2"
                    ],
                    "overrides": []
                }
            },
            "origin": "virtual_calls.cpp"
        },
        "2": {
            "callees": {},
            "functionName": "_ZN1B3fooEv",
            "hasBody": true,
            "meta": {
                "fileProperties": {
                    "systemInclude": false
                },
                "overrideMD": {
                    "overriddenBy": [],
                    "overrides": [
                        "1"
                    ]
                }
            },
            "origin": "virtual_calls.cpp"
        }
    },
    "_MetaCG": {
        "generator": {
            "name": "MetaCG",
            "sha": "ce27d337f62288d0819ef3e0c837620e6f624b1f",
            "version": "0.7"
        },
        "version": "4.0"
    }
}
</pre>
</td>

</tr>

</table>


### Explanation of the v2 file format
- `_MetaCG` contains information about the file itself, such as the file format version.
- `_CG` contains the actual serialized call graph. For each function, it lists
  - `callers`: A set of functions that this function may be called from.
  - `callees`: A set of functions that this function may call.
  - `hasBody`: Whether a definition of the function was found in the processed source files.
  - `isVirtual`: Whether the function is marked as virtual.
  - `doesOverride`: Whether this function overrides another function.
  - `overrides`: A set of functions that this function overrides.
  - `overriddenBy`: A set of functions that this function is overridden by.
  - `meta`: A special field into which a tool can export its (intermediate) results.

The version 2 specification is mainly used for the PIRA profiler, to export various additional information about the 
program's functions into the MetaCG file.
It uses the meta field to export e.g. empirically determined performance models, runtime measurements, or loop nesting 
depth per function.

### Differences in file format v4
Version 4 aims to reduce the file size, by eliminating redundant or superfluous information, and removing the necessity 
of referring to node by (potentially very long) function names. 
- Functions are identified by unique string IDs (which may or may not coincide with the `functionName`).
- There is no `callers` entry.
- Information about virtual functions (`isVirtual`, `doesOverride`, `overrides` and `overriddenBy`) has been moved into the `overrideMD` metadata.
- The `origin` field, previously in the `fileProperties` metadata, has been promoted to a first-class property.

In addition to these changes, v4 adds the following new features:
- It is possible to add multiple nodes with the same `functionName`.
- Metadata can be attached to the edges in the `callees` field.



### Format Version 3 (deprecated)
Format version 3 is not actively supported anymore and has been replaced by version 4.
It uses hashes to identify function names within the file. 
The V3 specification also includes a more human-readable representation for debugging purposes.  
It forgoes the hashing of names and explicitly lists caller and callees in exchange for increased filesize.  
<table>
<tr>
<td>
MetaCGV3 format
<td>
MetaCGV3 debug format
</td>
</tr>
<tr>
<td>
<pre>
{
 "_MetaCG":{
    "version":"3.0",
    "generator":{```
      "name":"ToolName",
      "sha":"GitSHA",
      "version":"ToolVersion"
    }
  },
  "_CG":{
    "edges":[
      [
        [11868120863286193964,9631199822919835226],
        {"EdgeMetadata":{"isHotEdge":true}}"
      ]
    ],
    "nodes":[
      [ 9631199822919835226,{
        <br>
        "functionName":" foo",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":5}
        },
        "origin":"main.cpp"
        }
      ],
      [11868120863286193964,{
        <br>
        "functionName":"main",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":8}
        },
        "origin":"main.cpp"
        }
      ]
    ]
  }
}
</pre>
</td>
<td>
<pre>
{
  "_MetaCG":{
    "version":"3.0",
    "generator":{
      "name":"ToolName",
      "sha":"GitSHA",
      "version":"ToolVersion"
    }
  },
  "_CG":{
    "edges":[
      [
        [11868120863286193964,9631199822919835226],
        {"EdgeMetadata":{"isHotEdge":true}}"
      ]
    ],
    "nodes":[
      [ "foo",{
        "callees":[],
        "callers":["main"],
        "functionName":" foo",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":5}
        },
        "origin":"main.cpp"
        }
      ],
      [ "main",{
        "callees":["foo"],
        "callers":[],
        "functionName":"main",
        "hasBody":true,
        "meta":{
          "NumInstructionsMetadata":{"instructions":8}
        },
        "origin":"main.cpp"
        }
      ]
    ]
  }
}
</pre>
</td>
</tr>

</table>