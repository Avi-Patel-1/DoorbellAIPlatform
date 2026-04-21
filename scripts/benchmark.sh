#!/usr/bin/env bash
set -euo pipefail
cmake -S edge-runtime -B edge-runtime/build -DCMAKE_BUILD_TYPE=Release
cmake --build edge-runtime/build --parallel
python benchmarks/scripts/run_benchmarks.py --edge-binary edge-runtime/build/porchlight_edge --scenario demo-assets/scenarios/entry_sequence.frames.csv --output-dir benchmarks/outputs
