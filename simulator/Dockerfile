FROM python:3.11-slim
WORKDIR /app
COPY simulator/porchlight_simulator.py ./porchlight_simulator.py
COPY demo-assets ./demo-assets
CMD ["python", "porchlight_simulator.py", "--scenario", "demo-assets/scenarios/entry_sequence.frames.csv", "--speed", "30"]
