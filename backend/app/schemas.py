from __future__ import annotations

from datetime import datetime
from typing import Any

from pydantic import BaseModel, ConfigDict, Field


class DeviceCreate(BaseModel):
    id: str
    name: str = "Front Door"
    location: str = "Entry"
    firmware_version: str = "edge-demo"


class DeviceOut(BaseModel):
    id: str
    name: str
    location: str
    status: str
    firmware_version: str
    last_seen: datetime | None
    created_at: datetime
    latest_metric: dict[str, Any] | None = None
    event_counts: dict[str, int] = Field(default_factory=dict)

    model_config = ConfigDict(from_attributes=True)


class EventIn(BaseModel):
    id: str
    device_id: str
    type: str
    severity: str = "info"
    timestamp_ms: int
    confidence: float = 0.0
    suppressed: bool = False
    reasons: list[str] = Field(default_factory=list)
    explanation: str = ""
    confidence_history: list[float] = Field(default_factory=list)
    snapshot_ref: str = ""
    clip_ref: str = ""
    processing_latency_ms: float = 0.0
    metadata: dict[str, Any] = Field(default_factory=dict)


class EventOut(EventIn):
    review_label: str | None = None
    created_at: datetime

    model_config = ConfigDict(from_attributes=True)


class EventLabel(BaseModel):
    label: str | None = None


class MetricIn(BaseModel):
    device_id: str
    timestamp_ms: int
    fps: float = 0.0
    cpu_load_1m: float = 0.0
    memory_rss_mb: float = 0.0
    queue_depth: int = 0
    dropped_frames: int = 0
    network_status: str = "unknown"
    uptime_seconds: float = 0.0


class MetricOut(MetricIn):
    id: int
    created_at: datetime

    model_config = ConfigDict(from_attributes=True)


class OverviewOut(BaseModel):
    devices_online: int
    devices_total: int
    events_today: int
    meaningful_events_today: int
    suppressed_events_today: int
    avg_processing_latency_ms: float
    queue_depth: int
    recent_events: list[EventOut]
    health_trend: list[MetricOut]


class BenchmarkIn(BaseModel):
    name: str = "local-replay"
    metrics: dict[str, Any]


class BenchmarkOut(BaseModel):
    id: int
    name: str
    created_at: datetime
    metrics: dict[str, Any]


class SettingsProfileIn(BaseModel):
    id: str = "balanced"
    name: str = "Balanced"
    config: dict[str, Any]


class SettingsProfileOut(SettingsProfileIn):
    updated_at: datetime
