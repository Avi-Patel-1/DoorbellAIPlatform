# Deployment notes

## Raspberry Pi / Linux

Build on the target device where possible:

```bash
sudo apt-get install -y cmake g++ libsqlite3-dev
cmake -S edge-runtime -B edge-runtime/build -DCMAKE_BUILD_TYPE=Release
cmake --build edge-runtime/build --parallel
```

Copy the binary to `/opt/porchlight/bin`, install `scripts/systemd/porchlight-edge.service`, and place `config/edge.pi.yaml` under `/etc/porchlight/edge.yaml`.

## Model integration

The default project uses replay/mock detections so the demo does not require a model download. A production build can implement `InferenceEngine` with ONNX Runtime and package a CPU-friendly detector under `edge-runtime/models/`.
