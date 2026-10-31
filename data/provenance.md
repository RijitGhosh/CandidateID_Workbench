# Data provenance

Both example files were generated for this project by `src/tools/make_examples.cpp` (run: `build/make_examples data`).
They are **self-authored**; no third-party data, no CAD export and no ANSYS output is included.

## geometry/example_bracket.stl
| Item | Value |
|---|---|
| Format | ASCII STL (`solid ... endsolid`) |
| Content | L-shaped bracket: base box 40 x 20 x 5 plus upright box 5 x 20 x 25 stacked on it |
| Triangles | 24 (2 boxes x 6 faces x 2 triangles); 0 degenerate, 0 duplicate (verified by `Integration.ExampleStlLoadsWithDocumentedCounts`) |
| Bounding box | (0, 0, 0) to (40, 20, 30) |
| Units | **Not encoded in STL.** Authored in millimetres by convention; the application only labels axes, it never scales the model. |
| File size | about 5 KB |
| Tested size range | Parser unit test: 20,000 triangles (~2.6 MB text) parses in a few ms. Hard limits in code: 256 MiB and 5,000,000 triangles. Files near those limits were NOT tested. |
| Geometry note | The two boxes overlap on a shared face, so the mesh is two closed shells, not one merged solid. Every edge is shared by an even number of triangles (test `ExampleStlIsWatertightEveryEdgeSharedByTwoTriangles`). |

## temperature/example_temperature.csv
Synthetic example generated for this prototype. Not an actual ANSYS export.

| Item | Value |
|---|---|
| Schema | `node_id,x_m,y_m,z_m,temperature_C` (columns in any order, all five required, no extra columns; `#` comment lines allowed) |
| Units | metres, degrees Celsius |
| Nodes | 189 = 21 x 3 x 3 regular grid, x in [0, 1] m, y and z in [-0.05, 0.05] m |
| Field | T = 100 - 80x + 500 x(1-x) + 40(y^2 + z^2) (analytic formula, not a solver result) |
| Registration | The CSV nodes are **not registered** to the example STL; the application shows them in separate viewports. |
| Limits | 100 MiB file size, 2,000,000 rows, first 20 errors reported. Tested: 50,000-row file in a unit test. |

## Limitations
- The CSV parser does not support quoted fields, embedded commas or non-UTF-8 text.
- Duplicate STL triangles are detected by exact vertex equality (any vertex order), not by tolerance.
