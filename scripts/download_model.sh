#!/usr/bin/env bash
set -euo pipefail
cat <<'MSG'
Porchlight runs out of the box in lightweight replay/mock mode.

For full inference mode, place a CPU-friendly ONNX detector under:
  edge-runtime/models/detector.onnx

The C++ runtime exposes an InferenceEngine interface so ONNX Runtime integration can be enabled without changing event logic, storage, or backend ingestion. Keep model licenses in docs/third-party-notices.md.
MSG
