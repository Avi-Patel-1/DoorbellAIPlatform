from fastapi.testclient import TestClient

from app.main import app
from app.db import init_db


init_db()
client = TestClient(app)
TOKEN = {"X-Device-Token": "dev-device-token"}


def test_health() -> None:
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json()["status"] == "ok"


def test_ingest_event_and_filter() -> None:
    payload = {
        "id": "pytest-event-1",
        "device_id": "pytest-device",
        "type": "person_at_door",
        "severity": "medium",
        "timestamp_ms": 123456,
        "confidence": 0.87,
        "suppressed": False,
        "reasons": ["stable confidence"],
        "explanation": "stable confidence",
        "metadata": {"scenario": "test"},
    }
    response = client.post("/events", json=payload, headers=TOKEN)
    assert response.status_code == 200
    assert response.json()["id"] == payload["id"]

    listed = client.get("/events", params={"device_id": "pytest-device"})
    assert listed.status_code == 200
    assert any(item["id"] == payload["id"] for item in listed.json())


def test_metric_ingest() -> None:
    metric = {
        "device_id": "pytest-device",
        "timestamp_ms": 123999,
        "fps": 10.4,
        "cpu_load_1m": 0.3,
        "memory_rss_mb": 60.2,
        "queue_depth": 2,
        "network_status": "online",
        "uptime_seconds": 42,
    }
    response = client.post("/devices/pytest-device/metrics", json=metric, headers=TOKEN)
    assert response.status_code == 200
    assert response.json()["queue_depth"] == 2
