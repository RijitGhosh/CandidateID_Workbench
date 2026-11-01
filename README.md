# Aether Engineering Workbench

A modular engineering workbench: ASCII-STL inspection, 1-D steady-state thermal finite-element analysis with analytical verification, serial/parallel batch computing with a real benchmark, simulated embedded-hardware adapters, and hand-derived backpropagation. C++17, CMake, Qt 6 Widgets.

> **Status of this delivery (read first).** The code, the 86-test suite and the Qt application were built and run on **Linux** (offscreen display); the numbers and screenshots in `results/` and `docs/screenshots/` come from that run. The Windows package (`bin/AetherWorkbench.exe` + Qt DLLs) is produced by the GitHub Actions workflow or `scripts\package_windows.bat` (see below) and must be launch-tested on Windows before submission. The `Launch.exe` in this folder was cross-compiled with MinGW-w64 and has not been run on Windows. See `docs/limitations.md`.

## Prerequisites
- To **run** the finished package: Windows 11 x64 only. No admin rights, installer, terminal, PowerShell, internet or editor.
- To **build** on Windows: Visual Studio 2022 Build Tools (or MinGW-w64), CMake >= 3.21, Qt 6.4 or newer (6.5 LTS recommended), Git optional.
- To build on Linux: `g++` (C++17), `cmake`, `ninja`, `qt6-base-dev`, `libgtest-dev` (see versions below).

## One-click launch (after packaging on Windows)
1. Unzip the package anywhere. 2. Double-click `Launch.exe`.
Layout: `Launch.exe`, `bin\AetherWorkbench.exe` + Qt DLLs, `data\`, `docs\`, `results\`.
Editor: `configs/editor/tasks.json` (copy into `.vscode/`, already copied here) defines the task **"Aether: launch packaged application"**, which starts `${workspaceFolder}/Launch.exe`. In VS Code or a compatible editor choose *Terminal > Run Build Task* (default task). No extension is required.

## Build the Windows package
**Option A - no Windows machine (GitHub Actions, free).** Push this repository to GitHub, open *Actions > windows-package > Run workflow*. The job (`.github/workflows/windows-package.yml`) builds with MSVC and Qt 6.5.3, runs the 86 tests, runs `windeployqt`, bundles the C++ runtime DLLs, **starts the GUI and Launch.exe on a Windows runner**, and uploads two artifacts: `AetherWorkbench_Windows_package` (the folder to ZIP and submit) and `windows_evidence` (Windows screenshots, session log, benchmark from that runner).
**Option B - on a Windows 11 machine.** `scripts\package_windows.bat C:\Qt\6.5.3\msvc2019_64` (builds, runs the tests and stops if any fail, runs `windeployqt`, assembles `dist\AetherWorkbench\Launch.exe`).

## macOS (development, rehearsal and your own benchmark)
`scripts/build_macos.sh` installs the tools with Homebrew, builds, runs the tests, runs the benchmark on your Mac (results in `results/benchmarks/macos/`) and starts the app. macOS is not the submission target; the Windows package is.

## Build from source (any platform)
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
Options: `-DAETHER_BUILD_GUI=OFF` (core, tools, tests only), `-DAETHER_BUILD_TESTS=OFF`. If GoogleTest is not installed, CMake downloads v1.14.0 (internet needed once).

## Test
```
ctest --test-dir build --output-on-failure
./build/aether_tests
```
Actual results obtained for this delivery are in `docs/testing.md` and `results/raw/gtest_run.log` (86 of 86 passed on Linux).

## Architecture
`src/core` (Qt-free numerics) <- `src/application` (workspace state, context) <- `src/frontend` (Qt UI). Details in `docs/architecture.md`.

## Supported file formats
| Format | Support |
|---|---|
| STL | **ASCII only** (`solid ... endsolid`); binary STL is detected and rejected; units not encoded; limits 256 MiB / 5,000,000 triangles |
| Temperature CSV | `node_id,x_m,y_m,z_m,temperature_C`; `#` comments; limits 100 MiB / 2,000,000 rows |
| Project | `.aewb` plain text (name, last STL/CSV path) |
| Exports | CSV and text summaries; PNG images of viewports |

## Dependency versions used for the committed results
GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, Qt 6.4.2, GoogleTest 1.14.0, MinGW-w64 GCC 13 (cross-compile of Launch.exe). See `LICENSES.md`.

## Example workflow
1. Overview: note that nothing is loaded yet. 2. Geometry: *Load example* (24 triangles), rotate/zoom, *Fit to view*. 3. Thermal Analysis > Rod FE: *Solve* with N = 20, then *Mesh study*; export CSVs. 4. Temperature result tab: *Load synthetic example*, click a point.
5. Performance: *Run benchmark*, *Save report*. 6. Hardware: discover/connect, *Inject timeout*, read telemetry (error), *Recover*. 7. Neural Network: *Compute*. 8. Verification: *Run self-checks*. 9. Toggle the theme (top-right).

## Regenerate results and screenshots
```
./build/make_examples data          # example STL/CSV
./build/aether_results results      # thermal exports, convergence, backprop values, benchmark
QT_QPA_PLATFORM=offscreen ./build/src/frontend/AetherWorkbench --screenshots docs/screenshots
```

## Known limitations
See `docs/limitations.md` (Windows package unverified, 2-CPU benchmark, ASCII STL only, simulated hardware, unverified embedded items).

## Documents
`docs/`: architecture, thermal method, embedded design (+ research notes), testing, limitations, report (`report.md`, PDF made from it), AI-use log, ownership statement, self-assessment against the rubric, viva guide, handwritten-derivation worksheets (PDF), demo walkthrough, commit log.
