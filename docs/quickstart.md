# Quick start

```bash
make edge
make benchmark
make demo
```

Use `make demo-lite` when Docker is not available. It starts the backend in a local Python virtual environment, runs the edge replay, and then runs the simulator.

The dashboard is served from `dashboard/` with Vite. The backend OpenAPI document is available at `/docs` while the FastAPI service is running.
