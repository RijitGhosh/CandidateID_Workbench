# Benchmark report (measured on this machine)

| Item | Value |
|---|---|
| CPU model | Intel(R) Xeon(R) Processor @ 2.10GHz |
| Physical cores | 2 |
| Logical cores | 2 |
| RAM | 7.84 GiB |
| OS | Linux |
| Compiler | GCC 13.3.0 |
| Build type / flags | Release / ` -O3 -DNDEBUG ` |
| Workload | independent 1-D thermal FE solves (L=1 m, A=1e-4 m^2, T(0)=100 C, T(L)=20 C) |
| Case count | 2000 |
| Mesh size | 2000 elements |
| Random seed | 20260101 (k in [1,50], q in [1e3,5e4]) |
| Warm-up policy | 1 untimed run per worker count |
| Repetitions | 5 timed runs; median reported |
| Timed region | compute only (excludes file I/O, rendering, compilation, UI); includes thread start/join |

| Workers | Median time [s] | Speedup T1/Tp | Efficiency | max abs diff vs serial [C] | Note |
|---|---|---|---|---|---|
| 1 | 0.071782 | 1.000 | 1.000 | 0.00e+00 | |
| 2 | 0.035576 | 2.018 | 1.009 | 0.00e+00 | |
| 3 | - | - | - | - | not measured: 3 workers exceeds 2 logical CPU(s) |
| 4 | - | - | - | - | not measured: 4 workers exceeds 2 logical CPU(s) |

Agreement check (<= 1e-8 C): PASS
