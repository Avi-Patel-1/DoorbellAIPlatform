#!/usr/bin/env bash
set -euo pipefail
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r backend/requirements.txt pytest httpx ruff
if command -v npm >/dev/null 2>&1; then
  (cd dashboard && npm ci)
  (cd docs-site && npm ci)
fi
cmake -S edge-runtime -B edge-runtime/build -DCMAKE_BUILD_TYPE=Release
cmake --build edge-runtime/build --parallel
