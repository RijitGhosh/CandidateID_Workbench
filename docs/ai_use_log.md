# AI use log

Everything below is a record of what actually happened; nothing was invented. Dates are 2026-10-31 to 2026-11-01.

## Tools and models
- Claude Code with Claude Sonnet 5.5 (model id `claude-sonnet-5-5`) in a Linux cloud container. It wrote, built and ran the code, tests and documents in this repository.
- One delegated research sub-agent of the same session (web search and page fetch) for the embedded-board facts.
- Candidate role: set the requirements, supplied the assignment brief and reviewed the results. **The candidate wrote no source code in this repository so far.** Any later edit by the candidate should be added to the table at the end.

## Prompts given by the candidate (all of them, in order)
| # | Prompt (verbatim or near-verbatim) | Result |
|---|---|---|
| 1 | Attached assignment text ("You are a senior engineering software architect ... build a complete, working engineering application ... Aether Engineering Workbench ... Do not simulate the thermal solver or multithreading. Do not create fake benchmark results ... Do not fabricate test results, Git history, AI interactions, or evidence.") plus "build it fully and fast now" | Core C++ library, tests, tools, first docs |
| 2 | "Continue and finish it fast now" | Qt 6 frontend, screenshots, results, packaging scripts, ZIP |
| 3 | Attached the official brief (PDF): "Give me the setup in steps. And check whether it is fully done and can be evaluated for getting selected also I have macbook air m1 I will not do parallel windows ..." | Gap analysis against the rubric; GitHub Actions Windows workflow; macOS script; macOS system-info code; setup steps |
| 4 | "Whatever is not done yet you finish it fast and do make fully done handwritten one ai log etc everything also make the evaluation fully done ... then give me proper set-up steps" | Self-assessment, viva guide, print-ready worksheets, this log; the assistant declined to fabricate a handwritten scan or invent the candidate's own edits (see below) |

## Relevant AI outputs (representative)
- `src/core/thermal/thermal.cpp`: 1-D FE solver; `src/core/performance/batch.cpp`: bounded worker pool with atomic work index and disjoint result slices; `src/core/neural/backprop.cpp`: hand-derived gradients; `src/frontend/*`: Qt UI.
- Embedded facts: sub-agent output stored in `docs/embedded_research_notes.md` with source URLs and UNVERIFIED flags.

## Incorrect or unsuitable AI suggestions found (all real, all fixed or disclosed)
1. **Test with undefined behaviour.** A generated test passed `begin()` and `end()` of two different temporary strings to `std::count`; the test binary segfaulted at `Export.CsvContainsHeaderAndAllNodes`. Fixed by keeping the strings alive.
2. **Wrong empty-file check.** The CSV parser trimmed only spaces, tabs and CR, so a file of only newlines was not reported as empty (`Csv.EmptyAndWhitespaceFilesRejected` failed). Fixed with `find_first_not_of(" \t\r\n")`.
3. **Obscure expected value in a unit test**, replaced by the plain formula value with a comment.
4. **JTAG-HS3 assumed to debug both boards.** Research showed the Nucleo's ST-LINK is SWD-only and the Arty's external JTAG port is documented for an HS2. The design document states this instead of hiding it.
5. **Link error:** the GUI's `AppContext` signals were not generated because the header was not listed for Qt's automatic MOC.
6. **Hex-escape bug:** the string `"\xE2\x88\x82b"` in the Neural page text swallowed the letter `b` (compiler warning). Fixed by splitting the literal. Found by a compiler warning, not by testing.
7. **Script bug:** unescaped backticks in `scripts/make_demo_log.sh` triggered command substitution ("Permission denied"). Fixed.
8. **Unverified-claim risk:** the first status message to the candidate called the work "done" while the Windows build had never run; the assistant corrected this in the next messages and in `docs/limitations.md`.

## Deliberate challenge test (executed)
Question: do the tests really protect the singular-system detection in the thermal solver?
1. Copy of the repository; both pivot checks in `solveTridiagonal` replaced by `if (false)`. **Result: all 86 tests still passed.** Reason: a second guard ("solution contains non-finite values") caught the singular system. The test only checked that *some* `SingularSystemError` was thrown.
2. Also disabled the non-finite guard. **Result: 85 passed, 1 failed** (`Assembly.SingularWithoutBoundaryConditions`: "Expected ... throws an exception of type SingularSystemError. Actual: it throws nothing.").
3. Finding: the test could not tell which guard fired. Fix: the test now also requires the message to contain "pivot". Re-run of step 1 with the strengthened test: **85 passed, 1 failed**, message "solution contains non-finite values" (so the weakened solver is now detected). The real code was not modified for the challenge; only scratch copies were.

## Validation of AI output (evidence files)
`results/raw/gtest_run.log` (86 tests), `docs/screenshots/session_log.txt` (application activity log), `results/benchmarks/`, `results/raw/backprop_derivation_values.txt`, the Wine run of the Windows core tool (`docs/testing.md`), and visual inspection of every screenshot.

## What the candidate must do (cannot be done by the AI)
- Read the code you will defend (see `docs/viva_guide.md`), re-run the tests yourself, and copy the worksheet by hand.
- Add a row here for every change you make yourself:

| Date | What I changed myself | Why | How I checked |
|---|---|---|---|
| | | | |
