#!/usr/bin/env bash
# macOS (Apple Silicon or Intel): build, test, run the benchmark natively and start the app.
# This is for development and for measuring the benchmark on your own machine. The submission target is still the Windows package.
set -euo pipefail
cd "$(dirname "$0")/.."
command -v brew >/dev/null || { echo "Install Homebrew first: https://brew.sh"; exit 1; }
brew install cmake ninja qt googletest >/dev/null
cmake -S . -B build-mac -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix googletest)"
cmake --build build-mac
ctest --test-dir build-mac --output-on-failure
mkdir -p results/benchmarks/macos
./build-mac/aether_results results/benchmarks/macos     # real benchmark on this Mac (also rewrites its raw/ copies in that folder)
echo "Starting the application (close the window to finish)..."
./build-mac/src/frontend/AetherWorkbench
