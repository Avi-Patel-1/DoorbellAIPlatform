#!/usr/bin/env bash
set -euo pipefail
mkdir -p runtime-data/backend runtime-data/edge benchmarks/outputs
python -m venv .venv-lite >/dev/null 2>&1 || true
source .venv-lite/bin/activate
pip install -q -r backend/requirements.txt >/dev/null
(
  cd backend
  PORCHLIGHT_DATABASE_URL=sqlite:///../runtime-data/backend/porchlight.db \
  PORCHLIGHT_DEVICE_TOKEN=dev-device-token \
  uvicorn app.main:app --host 127.0.0.1 --port 8000
) &
BACKEND_PID=$!
trap 'kill $BACKEND_PID 2>/dev/null || true' EXIT
python - <<'PYWAIT'
import time, urllib.request
for _ in range(60):
    try:
        urllib.request.urlopen('http://127.0.0.1:8000/health', timeout=0.4).read()
        raise SystemExit(0)
    except Exception:
        time.sleep(0.25)
raise SystemExit('backend did not become ready')
PYWAIT
./edge-runtime/build/porchlight_edge --config config/edge.demo.yaml --output-json runtime-data/edge/demo_events.jsonl --simulate-offline-frames 40
python simulator/porchlight_simulator.py --speed 0 --backend-url http://127.0.0.1:8000 --device-id porch-sim-lite
printf '\nBackend is running at http://127.0.0.1:8000. Start the dashboard with: cd dashboard && npm run dev\n'
wait $BACKEND_PID
