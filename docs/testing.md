# Testing

## Environment (where the results below were actually obtained)
| Item | Value |
|---|---|
| OS | Linux (Ubuntu 24.04 container), 2 logical CPUs, 7.8 GiB RAM, Intel Xeon @ 2.10 GHz (virtualised) |
| Compiler | GCC 13.3.0, `-std=c++17`, flags from the Release build (`-O3 -DNDEBUG`) |
| Build | CMake 3.28.3 + Ninja 1.11.1, build type Release |
| Test framework | GoogleTest 1.14.0 (`libgtest-dev` 1.14.0-1) |
| Qt | 6.4.2 (GUI built and started; Qt is not used by the tests) |

## Commands
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure      # or: ./build/aether_tests
./build/aether_results results                  # regenerates results/ from the core
QT_QPA_PLATFORM=offscreen ./build/src/frontend/AetherWorkbench --screenshots docs/screenshots
```

## Actual results (executed in this environment)
- `./build/aether_tests`: **86 tests from 17 test suites ran, 86 passed** (log: `results/raw/gtest_run.log`).
- `ctest`: **100% tests passed, 0 tests failed out of 86**.
- In-application self-checks (Verification page): 8 of 8 passed when run for the screenshots (see `docs/screenshots/08_verification_*.png`).
- These results are for the Linux build only. The Windows build was not executed (see `docs/limitations.md`).

Two real defects were found by the test run and fixed before the final run (also recorded in `docs/ai_use_log.md`):
1. A test took `begin()` and `end()` from two different temporary strings (`std::string::str()`), which crashed the test binary with a segmentation fault; fixed by keeping the strings alive.
2. `Csv.EmptyAndWhitespaceFilesRejected` failed because a whitespace-only file containing newlines was not reported as "empty"; the parser was fixed.

## Coverage by requirement
| Requirement | Tests |
|---|---|
| Element stiffness / load | `ElementMatrices.*` |
| Matrix assembly | `Assembly.*` |
| Boundary conditions | `BoundaryConditions.*` |
| Analytical comparison, midpoint error | `Analytical.*` (nodes, closed-form midpoint error 1.25 C at N=10) |
| Mesh convergence | `Convergence.SecondOrderAndMonotone` (order 2.00 +/- 0.01) |
| Invalid parameters | `InvalidParameters.*`, `ElementMatrices.RejectsNonPositiveLength`, `Batch.InvalidInputsThrow` |
| STL: malformed, empty, non-finite, degenerate, duplicate, binary, truncated | `Stl.*` (20 tests) |
| CSV: empty, header-only, missing/extra/duplicate columns, malformed row, non-finite, duplicate node IDs, line numbers | `Csv.*` (16 tests) |
| Parallel agreement <= 1e-8 C, ordering, clamping, cancellation, progress | `Batch.*`, `Benchmark.*` |
| Gradient finite differences (eps = 1e-6, tol 1e-5) | `Backprop.*` (max error 1.1e-10 at the initial point) |
| Mock hardware recovery | `Hardware.*` |
| Integration | `Integration.*` (example files, watertight edges, export -> re-import), `SelfCheck.*`, `Workspace.*` |

## Known test limitations
- Concurrency tests show agreement and absence of crashes on 2 CPUs; ThreadSanitizer was not run.
- The GUI has no automated interaction tests; its behaviour was checked by driving it programmatically in `--screenshots` mode and looking at the images.
- No fuzzing; files near the 256 MiB / 100 MiB limits were not tested.
