#!/usr/bin/env bash
# Linux/dev build + tests + regenerate results/ and example data. (Engineering calculations happen in the C++ binaries only.)
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/make_examples data
./build/aether_results results
