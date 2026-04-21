from __future__ import annotations

import json
import time
from datetime import datetime, timezone
from typing import Any

from sqlalchemy import func
from sqlalchemy.orm import Session

from app import models, schemas


def ensure_device(db: Session, device_id: str, name: str | None = None) -> models.Device:
    device = db.get(models.Device, device_id)
    if device is None:
        device = models.Device(id=device_id, name=name or "Front Door", status="online", last_seen=datetime.now(timezone.utc))
        db.add(device)
        db.flush()
    return device


def event_to_schema(event: models.Event) -> schemas.EventOut:
    return schemas.EventOut(
        id=event.id,
        device_id=event.device_id,
        type=event.type,
        severity=event.severity,
        timestamp_ms=event.timestamp_ms,
        confidence=event.confidence,
        suppressed=event.suppressed,
        reasons=json.loads(event.reasons_json or "[]"),
        explanation=event.explanation,
        confidence_history=json.loads(event.confidence_history_json or "[]"),
        snapshot_ref=event.snapshot_ref,
        clip_ref=event.clip_ref,
        processing_latency_ms=event.processing_latency_ms,
        metadata=json.loads(event.metadata_json or "{}"),
        review_label=event.review_label,
        created_at=event.created_at,
    )


def metric_to_schema(metric: models.DeviceMetric) -> schemas.MetricOut:
    return schemas.MetricOut.model_validate(metric)


def ingest_event(db: Session, payload: schemas.EventIn) -> models.Event:
    device = ensure_device(db, payload.device_id)
    device.status = "online"
    device.last_seen = datetime.now(timezone.utc)
    event = db.get(models.Event, payload.id)
    if event is None:
        event = models.Event(id=payload.id, device_id=payload.device_id)
        db.add(event)
    event.type = payload.type
    event.severity = payload.severity
    event.timestamp_ms = payload.timestamp_ms
    event.confidence = payload.confidence
    event.suppressed = payload.suppressed
    event.explanation = payload.explanation or "; ".join(payload.reasons)
    event.reasons_json = json.dumps(payload.reasons)
    event.confidence_history_json = json.dumps(payload.confidence_history)
    event.metadata_json = json.dumps(payload.metadata)
    event.snapshot_ref = payload.snapshot_ref
    event.clip_ref = payload.clip_ref
    event.processing_latency_ms = payload.processing_latency_ms
    db.commit()
    db.refresh(event)
    return event


def ingest_metric(db: Session, device_id: str, payload: schemas.MetricIn) -> models.DeviceMetric:
    device = ensure_device(db, device_id)
    device.status = "online" if payload.network_status != "offline" else "degraded"
    device.last_seen = datetime.now(timezone.utc)
    metric = models.DeviceMetric(
        device_id=device_id,
        timestamp_ms=payload.timestamp_ms,
        fps=payload.fps,
        cpu_load_1m=payload.cpu_load_1m,
        memory_rss_mb=payload.memory_rss_mb,
        queue_depth=payload.queue_depth,
        dropped_frames=payload.dropped_frames,
        network_status=payload.network_status,
        uptime_seconds=payload.uptime_seconds,
    )
    db.add(metric)
    db.commit()
    db.refresh(metric)
    return metric


def overview(db: Session) -> schemas.OverviewOut:
    now_ms = int(time.time() * 1000)
    day_start_ms = now_ms - 24 * 60 * 60 * 1000
    devices_total = db.query(models.Device).count()
    devices_online = db.query(models.Device).filter(models.Device.status == "online").count()
    events_today_q = db.query(models.Event).filter(models.Event.timestamp_ms >= day_start_ms)
    events_today = events_today_q.count()
    suppressed = events_today_q.filter(models.Event.suppressed.is_(True)).count()
    meaningful = events_today - suppressed
    avg_latency = db.query(func.avg(models.Event.processing_latency_ms)).scalar() or 0.0
    queue_depth = db.query(func.max(models.DeviceMetric.queue_depth)).scalar() or 0
    recent_events = db.query(models.Event).order_by(models.Event.timestamp_ms.desc()).limit(8).all()
    health = db.query(models.DeviceMetric).order_by(models.DeviceMetric.timestamp_ms.desc()).limit(40).all()
    return schemas.OverviewOut(
        devices_online=devices_online,
        devices_total=devices_total,
        events_today=events_today,
        meaningful_events_today=meaningful,
        suppressed_events_today=suppressed,
        avg_processing_latency_ms=float(avg_latency),
        queue_depth=int(queue_depth),
        recent_events=[event_to_schema(e) for e in recent_events],
        health_trend=[metric_to_schema(m) for m in reversed(health)],
    )


def device_summary(db: Session, device: models.Device) -> schemas.DeviceOut:
    latest = (
        db.query(models.DeviceMetric)
        .filter(models.DeviceMetric.device_id == device.id)
        .order_by(models.DeviceMetric.timestamp_ms.desc())
        .first()
    )
    counts = {
        "events": db.query(models.Event).filter(models.Event.device_id == device.id).count(),
        "meaningful": db.query(models.Event).filter(models.Event.device_id == device.id, models.Event.suppressed.is_(False)).count(),
        "suppressed": db.query(models.Event).filter(models.Event.device_id == device.id, models.Event.suppressed.is_(True)).count(),
    }
    return schemas.DeviceOut(
        id=device.id,
        name=device.name,
        location=device.location,
        status=device.status,
        firmware_version=device.firmware_version,
        last_seen=device.last_seen,
        created_at=device.created_at,
        latest_metric=metric_to_schema(latest).model_dump(mode="json") if latest else None,
        event_counts=counts,
    )
