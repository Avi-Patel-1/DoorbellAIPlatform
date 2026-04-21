# Architecture

Porchlight is organized around an edge-first data path.

1. Capture source produces frames from live camera, replay CSV, or mock generator.
2. Motion gate avoids work when a frame is quiet.
3. Inference backend returns object detections. The default demo uses embedded replay detections; the interface can be replaced with an ONNX Runtime backend.
4. Event aggregation performs temporal smoothing, confidence gating, cooldowns, dedupe, linger detection, package-zone persistence, and suppression.
5. Events and outbound messages are persisted in SQLite.
6. The backend ingests events and health metrics and serves the dashboard.

Suppressed events are stored intentionally. They make the decision layer inspectable and create a feedback loop for threshold tuning.
