# Demo scenario bundle

`entry_sequence.frames.csv` is a deterministic replay stream used by the edge runtime, simulator, and benchmark harness. Each row represents one frame at 10 FPS and includes a motion score plus optional detection summaries.

The sequence covers:

- quiet porch / no event
- person approaches door
- visitor lingers near entry
- low-confidence motion suppressed
- package delivered inside the configured package zone
- repeated motion deduplicated
- package removed after a pickup

The project can also ingest user-provided clips through the live/replay interfaces. The bundled CSV keeps the demo runnable without a camera, GPU, or model download.
