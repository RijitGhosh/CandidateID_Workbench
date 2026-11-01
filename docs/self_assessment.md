# Self-assessment against the published 100-point rubric

Scores are the author's honest estimates from the evidence in this repository, using the brief's anchors (0 absent, 25 major faults, 50 partly, 75 mostly correct with minor gaps, 100 complete and reproducibly supported). They are not a promise of the official score. "Pending" means the evidence does not exist yet.

| Criterion (max) | Estimate | Evidence | Known gaps |
|---|---|---|---|
| Interface / design (8) | 6.5-7 | `docs/screenshots/*` (both themes, 10 pages states), validation messages, status bar with cancel, theme toggle top-right | Not tested at Windows DPI scaling; fixed minimum window 1100x700 |
| C++ backend and testing (12) | 10-11 | 86 tests (`results/raw/gtest_run.log`), positive/negative/numerical/malformed/empty/non-finite/parallel/gradient/mock-recovery; challenge test in `docs/ai_use_log.md` | No ThreadSanitizer; no fuzzing |
| Folder architecture (6) | 5 | `docs/architecture.md`; core is Qt-free; relative paths; versions in `LICENSES.md` | No lock file (dependencies are few and listed) |
| Code readability (6) | 4.5-5 | Small focused functions, comments on non-obvious choices | Some long UI constructors |
| Version control (5) | 3-4 | 20+ commits with descriptive messages, `docs/commit_log.txt`, Git bundle | Commits were created in two sessions, not spread over days; add your own commits as you review and edit |
| Thermal FEA and adoption (14) | 12-13 | Formulation and BCs (6): `thermal.cpp`, tests; verification and convergence (5): `results/raw/thermal_convergence.csv`, plots; assumptions and learning (3): `docs/thermal_method.md` incl. debugging insight | You must be able to explain the derivation yourself (`docs/viva_guide.md`) |
| CAD / ANSYS visualization (10) | 8.5-9 | Both imports, legend and units, provenance (`data/provenance.md`), malformed/empty/non-finite handling, tested size limits | Large-file limits (256/100 MiB) not tested at the limit |
| Multicore performance (10) | 7.5-9 | Real bounded worker pool, fixed seed, 1+5 repetitions, median, agreement 0 C (`results/benchmarks/`) | Measured on 2 CPUs only until the Windows CI/your Mac benchmark is added; 3-4 worker rows pending |
| Embedded integration (8) | 5-6 | `docs/embedded_design.md`, three mocks, timeout + recovery in UI and tests | JTAG-HS3 pin map and a few board facts UNVERIFIED; HS3 not documented for either board |
| Backpropagation (7) | 3 now; 6-7 after you copy the worksheet by hand | C++ gradients, 1.1e-10 error, loss plot (3 points); handwritten (4 points): `docs/worksheet_to_copy_by_hand.pdf` | The handwritten pages are yours to produce; the brief requires you to defend them |
| Prompt engineering / AI judgement (8) | 5-6 | `docs/ai_use_log.md`: real prompts, 8 real faults, executed challenge test | Your own review and edits must be added; a thin candidate section lowers this score |
| One-click delivery / report (6) | 3 now; 5 after Windows evidence | Launch.exe, CI workflow, report, limitations | **Windows launch not yet verified on Windows.** |
| **Total (100)** | **about 66-80** | | The brief's gates: C++ >= 6/12 (met), FEA >= 7/14 (met), delivery >= 3/6 (needs the Windows launch), a successful standalone launch (**pending**) |

Honest reading of the shortlist rules: 80-100 strong shortlist, 50-80 interview shortlist. The two things that can move this down sharply are a failed Windows launch and being unable to explain the code in the verification discussion.

## Effort record (required by the brief: "record actual effort")
Target 40-50 hours. The AI-assisted build took two working sessions. Add your own hours here: reading and understanding the code ____ h, handwritten sheet ____ h, Windows testing ____ h, other ____ h.
