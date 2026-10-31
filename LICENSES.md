# Licences and dependencies

| Component | Version used for the results in this repository | Licence | Use |
|---|---|---|---|
| Qt 6 Widgets, Gui, Core, Concurrent | 6.4.2 (Ubuntu 24.04 package `qt6-base-dev`) | LGPL v3 / GPL v3 / commercial | Desktop UI only. Dynamically linked; see the Qt LGPL notes below. |
| GoogleTest | 1.14.0 (`libgtest-dev` 1.14.0-1) | BSD 3-Clause | Tests only; not shipped |
| GCC / MinGW-w64 | GCC 13.3.0 (Linux build); MinGW-w64 GCC 13 posix (cross-compiled Launch.exe and CLI test build) | GPL with runtime library exception | Compiler |
| CMake / Ninja | 3.28.3 / 1.11.1 | BSD 3-Clause / Apache 2.0 | Build |
| C++ standard library | libstdc++ (GCC 13) | GPL v3 with GCC runtime library exception | Linked statically into Launch.exe |

All application source in `src/`, `tests/`, `scripts/` and the example data was written for this project (see `docs/ownership_statement.md`).
No fonts, icons or images are bundled; the logo and theme icons are drawn in code.

## Qt LGPL obligations (when you distribute the Windows package)
Qt is used under LGPL v3 as shared libraries (the DLLs placed by `windeployqt`). Keep the Qt DLLs as separate files (do not statically link Qt),
include this notice and the LGPL v3 text (https://www.gnu.org/licenses/lgpl-3.0.html), and do not prevent users from replacing the Qt DLLs.
This is a summary, not legal advice; check the Qt licensing page for your exact situation.
