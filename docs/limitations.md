# Limitations (known and honest)

**Delivery / platform**
- The development container is Linux with 2 logical CPUs. The Qt application was built and run here (offscreen platform) and all screenshots come from that run.
- **A Windows build of the GUI (`bin/AetherWorkbench.exe` and Qt DLLs) was NOT produced here**, because no Qt for Windows is available in this environment. `Launch.exe` and the C++ core (as a command-line tool, `aether_results.exe`) were cross-compiled with MinGW-w64 and confirmed to be valid Windows PE files, but **neither was executed on Windows**. The complete package is produced by `.github/workflows/windows-package.yml` (GitHub-hosted Windows runner) or `scripts\package_windows.bat` on a Windows 11 machine (see README). The workflow itself has not been run by the author of this repository yet: until a run succeeds and its output has been started on a Windows PC, treat the one-click requirement as unverified. A hosted runner proves the program starts on Windows; it does not replace trying a clean Windows 11 PC.
- Not tested on Windows 11: file dialogs, DPI scaling, `windeployqt` output, the Launch.exe -> bin\AetherWorkbench.exe hand-off.

**Benchmark**
- Results in `results/benchmarks/` were measured on a 2-logical-CPU virtual machine. The 3- and 4-worker rows are reported as "not measured" because the requirement caps workers at the logical CPU count. Re-run on your own machine for the 1-4 worker table (Performance page, or `aether_results`).
- Physical core count is taken from the OS (`cpu cores` / Windows API); virtual machines may report it differently from the host.
- Launch-to-ready is measured from the first line of `main()` to the main window being shown; OS process creation and DLL loading before `main()` are not included.

**Features**
- STL: ASCII only. Binary STL is detected and rejected with a message. No units, no multi-solid files, no repair. Facet normals in the file are parsed but ignored (normals are recomputed for shading).
- CSV: no quoted fields. Nodes are not registered to the STL.
- The 3-D viewport is software-rendered: no GPU acceleration, no lighting beyond flat shading, point picking uses the nearest projected point within 14 px.
- The thermal solver is 1-D, uniform mesh, constant properties, Dirichlet conditions only.
- Hardware: all three devices are simulations. No serial/USB/JTAG I/O exists in the code.
- Project files store only the project name and the last STL/CSV paths (plain text `.aewb`).
- Theme "System" detects the OS light/dark setting once at start-up.

**Documentation**
- Embedded electrical data comes from vendor documents read through a summarising fetch tool; items not seen are marked UNVERIFIED in `docs/embedded_design.md`. Re-check them against the PDFs before building hardware.
- `docs/ai_use_log.md` and `docs/ownership_statement.md` contain fields only the candidate can truthfully complete.
