# Architecture

```
src/
  core/            Qt-free C++17 numerical core (static library aether_core)
    thermal/         1-D FE solver, analytical solution, convergence study, CSV/text export
    geometry/        ASCII STL parser
    csv/             Temperature CSV parser and statistics
    performance/     batch runner (serial + bounded worker pool), benchmark, system information
    neural/          hand-derived backpropagation, central-difference check, training loop
    hardware/        SIMULATED microcontroller / FPGA / JTAG adapter mocks
    verification/    in-application self-checks (each runs its computation when called)
  application/     Qt-free workspace state (services/) + thin QObject AppContext (controllers/)
  frontend/        Qt 6 Widgets only: theme, widgets, pages, main window
  tools/           aether_results (writes results/), make_examples (writes data/)
tests/             GoogleTest suite (links aether_application, never Qt)
```

Dependency direction: `frontend -> application -> core`. The core includes no Qt header. The frontend contains drawing and event-handling code only:
every number it displays comes from a core call (`thermal::solve`, `geometry::loadAsciiStlFile`, `csv::loadTemperatureCsvFile`, `performance::runBenchmark`,
`neural::checkGradients/train`, `verification::runAllChecks`, `hardware::MockDevice`).

## Key design decisions
- **One numerical path for serial and parallel batches.** `thermal::solveInto()` is called by the interactive solver, the serial batch and every worker thread.
  Agreement between serial and parallel is therefore a test of scheduling and memory safety, not of two implementations.
- **Data-race freedom by construction.** Case `i` writes only the disjoint slice `[i*(N+1), (i+1)*(N+1))` of a pre-sized result array; each worker owns its scratch buffers;
  the only shared mutable state is `std::atomic` counters and a mutex-protected first-exception slot. Results are in input order regardless of completion order.
- **Cancellation.** A `CancelToken` (atomic flag) is checked between cases; a cancelled run reports `Cancelled` and how many cases completed. Threads are always joined.
- **Rendering.** The 3-D viewport and plots are software-rendered with `QPainter` (painter's-algorithm depth sort). This avoids GPU/OpenGL driver dependence on arbitrary Windows 11 machines
  and makes screenshots reproducible. Trade-off: it is intended for meshes up to roughly 10^5 triangles / points; larger models will be slow.
- **Theme.** `ThemeManager` owns two palettes (values from the assignment), rebuilds one application style sheet and palette, and emits `changed()`; custom-painted widgets repaint on that signal.
  The choice (dark / light / system) is saved to `aether_settings.ini` next to the executable (falls back to the per-user app-data folder if that folder is read-only).
- **Long tasks** (benchmark, self-checks) run via `QtConcurrent::run`; the status bar shows progress and a Cancel button when the task supports it. STL/CSV imports run on the UI thread (they take milliseconds at the tested sizes).
- **Resources** are found relative to the executable (`ui::findDir` walks up to four parent folders), so the package can be moved.
