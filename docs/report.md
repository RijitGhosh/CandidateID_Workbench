# Aether Engineering Workbench - technical report

Candidate: ____________ (ID ____________). Date of build: 2026-10-31. All numbers below were produced by the code in this repository on the environment named in section 7.

## 1. Design choices
A desktop workbench (C++17, CMake, Qt 6 Widgets) with a strict split: a Qt-free numerical **core**, a thin Qt-free **application** layer and a **frontend** that only draws and handles input.
The interface follows the supplied dark/light palettes; violet marks active navigation, primary actions and selected tools, while green/amber/red appear only for real status. Numbers use a monospaced font.
3-D and 2-D views are software-rendered (QPainter) to avoid GPU-driver dependence. Choices and trade-offs are in `docs/architecture.md`.

## 2. Architecture
`frontend -> application -> core`. The core holds thermal FE, STL and CSV parsers, batch execution, benchmark, backpropagation, hardware mocks and self-checks. The tests and two command-line tools link only the core and application libraries.
The same `solveInto()` routine serves the interactive solver, serial batch and parallel workers, so serial/parallel agreement tests scheduling and memory safety.

## 3. FEA formulation
Equation -d/dx(kA dT/dx) = qA with L = 1 m, A = 1e-4 m^2, k = 10 W/(m K), q = 1e4 W/m^3, T(0) = 100 C, T(L) = 20 C. Two-node linear elements, Ke = (kA/h)[[1,-1],[-1,1]], fe = (qAh/2)[1,1].
Tridiagonal assembly, Dirichlet conditions by right-hand-side correction plus identity rows, Thomas solve with singularity detection. Invalid parameters raise errors naming the parameter.

## 4. Analytical verification and mesh convergence
T_exact(x) = 100 - 80x/L + q/(2k) x(L-x). Nodal values are exact to round-off (about 1e-13 C), as expected for linear elements with a uniform source in 1-D. The error is therefore evaluated at element midpoints using FE interpolation.

| N | max abs midpoint error [C] | observed order |
|---|---|---|
| 10 | 1.25 | - |
| 20 | 0.3125 | 2.00 |
| 40 | 0.078125 | 2.00 |
| 80 | 0.01953125 | 2.00 |

The values equal q h^2/(8k) exactly. Plots: `results/plots/thermal_profile_*.png`, `thermal_convergence_*.png`. Exports: `results/raw/thermal_*`.

## 5. Performance results (measured; see `results/benchmarks/benchmark.md`)
Workload: 2000 independent solves, 2000 elements each, fixed seed 20260101, k in [1, 50], q in [1e3, 5e4]. Protocol: one warm-up and five timed repetitions per worker count, median reported, compute only.
Machine: virtual Intel Xeon @ 2.10 GHz, **2 logical CPUs**, 7.8 GiB RAM, GCC 13.3.0, Release `-O3`.

| Workers | Median [s] | Speedup | Efficiency | max abs diff vs serial |
|---|---|---|---|---|
| 1 | 0.0718 | 1.000 | 1.000 | 0 |
| 2 | 0.0356 | 2.018 | 1.009 | 0 |
| 3, 4 | not measured | | | exceeds the logical CPU count |

Agreement criterion 1e-8 C: met (difference 0). An efficiency slightly above 1 on a virtual machine is run-to-run noise or turbo/cache effects and should not be read as super-linear scaling; the 3- and 4-worker rows must be measured on a machine with at least that many logical CPUs.
Why speedup can be below p: thread start/join cost, queue contention, workloads too small to keep all workers busy, shared memory bandwidth and caches, hyper-threads sharing a core, other processes. Launch-to-ready is displayed separately in the application.

## 6. Embedded design
NUCLEO-F446RE (3.3 V I/O, ST-LINK SWD + VCP), Arty A7-35T (3.3 V I/O, FT2232HQ USB-JTAG/UART) and a JTAG-HS3 (Vref 1.8-5 V, drives TCK/TMS/TDI). Research found that the HS3 is not a documented fit for either board directly (Nucleo is SWD-only; Arty J8 is documented for an HS2), which the design states openly.
Voltage domains are 3.3 V throughout with a common ground; level shifting is required only for 5 V or 1.8 V peripherals. Items not confirmed from documents are marked UNVERIFIED. Three C++ mock devices (microcontroller, FPGA, JTAG) support discovery, capabilities, connect, disconnect, telemetry and a recoverable timeout; they are labelled SIMULATED in code and UI. Details: `docs/embedded_design.md`.

## 7. Test outcomes
86 GoogleTest tests passed; 8 of 8 in-application self-checks passed; gradient check maximum error 1.12e-10 (limit 1e-5); loss falls from 1.1825 to 1.08e-20 over 101 updates (1 hand-checkable update plus 100 more). Environment and commands: `docs/testing.md`.
Defects found by testing: a segfault caused by a faulty test and a whitespace-only CSV mishandled by the parser; both were fixed.

## 8. Limitations
The Windows GUI package was not built or run here; Launch.exe and the core CLI were only cross-compiled. Benchmark limited to 2 logical CPUs. STL is ASCII-only; CSV nodes are not registered to the STL; the viewport is software-rendered; hardware is simulated; embedded facts partly UNVERIFIED. Full list: `docs/limitations.md`.

## 9. Future improvements
Build and test on Windows 11 with CI; binary STL; mesh-to-result registration; GPU viewport for large models; ThreadSanitizer runs; real serial adapters (read-only first); non-uniform and 2-D/3-D FE; more benchmark workloads and a persisted benchmark history.

## Appendix (2 pages)
Handwritten derivation: print/copy `docs/handwritten_derivation_worksheet.md` by hand, then scan it as the appendix.
