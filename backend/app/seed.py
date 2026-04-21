from __future__ import annotations

import argparse
import json
import os
import random
import time
from datetime import datetime, timezone

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

from app import models, schemas
from app.db import Base, SessionLocal
from app.services import ingest_event, ingest_metric


def seed_demo(db, replace: bool = False) -> None:
    if replace:
        db.query(models.Event).delete()
        db.query(models.DeviceMetric).delete()
        db.query(models.Device).delete()
        db.commit()
    if db.query(models.Event).count() > 0:
        return

    device = models.Device(
        id="porch-sim-001",
        name="Front Porch",
        location="Entry Door",
        status="online",
        firmware_version="edge-demo-0.1.0",
        last_seen=datetime.now(timezone.utc),
    )
    db.add(device)
    db.commit()

    now = int(time.time() * 1000)
    event_specs = [
        ("person_at_door", "medium", 0.88, False, "Person detected in entry zone with stable confidence above threshold"),
        ("duplicate_event_suppressed", "info", 0.86, True, "Duplicate person event suppressed inside dedupe window"),
        ("visitor_linger", "high", 0.91, False, "Person remained in entry zone beyond linger threshold"),
        ("low_confidence_motion_suppressed", "info", 0.52, True, "Person-like detection did not meet confidence threshold"),
        ("package_delivered", "high", 0.84, False, "Package-sized object entered package zone and persisted across frames"),
        ("motion_without_detection_suppressed", "info", 0.26, True, "Motion detected without qualifying object detection"),
        ("package_removed", "high", 0.88, False, "Tracked package disappeared from package zone after persistent absence"),
    ]
    for i, (kind, severity, confidence, suppressed, explanation) in enumerate(event_specs):
        payload = schemas.EventIn(
            id=f"seed-{i}-{kind}",
            device_id=device.id,
            type=kind,
            severity=severity,
            timestamp_ms=now - (len(event_specs) - i) * 90_000,
            confidence=confidence,
            suppressed=suppressed,
            reasons=[explanation],
            explanation=explanation,
            confidence_history=[round(confidence - 0.04, 3), round(confidence - 0.01, 3), confidence],
            snapshot_ref=f"demo/snapshots/{kind}.svg",
            clip_ref=f"demo/clips/{kind}.mp4",
            processing_latency_ms=round(3.0 + i * 0.7, 3),
            metadata={"scenario": "seeded_demo", "frame_id": i * 12, "package_zone": {"x": 0.52, "y": 0.55, "w": 0.33, "h": 0.36}},
        )
        ingest_event(db, payload)

    for i in range(36):
        ingest_metric(
            db,
            device.id,
            schemas.MetricIn(
                device_id=device.id,
                timestamp_ms=now - (35 - i) * 20_000,
                fps=9.8 + random.random() * 1.4,
                cpu_load_1m=0.24 + random.random() * 0.28,
                memory_rss_mb=58 + random.random() * 7,
                queue_depth=0 if i < 22 else max(0, 8 - (i - 22)),
                dropped_frames=max(0, 4 - i // 8),
                network_status="offline" if 22 <= i < 27 else "online",
                uptime_seconds=i * 20,
            ),
        )

    balanced = models.SettingsProfile(
        id="balanced",
        name="Balanced",
        config_json=json.dumps({
            "motion_threshold": 0.18,
            "person_threshold": 0.72,
            "package_threshold": 0.68,
            "linger_seconds": 2.4,
            "dedupe_window_seconds": 18,
            "quiet_hours_enabled": False,
            "home_mode": "away",
            "package_zone": {"x": 0.52, "y": 0.55, "w": 0.33, "h": 0.36},
        }),
    )
    db.merge(balanced)
    db.commit()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--database", default=os.getenv("PORCHLIGHT_DATABASE_URL", "sqlite:///./runtime-data/backend/porchlight.db"))
    parser.add_argument("--replace", action="store_true")
    args = parser.parse_args()
    engine = create_engine(args.database, connect_args={"check_same_thread": False} if args.database.startswith("sqlite") else {})
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine, expire_on_commit=False)
    with Session() as db:
        seed_demo(db, replace=args.replace)


if __name__ == "__main__":
    main()
