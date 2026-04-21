# Porchlight

**Edge AI Smart Doorbell and Intelligent Event Detection Platform**

Porchlight is a Linux-first smart doorbell and entry-monitoring platform that performs event understanding at the edge, suppresses nuisance alerts, queues locally when offline, and syncs explainable event history plus device telemetry to a cloud-style backend and dashboard.

The project is designed to be reviewable on a normal laptop without special hardware while still preserving a Raspberry Pi-friendly edge architecture. Replay and mock modes exercise the same event logic, local SQLite storage, offline queue, backend ingestion, benchmark harness, and dashboard used by a live device deployment.

![Porchlight dashboard overview](docs-site/public/assets/dashboard-overview.svg)

## Why this exists

Most entry cameras can detect motion; fewer can decide whether that motion should interrupt someone. Doorbells are noisy environments: passing headlights, sidewalk activity, wind, shadows, animals, repeated visitors, and package drop-offs all look like activity. Porchlight focuses on the device-engineering problem behind a better experience:

- run useful logic on a constrained Linux device;
- avoid blindly publishing every frame or motion event;
- keep working when the network is unreliable;
- expose enough telemetry to debug the device in the field;
- make benchmark and demo results reproducible from bundled scenarios.

## Key features

- **C++17 edge runtime:** capture abstraction, replay/mock/live modes, modular inference interface, motion gate, event aggregation, SQLite persistence, HTTP publishing, health telemetry, and bounded retry behavior.
- **Event intelligence layer:** temporal smoothing, confidence gates, person-at-door detection, visitor linger detection, package delivered/removed heuristics, package-zone support, cooldown and dedupe windows, quiet-hours and home/away policy hooks, and structured explanations.
- **Offline-first storage:** local event table and outbound queue are stored in SQLite. Events and health messages are retried with backoff after backend outages.
- **FastAPI backend:** token-authenticated device ingestion, event search/filtering, device registration, health metrics, exports, benchmark storage, OpenAPI docs, and WebSocket updates.
- **React dashboard:** overview metrics, event timeline, event detail review, device health trends, settings/profile editor, benchmark page, and replay story.
- **GitHub Pages docs site:** product landing page, architecture diagrams, benchmark charts, setup notes, API summary, FAQ, and static demo preview.
- **Reproducible benchmarks:** scripts run the bundled replay scenario, compute latency/FPS/alert counts, simulate offline queue recovery, emit JSON/Markdown/SVG artifacts, and can sample API response timing when the backend is running.

## Architecture overview

![Porchlight architecture](docs-site/public/assets/architecture.svg)

The edge device is the source of truth for local event decisions. The backend is useful for history, review, exports, and dashboard views, but the device keeps detecting and queueing when upstream services are unavailable.

```text
camera/replay/mock source
  -> motion gate
  -> inference backend
  -> temporal event aggregation
  -> suppression / package-zone / linger policy
  -> SQLite event store + outbound queue
  -> backend API
  -> dashboard + docs/demo artifacts
```

The default demo path uses replay detections embedded in `demo-assets/scenarios/entry_sequence.frames.csv`. This keeps the project runnable without a camera, GPU, or model download. The inference boundary is isolated behind `InferenceEngine`, so a CPU-friendly ONNX Runtime detector can be added without changing storage, event logic, backend ingestion, or dashboard code.

## Tech stack

| Layer | Implementation |
|---|---|
| Edge runtime | C++17, CMake, SQLite, POSIX HTTP client, optional OpenCV live source |
| Event logic | Motion gate, temporal confidence history, suppression policy, package-zone monitor, linger logic |
| Backend | Python 3.11+, FastAPI, Pydantic, SQLAlchemy, SQLite by default, optional Postgres-ready config path |
| Dashboard | React, TypeScript, Vite, Tailwind CSS |
| Docs site | Static Vite/React site deployable to GitHub Pages |
| Demo/benchmarks | Python simulator, C++ replay runner, benchmark JSON/Markdown/SVG artifacts |
| Local stack | Docker Compose with backend, dashboard, simulator, and MQTT broker |

## Quick start

Prerequisites for the native path:

- CMake 3.20+
- a C++17 compiler
- SQLite development headers
- Python 3.11+
- Node.js 20+ for dashboard/docs builds

```bash
git clone <your-fork-url> porchlight
cd porchlight

make edge
make benchmark
```

The edge runtime binary is created at:

```text
edge-runtime/build/porchlight_edge
```

Run the replay directly:

```bash
./edge-runtime/build/porchlight_edge \
  --config config/edge.demo.yaml \
  --no-publish \
  --output-json runtime-data/edge/replay_events.jsonl \
  --benchmark-json benchmarks/outputs/manual_edge_stats.json
```

## Demo modes

Porchlight supports three demo/execution modes.

### 1. Live webcam mode

Live mode is selected with `source_mode: live`. It falls back to mock mode unless the edge runtime is built with OpenCV support.

```bash
cmake -S edge-runtime -B edge-runtime/build -DCMAKE_BUILD_TYPE=Release -DPORCHLIGHT_WITH_OPENCV=ON
cmake --build edge-runtime/build --parallel
./edge-runtime/build/porchlight_edge --config config/edge.pi.yaml
```

### 2. Demo replay mode

Replay mode reads deterministic frame records from `demo-assets/scenarios/entry_sequence.frames.csv`. This is the default for the benchmark harness and local demo.

```bash
./edge-runtime/build/porchlight_edge --config config/edge.demo.yaml --no-publish
```

### 3. Mock/synthetic mode

Mock mode generates the same story in-process and does not require a camera, model, backend, or replay file.

```bash
./edge-runtime/build/porchlight_edge --config config/edge.mock.yaml
```

## Running the full local stack

The Docker Compose stack starts:

- backend API on `http://localhost:8000`
- dashboard on `http://localhost:5173`
- simulator that posts seeded device events and metrics
- Mosquitto MQTT broker for future notification adapters

```bash
make demo
```

For a no-Docker path:

```bash
make edge
make demo-lite
```

`demo-lite` starts the backend, runs an edge replay with a simulated offline window, then runs the Python simulator to populate the dashboard. Start the dashboard separately when using the lite path:

```bash
cd dashboard
npm install
npm run dev
```

## Raspberry Pi / Linux deployment notes

Install system dependencies on the device:

```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libsqlite3-dev
```

Build the edge runtime:

```bash
cmake -S edge-runtime -B edge-runtime/build -DCMAKE_BUILD_TYPE=Release
cmake --build edge-runtime/build --parallel
```

Prepare runtime directories and config:

```bash
sudo useradd --system --home /var/lib/porchlight --shell /usr/sbin/nologin porchlight || true
sudo mkdir -p /opt/porchlight/bin /etc/porchlight /var/lib/porchlight/snapshots
sudo cp edge-runtime/build/porchlight_edge /opt/porchlight/bin/
sudo cp config/edge.pi.yaml /etc/porchlight/edge.yaml
sudo cp scripts/systemd/porchlight-edge.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now porchlight-edge
```

Production deployments should add TLS, signed device provisioning, model packaging, log rotation, watchdog integration, privacy-aware media retention, and an OTA update strategy.

## Benchmark methodology

The benchmark harness is intentionally reproducible and file-based. It runs the C++ edge replay over the bundled scenario, stores emitted event JSONL, and writes a machine-readable report.

```bash
make benchmark
```

Outputs:

```text
benchmarks/outputs/latest.json
benchmarks/outputs/summary.md
benchmarks/outputs/latest_events.jsonl
benchmarks/outputs/latency_chart.svg
benchmarks/outputs/alert_mix.svg
```

The suite measures:

- replay frame count and FPS;
- average and p95 processing latency;
- meaningful alerts versus suppressed decisions;
- threshold sweep behavior across sensitive/balanced/low-noise profiles;
- synthetic offline queue peak, final depth, and flush success;
- backend `/health` and `/overview` timings when the API is reachable;
- process RSS snapshot for the benchmark process.

The benchmark does not claim Raspberry Pi performance unless it is run on a Raspberry Pi. Rerun it on target hardware before making hardware-specific statements.

## Benchmark results

The checked-in report below was produced with:

```bash
python benchmarks/scripts/run_benchmarks.py \
  --edge-binary edge-runtime/build/porchlight_edge \
  --scenario demo-assets/scenarios/entry_sequence.frames.csv \
  --output-dir benchmarks/outputs
```

| Metric | Value |
|---|---:|
| Replay frames processed by C++ edge runtime | 180 |
| Replay FPS | 17629.160 |
| Average edge processing latency | 0.00065 ms |
| P95 edge processing latency | 0.00015 ms |
| Meaningful alerts | 5 |
| Suppressed decisions | 10 |
| Queue peak in reconnect simulation | 6 |
| Queue final depth in reconnect simulation | 0 |
| Queue flush success | true |

The API timing section was skipped in this run because the backend was not running. Start the backend and rerun `make benchmark` to include backend response timing samples.

![Latency chart](benchmarks/outputs/latency_chart.svg)

![Alert mix chart](benchmarks/outputs/alert_mix.svg)

## Dashboard walkthrough

The dashboard is an operations console for device and event review.

### Overview

Shows online/offline device state, events today, meaningful versus suppressed decisions, average processing latency, queue depth, and recent device health trends.

### Event timeline

Searchable and filterable table of surfaced and suppressed events. Suppression is not hidden; it is stored to make threshold tuning and decision review possible.

### Event detail

Shows confidence, latency, confidence history, device metadata, trigger reasons, raw metadata, and review actions such as `reviewed` or `false_positive`.

### Device page

Shows each registered device, last heartbeat, firmware version, event counts, queue depth, FPS trend, and memory trend.

### Settings page

Lets reviewers inspect and tune threshold profile values such as motion threshold, person threshold, package threshold, linger seconds, and dedupe windows. The package-zone preview mirrors the runtime config.

### Benchmark page

Displays benchmark runs stored by the backend and links the local workflow used to produce JSON/Markdown/SVG reports.

## Event intelligence and suppression logic

Porchlight treats event decisions as first-class records. A surfaced alert and a suppressed decision have the same core schema, including explanation metadata.

Core rules include:

- **Motion gate:** quiet frames are dropped before inference unless detections are already present in replay/mock mode.
- **Temporal smoothing:** person and package confidence history is kept across recent frames. A single-frame spike does not immediately alert.
- **Person-at-door:** emits when a person detection intersects the entry zone and remains above threshold across recent frames.
- **Visitor linger:** emits when a tracked person remains in the entry zone beyond `linger_seconds`.
- **Package delivered:** emits when a package-sized object enters the configured package zone and persists for multiple frames.
- **Package removed:** emits when a previously tracked package disappears from the package zone for a persistence window.
- **Dedupe/cooldown:** repeated person/package decisions inside the dedupe window are stored as `duplicate_event_suppressed`.
- **Low-confidence suppression:** person-like detections below threshold are stored as suppressed events with a reason.
- **Policy hooks:** quiet-hours and home/away mode can suppress non-critical person alerts without disabling package logic.

Example event explanation:

```text
Person detected in entry zone with stable confidence above threshold; Temporal smoothing confirmed detection across recent frames
```

## Sample event JSON

```json
{
  "id": "porch-pi-001-package_delivered-9500-95",
  "device_id": "porch-pi-001",
  "type": "package_delivered",
  "severity": "high",
  "timestamp_ms": 9500,
  "confidence": 0.712,
  "suppressed": false,
  "reasons": [
    "Package-sized object entered the configured package zone",
    "Object persisted across consecutive frames before alerting"
  ],
  "explanation": "Package-sized object entered the configured package zone; Object persisted across consecutive frames before alerting",
  "confidence_history": [0.684, 0.698, 0.712],
  "snapshot_ref": "snapshots/porch-pi-001-package_delivered-9500-95.jpg",
  "clip_ref": "clips/porch-pi-001-package_delivered-9500-95.mp4",
  "processing_latency_ms": 0.001,
  "metadata": {
    "frame_id": 95,
    "scenario": "package_delivered",
    "motion_score": 0.33,
    "package_zone": { "x": 0.52, "y": 0.55, "w": 0.33, "h": 0.36 }
  }
}
```

## Sample logs

```json
{"level":"info","message":"event emitted","event_id":"porch-pi-001-person_at_door-2000-20","type":"person_at_door","confidence":"0.742","queue_depth":"1","reason":"Person detected in entry zone with stable confidence above threshold; Temporal smoothing confirmed detection across recent frames"}
{"level":"info","message":"event suppressed","event_id":"porch-pi-001-duplicate_event_suppressed-2500-25","type":"duplicate_event_suppressed","confidence":"0.824","queue_depth":"2","reason":"Duplicate person event suppressed because a similar alert was emitted inside the dedupe window"}
```

## Repo structure

```text
.
├── backend/                  # FastAPI backend, SQLAlchemy models, tests
├── benchmarks/               # Benchmark harness and generated output artifacts
├── config/                   # Edge configs and threshold profiles
├── dashboard/                # React + TypeScript operations console
├── demo-assets/              # Replay scenario and demo asset notes
├── docs/                     # Engineering docs and deployment notes
├── docs-site/                # GitHub Pages product/docs site
├── edge-runtime/             # C++17 Linux edge runtime
├── reports/                  # Exported diagrams/screenshots for docs
├── scripts/                  # Setup, demo, benchmark, docs asset helpers
├── simulator/                # Synthetic multi-device event/telemetry simulator
├── docker-compose.yml
├── Makefile
└── README.md
```

## Development workflow

```bash
make setup        # create local envs, install deps, build edge runtime
make edge         # build C++ edge runtime
make edge-test    # run C++ unit tests
make backend-test # run FastAPI tests
make test         # edge + backend tests
make benchmark    # run replay benchmark and write artifacts
make assets       # export docs diagrams and benchmark visuals
make docs-build   # build docs site
make clean        # remove local build/runtime artifacts
```

## Testing

C++ tests cover suppression and event aggregation behavior:

```bash
make edge-test
```

Backend tests cover health, event ingestion/filtering, and metric ingestion:

```bash
cd backend
python -m pytest -q
```

The benchmark suite is also a smoke test for the replay source, edge runtime, event emission, and artifact generation:

```bash
make benchmark
```

## Configuration

Primary configs live in `config/`.

Important edge keys:

| Key | Purpose |
|---|---|
| `device_id` | Stable device identity used in events, metrics, and queue payloads |
| `source_mode` | `live`, `replay`, or `mock` |
| `replay_path` | CSV replay source path |
| `backend_url` | Backend base URL for event and metric publishing |
| `auth_token` | Device token sent as `X-Device-Token` |
| `store_path` | Local SQLite event and queue database |
| `motion_threshold` | Minimum motion score for processing frames without detections |
| `person_threshold` | Confidence threshold for person decisions |
| `package_threshold` | Confidence threshold for package decisions |
| `linger_seconds` | Person dwell time before visitor linger alert |
| `dedupe_window_seconds` | Window used to suppress duplicate events |
| `quiet_hours_*` | Quiet-hours policy controls |
| `home_mode` | `away`, `home`, or `disarmed` policy hook |
| `package_zone_*` | Normalized region of interest for package persistence |

## API summary

FastAPI exposes an OpenAPI spec automatically at `/openapi.json` and interactive docs at `/docs` while running.

| Method | Path | Purpose |
|---|---|---|
| `GET` | `/health` | Service health check |
| `GET` | `/overview` | Dashboard rollup data |
| `POST` | `/devices` | Register/update a device |
| `GET` | `/devices` | List devices with latest metrics and counts |
| `POST` | `/devices/{id}/metrics` | Ingest device health metrics |
| `GET` | `/devices/{id}/metrics` | Fetch device health history |
| `POST` | `/events` | Ingest event or suppressed decision |
| `GET` | `/events` | Filter/search event timeline |
| `GET` | `/events/{id}` | Fetch one event detail record |
| `PATCH` | `/events/{id}/label` | Mark review label such as false positive |
| `GET` | `/benchmarks` | List stored benchmark runs |
| `POST` | `/benchmarks` | Store benchmark summary |
| `GET` | `/exports/events.csv` | Export event history to CSV |
| `GET` | `/exports/events.json` | Export event history to JSON |
| `WS` | `/ws/events` | Live event/metric updates |

Device ingestion endpoints require:

```text
X-Device-Token: dev-device-token
```

Change the token with `PORCHLIGHT_DEVICE_TOKEN` and `.env` or Docker Compose environment variables.

## Docs site and GitHub Pages

Build the docs site locally:

```bash
make assets
make docs-build
```

Preview it:

```bash
cd docs-site
npm run preview
```

GitHub Pages deployment is configured in `.github/workflows/pages.yml`. For repository Pages under `/repo-name/`, the workflow sets `PORCHLIGHT_DOCS_BASE` automatically.

## Lightweight demo mode vs. full inference mode

**Lightweight demo mode** is the default. It uses replay/mock detections embedded in the scenario bundle so the system works without camera hardware, a GPU, or external model weights. It still exercises the C++ runtime, SQLite queue, event aggregation, backend, dashboard, simulator, and benchmarks.

**Full inference mode** is the intended production extension. Add an ONNX Runtime-backed implementation of `InferenceEngine`, place licensed model assets under `edge-runtime/models/`, and keep the output labels compatible with the event aggregator (`person`, `package`, `vehicle`). The rest of the system does not need to change.

## Troubleshooting

### Backend returns 401

The device token is wrong. The local default is `dev-device-token`.

### Dashboard is empty

Start the simulator or run an edge replay after the backend is up:

```bash
python simulator/porchlight_simulator.py --speed 0
```

### Benchmark API timing is unavailable

Start the backend first, then rerun:

```bash
cd backend
uvicorn app.main:app --reload
# in another terminal
make benchmark
```

### Live camera falls back to mock mode

Build with OpenCV support:

```bash
cmake -S edge-runtime -B edge-runtime/build -DPORCHLIGHT_WITH_OPENCV=ON
cmake --build edge-runtime/build --parallel
```

### SQLite database path errors

Create runtime directories or use a config path under the project:

```bash
mkdir -p runtime-data/edge runtime-data/backend
```

## Engineering tradeoffs

- **Replay-first demo:** deterministic replay makes the project reliable in interviews and CI while preserving the edge pipeline boundaries.
- **SQLite at the edge:** SQLite is enough for single-device local durability and makes queue recovery inspectable with normal tooling.
- **Suppression as data:** suppressed events are retained because they are essential for threshold tuning and alert-quality review.
- **Plain HTTP client in C++:** the edge runtime avoids a heavy dependency for local demos. Production deployments should use TLS and a hardened client library.
- **Optional model runtime:** the event system is model-agnostic so the project remains runnable without distributing large weights.
- **Light dashboard dependencies:** custom cards and SVG charts keep the UI easy to build and inspect.

## Roadmap / future improvements

- ONNX Runtime detector integration with model packaging and license notice.
- Signed device provisioning and per-device API tokens.
- TLS and certificate pinning for edge-to-backend publishing.
- Media clip generation and privacy-aware retention policies.
- MQTT/Webhook notification adapters.
- OTA update manifest and rollback flow.
- Multi-device fleet view with stale heartbeat alerting.
- Hardware watchdog and camera failure recovery policies.
- Package-zone calibration tool using live frame snapshots.
- Postgres migration profile for hosted backend deployments.

## License and third-party notices

This project is licensed under the MIT License. See `LICENSE`.

Third-party dependency and model notes are tracked in `docs/third-party-notices.md`. No model weights are bundled by default.
