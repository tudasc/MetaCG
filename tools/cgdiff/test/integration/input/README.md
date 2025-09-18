| Testcase                        | Description         | Flags                | Notes                          | Differs? | groundtruth file         |
| ------------------------------- | ------------------- | -------------------- | ------------------------------ | -------- | ------------------------ |
| cgA_basic vs. cgB_basic         | node missing        | none                 | "foo" missing in B             | yes      | node_missing.md          |
| cgA_basic vs. cgC_basic         | edge missing        | none                 | edge main -> foo missing in B  | yes      | edge_missing_flag        |
| cgA_basic vs. cgC_basic         | edge missing        | `--ignore-edges`     | edge main -> foo missing in B  | no       | edge_missing_flag        |
| cgA_basic vs. cgD_basic         | body different      | none                 | main.hasBody = false in D      | yes      | body_different           |
| cgA_basic vs. cgD_basic         | body different      | `ignore-has-body`    | main.hasBody = false in D      | no       | body_different_flag      |
| cgA_basic vs. cgE_basic         | edge different      | none                 | edges different                | yes      | edge_different           |
| cgA_basic vs. cgE_basic         | edge different      | `--ignore-edges`     | edges different                | no       | edge_different_flag      |
|                                 |                     |                      |                                |          |                          |
| cgA_metadata vs. cgA_basic      | metadata missing    | none                 | metadata missing in cgA_basic  | yes      | metadata_missing         |
| cgA_metadata vs. cgA_basic      | metadata missing    | `--ignore-metadata`  | metadata missing in cgA_basic  | no       | metadata_missing_flag    |
| cgA_metadata vs. cgB_metadata   | metadata different  | none                 | metadata different             | yes      | metadata_different       |
| cgA_metadata vs. cgB_metadata   | metadata different  | `--ignore-metadata`  | metadata different             | no       | metadata_different_flag  |
|                                 |                     |                      |                                |          |                          |
| cgA_global_md vs. cgA_basic     | global md missing   | none                 | global md missing in cgA_basic | yes      | global_md_missing        |
| cgA_global_md vs. cgA_basic     | global md missing   | `--ignore-global-md` | global md missing in cgA_basic | no       | global_md_missing_flag   |
| cgA_global_md vs. cgB_global_md | global md different | none                 | global md different            | yes      | global_md_different      |
| cgA_global_md vs. cgB_global_md | global md different | `--ignore-global-md` | global md different            | no       | global_md_different_flag |
|                                 |                     |                      |                                |          |                          |
|                                 |                     |                      |                                |          |                          |
|                                 |                     |                      |                                |          |                          |
|                                 |                     |                      |                                |          |                          |
|                                 |                     |                      |                                |          |                          |
