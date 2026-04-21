from __future__ import annotations

import json
from datetime import datetime, timezone

from fastapi import Depends, FastAPI, Header, HTTPException, Query, Response, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy import or_
from sqlalchemy.orm import Session

from app import models, schemas
from app.core.config import get_settings
from app.db import get_db, init_db
from app.exporters import export_events_csv, export_events_json
from app.realtime import manager
from app.seed import seed_demo
from app.services import device_summary, event_to_schema, ingest_event, ingest_metric, metric_to_schema, overview

settings = get_settings()
app = FastAPI(
    title="Porchlight Device Intelligence API",
    version="0.1.0",
    description="Cloud-style ingestion, telemetry, event history, and dashboard API for the Porchlight edge runtime.",
)
app.add_middleware(
    CORSMiddleware,
    allow_origins=[settings.dashboard_origin, "http://localhost:5173", "http://127.0.0.1:5173", "*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def require_device_token(x_device_token: str = Header(default="")) -> None:
    if x_device_token != settings.device_token:
        raise HTTPException(status_code=401, detail="invalid device token")


@app.on_event("startup")
def startup() -> None:
    init_db()
    if settings.enable_seed:
        from app.db import SessionLocal

        with SessionLocal() as db:
            seed_demo(db)


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok", "service": "porchlight-backend"}


@app.get("/overview", response_model=schemas.OverviewOut)
def get_overview(db: Session = Depends(get_db)) -> schemas.OverviewOut:
    return overview(db)


@app.post("/devices", response_model=schemas.DeviceOut)
def register_device(payload: schemas.DeviceCreate, _: None = Depends(require_device_token), db: Session = Depends(get_db)) -> schemas.DeviceOut:
    device = db.get(models.Device, payload.id)
    if device is None:
        device = models.Device(id=payload.id)
        db.add(device)
    device.name = payload.name
    device.location = payload.location
    device.firmware_version = payload.firmware_version
    device.status = "online"
    device.last_seen = datetime.now(timezone.utc)
    db.commit()
    db.refresh(device)
    return device_summary(db, device)


@app.get("/devices", response_model=list[schemas.DeviceOut])
def list_devices(db: Session = Depends(get_db)) -> list[schemas.DeviceOut]:
    devices = db.query(models.Device).order_by(models.Device.id).all()
    return [device_summary(db, device) for device in devices]


@app.get("/devices/{device_id}", response_model=schemas.DeviceOut)
def get_device(device_id: str, db: Session = Depends(get_db)) -> schemas.DeviceOut:
    device = db.get(models.Device, device_id)
    if device is None:
        raise HTTPException(status_code=404, detail="device not found")
    return device_summary(db, device)


@app.post("/devices/{device_id}/metrics", response_model=schemas.MetricOut)
async def post_metric(device_id: str, payload: schemas.MetricIn, _: None = Depends(require_device_token), db: Session = Depends(get_db)) -> schemas.MetricOut:
    metric = ingest_metric(db, device_id, payload)
    result = metric_to_schema(metric)
    await manager.broadcast({"kind": "metric", "payload": result.model_dump(mode="json")})
    return result


@app.get("/devices/{device_id}/metrics", response_model=list[schemas.MetricOut])
def list_metrics(device_id: str, limit: int = Query(120, le=500), db: Session = Depends(get_db)) -> list[schemas.MetricOut]:
    metrics = (
        db.query(models.DeviceMetric)
        .filter(models.DeviceMetric.device_id == device_id)
        .order_by(models.DeviceMetric.timestamp_ms.desc())
        .limit(limit)
        .all()
    )
    return [metric_to_schema(m) for m in reversed(metrics)]


@app.post("/events", response_model=schemas.EventOut)
async def post_event(payload: schemas.EventIn, _: None = Depends(require_device_token), db: Session = Depends(get_db)) -> schemas.EventOut:
    event = ingest_event(db, payload)
    result = event_to_schema(event)
    await manager.broadcast({"kind": "event", "payload": result.model_dump(mode="json")})
    return result


@app.get("/events", response_model=list[schemas.EventOut])
def list_events(
    device_id: str | None = None,
    event_type: str | None = None,
    suppressed: bool | None = None,
    q: str | None = None,
    limit: int = Query(100, le=500),
    offset: int = 0,
    db: Session = Depends(get_db),
) -> list[schemas.EventOut]:
    query = db.query(models.Event)
    if device_id:
        query = query.filter(models.Event.device_id == device_id)
    if event_type:
        query = query.filter(models.Event.type == event_type)
    if suppressed is not None:
        query = query.filter(models.Event.suppressed.is_(suppressed))
    if q:
        like = f"%{q}%"
        query = query.filter(or_(models.Event.type.ilike(like), models.Event.explanation.ilike(like), models.Event.metadata_json.ilike(like)))
    events = query.order_by(models.Event.timestamp_ms.desc()).offset(offset).limit(limit).all()
    return [event_to_schema(e) for e in events]


@app.get("/events/{event_id}", response_model=schemas.EventOut)
def get_event(event_id: str, db: Session = Depends(get_db)) -> schemas.EventOut:
    event = db.get(models.Event, event_id)
    if event is None:
        raise HTTPException(status_code=404, detail="event not found")
    return event_to_schema(event)


@app.patch("/events/{event_id}/label", response_model=schemas.EventOut)
def label_event(event_id: str, payload: schemas.EventLabel, db: Session = Depends(get_db)) -> schemas.EventOut:
    event = db.get(models.Event, event_id)
    if event is None:
        raise HTTPException(status_code=404, detail="event not found")
    event.review_label = payload.label
    db.commit()
    db.refresh(event)
    return event_to_schema(event)


@app.get("/benchmarks", response_model=list[schemas.BenchmarkOut])
def list_benchmarks(db: Session = Depends(get_db)) -> list[schemas.BenchmarkOut]:
    runs = db.query(models.BenchmarkRun).order_by(models.BenchmarkRun.created_at.desc()).limit(20).all()
    return [schemas.BenchmarkOut(id=r.id, name=r.name, created_at=r.created_at, metrics=json.loads(r.metrics_json)) for r in runs]


@app.post("/benchmarks", response_model=schemas.BenchmarkOut)
def create_benchmark(payload: schemas.BenchmarkIn, db: Session = Depends(get_db)) -> schemas.BenchmarkOut:
    run = models.BenchmarkRun(name=payload.name, metrics_json=json.dumps(payload.metrics))
    db.add(run)
    db.commit()
    db.refresh(run)
    return schemas.BenchmarkOut(id=run.id, name=run.name, created_at=run.created_at, metrics=payload.metrics)


@app.get("/settings", response_model=list[schemas.SettingsProfileOut])
def list_settings(db: Session = Depends(get_db)) -> list[schemas.SettingsProfileOut]:
    profiles = db.query(models.SettingsProfile).order_by(models.SettingsProfile.id).all()
    return [schemas.SettingsProfileOut(id=p.id, name=p.name, config=json.loads(p.config_json), updated_at=p.updated_at) for p in profiles]


@app.put("/settings/{profile_id}", response_model=schemas.SettingsProfileOut)
def update_settings(profile_id: str, payload: schemas.SettingsProfileIn, db: Session = Depends(get_db)) -> schemas.SettingsProfileOut:
    profile = db.get(models.SettingsProfile, profile_id) or models.SettingsProfile(id=profile_id)
    profile.name = payload.name
    profile.config_json = json.dumps(payload.config)
    profile.updated_at = datetime.now(timezone.utc)
    db.merge(profile)
    db.commit()
    db.refresh(profile)
    return schemas.SettingsProfileOut(id=profile.id, name=profile.name, config=json.loads(profile.config_json), updated_at=profile.updated_at)


@app.get("/exports/events.json")
def export_json(db: Session = Depends(get_db)) -> Response:
    return Response(export_events_json(db), media_type="application/json")


@app.get("/exports/events.csv")
def export_csv(db: Session = Depends(get_db)) -> Response:
    return Response(export_events_csv(db), media_type="text/csv")


@app.websocket("/ws/events")
async def websocket_events(websocket: WebSocket) -> None:
    await manager.connect(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)
