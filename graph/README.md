# MetaCG Graph Library

The JSON structure follows either the version two (MetaCGV2) or version three (MetaCGV3) specification.  
MetaCGV3 is more suitable for a wider range of applications due to having less necessary attributes. 
For any given function it requires the name and origin of the function and whether its definition is available.
It is also usually more space efficient compared to MetaCGV2 due to hashed function names and allows to export metadata not only to nodes but also to edges  
The MetaCGV3 specification also includes a more human-readable representation for debugging purposes.  
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

The version-two specification (MetaCGV2) contains the following information:
```{.json}
{
 "_MetaCG": {
   "version": "2.0",
   "generator": {
    "name": "ToolName",
    "version": "ToolVersion"
    }
  },
  "_CG": {
   "main": {
    "callers": [],
    "callees": ["foo"],
    "hasBody": true,
    "isVirtual": false,
    "doesOverride": false,
    "overrides": [],
    "overriddenBy": [],
    "meta": {
     "MetaTool": {}
    }
   }
  }
}
```

- *_MetaCG* contains information about the file itself, such as the file format version.
- *_CG* contains the actual serialized call graph. For each function, it lists
  - *callers*: A set of functions that this function may be called from.
  - *callees*: A set of functions that this function may call.
  - *hasBody*: Whether a definition of the function was found in the processed source files.
  - *isVirtual*: Whether the function is marked as virtual.
  - *doesOverride*: Whether this function overrides another function.
  - *overrides*: A set of functions that this function overrides.
  - *overriddenBy*: A set of functions that this function is overridden by.
  - *meta*: A special field into which a tool can export its (intermediate) results.

The version two specification is mainly used for the PIRA profiler, to export various additional information about the program's functions into the MetaCG file.
It uses the meta field to export e.g. empirically determined performance models, runtime measurements, or loop nesting depth per function.
