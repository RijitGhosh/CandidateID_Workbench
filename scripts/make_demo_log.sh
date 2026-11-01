#!/usr/bin/env bash
# Produces docs/demo_walkthrough.md: a timestamped list of the screenshots written by the real application run.
set -euo pipefail
cd "$(dirname "$0")/.."
{
  echo "# Demonstration walkthrough (screenshots, not a video)"
  echo
  echo "Screenshots were written by \`AetherWorkbench --screenshots\` (the application driving itself: loads the example STL, solves, loads the CSV, runs the benchmark, injects and recovers a simulated timeout, computes backprop, runs self-checks). Timestamps are file modification times; the application activity log of the same run is in `docs/screenshots/session_log.txt`."
  echo
  echo "| Time (local) | Screenshot | What it shows |"
  echo "|---|---|---|"
  for f in docs/screenshots/*_dark.png docs/screenshots/*_light.png; do
    t=$(date -r "$f" '+%Y-%m-%d %H:%M:%S')
    n=$(basename "$f" .png)
    case "$n" in
      01_*) w="Overview before anything is loaded";; 02_*) w="Geometry: example bracket STL, bounding box, triangle counts";;
      03_*) w="Thermal FE: FE vs analytical profile, mesh convergence, midpoint table";; 04_*) w="Temperature point cloud with legend and statistics (synthetic CSV, label shown)";;
      05_*) w="Performance: measured benchmark table and speedup plot";; 06_*) w="Hardware: three SIMULATED devices after fault injection and recovery";;
      07_*) w="Neural network: gradient check, update, loss plot";; 08_*) w="Verification: self-checks executed";; 09_*) w="Reports and documentation browser";;
      10_*) w="Overview after the session: real module state and activity";; *) w="";;
    esac
    echo "| $t | $f | $w |"
  done
} > docs/demo_walkthrough.md
