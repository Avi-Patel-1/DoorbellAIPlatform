from __future__ import annotations

from datetime import datetime, timezone

from sqlalchemy import Boolean, DateTime, Float, ForeignKey, Integer, String, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.db import Base


def utcnow() -> datetime:
    return datetime.now(timezone.utc)


class Device(Base):
    __tablename__ = "devices"

    id: Mapped[str] = mapped_column(String(80), primary_key=True)
    name: Mapped[str] = mapped_column(String(120), default="Front Door")
    location: Mapped[str] = mapped_column(String(160), default="Entry")
    status: Mapped[str] = mapped_column(String(32), default="offline", index=True)
    firmware_version: Mapped[str] = mapped_column(String(64), default="edge-demo")
    last_seen: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=utcnow)

    events: Mapped[list[Event]] = relationship(back_populates="device", cascade="all,delete")
    metrics: Mapped[list[DeviceMetric]] = relationship(back_populates="device", cascade="all,delete")


class Event(Base):
    __tablename__ = "events"

    id: Mapped[str] = mapped_column(String(160), primary_key=True)
    device_id: Mapped[str] = mapped_column(ForeignKey("devices.id"), index=True)
    type: Mapped[str] = mapped_column(String(64), index=True)
    severity: Mapped[str] = mapped_column(String(32), default="info", index=True)
    timestamp_ms: Mapped[int] = mapped_column(Integer, index=True)
    confidence: Mapped[float] = mapped_column(Float, default=0.0)
    suppressed: Mapped[bool] = mapped_column(Boolean, default=False, index=True)
    explanation: Mapped[str] = mapped_column(Text, default="")
    reasons_json: Mapped[str] = mapped_column(Text, default="[]")
    metadata_json: Mapped[str] = mapped_column(Text, default="{}")
    confidence_history_json: Mapped[str] = mapped_column(Text, default="[]")
    snapshot_ref: Mapped[str] = mapped_column(String(240), default="")
    clip_ref: Mapped[str] = mapped_column(String(240), default="")
    processing_latency_ms: Mapped[float] = mapped_column(Float, default=0.0)
    review_label: Mapped[str | None] = mapped_column(String(80), nullable=True)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=utcnow)

    device: Mapped[Device] = relationship(back_populates="events")


class DeviceMetric(Base):
    __tablename__ = "device_metrics"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id: Mapped[str] = mapped_column(ForeignKey("devices.id"), index=True)
    timestamp_ms: Mapped[int] = mapped_column(Integer, index=True)
    fps: Mapped[float] = mapped_column(Float, default=0.0)
    cpu_load_1m: Mapped[float] = mapped_column(Float, default=0.0)
    memory_rss_mb: Mapped[float] = mapped_column(Float, default=0.0)
    queue_depth: Mapped[int] = mapped_column(Integer, default=0)
    dropped_frames: Mapped[int] = mapped_column(Integer, default=0)
    network_status: Mapped[str] = mapped_column(String(40), default="unknown")
    uptime_seconds: Mapped[float] = mapped_column(Float, default=0.0)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=utcnow)

    device: Mapped[Device] = relationship(back_populates="metrics")


class BenchmarkRun(Base):
    __tablename__ = "benchmark_runs"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    name: Mapped[str] = mapped_column(String(120), default="local-replay")
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=utcnow)
    metrics_json: Mapped[str] = mapped_column(Text, default="{}")


class SettingsProfile(Base):
    __tablename__ = "settings_profiles"

    id: Mapped[str] = mapped_column(String(80), primary_key=True)
    name: Mapped[str] = mapped_column(String(120), default="Balanced")
    config_json: Mapped[str] = mapped_column(Text, default="{}")
    updated_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=utcnow)
