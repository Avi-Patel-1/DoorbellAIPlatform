from __future__ import annotations

import csv
import io
import json

from sqlalchemy.orm import Session

from app import models
from app.services import event_to_schema


def export_events_json(db: Session) -> str:
    events = db.query(models.Event).order_by(models.Event.timestamp_ms.desc()).all()
    return json.dumps([event_to_schema(event).model_dump(mode="json") for event in events], indent=2)


def export_events_csv(db: Session) -> str:
    output = io.StringIO()
    writer = csv.writer(output)
    writer.writerow(["id", "device_id", "timestamp_ms", "type", "severity", "confidence", "suppressed", "explanation"])
    for event in db.query(models.Event).order_by(models.Event.timestamp_ms.desc()).all():
        writer.writerow([
            event.id,
            event.device_id,
            event.timestamp_ms,
            event.type,
            event.severity,
            f"{event.confidence:.4f}",
            int(event.suppressed),
            event.explanation,
        ])
    return output.getvalue()
