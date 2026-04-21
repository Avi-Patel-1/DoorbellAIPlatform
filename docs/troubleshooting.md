# Troubleshooting

## Backend rejects device requests

Check that `X-Device-Token` matches `PORCHLIGHT_DEVICE_TOKEN`. The local default is `dev-device-token`.

## Dashboard is empty

Run the simulator or edge replay after the backend has started:

```bash
python simulator/porchlight_simulator.py --speed 0
```

## Benchmarks show API timing as unavailable

This is expected when the backend is not running. Start the backend and rerun `make benchmark` to collect `/health` and `/overview` timings.

## Live camera falls back to mock mode

Build the edge runtime with `-DPORCHLIGHT_WITH_OPENCV=ON` and install OpenCV development packages. The replay and mock modes do not require OpenCV.
