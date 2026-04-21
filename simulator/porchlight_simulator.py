#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import os
import random
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any


@dataclass
class Detection:
    label: str
    confidence: float
    x: float
    y: float
    w: float
    h: float

    @property
    def center(self) -> tuple[float, float]:
        return (self.x + self.w / 2.0, self.y + self.h / 2.0)


def parse_detections(raw: str) -> list[Detection]:
    detections: list[Detection] = []
    for token in raw.split(';'):
        if not token:
            continue
        parts = token.split('@')
        if len(parts) != 6:
            continue
        detections.append(Detection(parts[0], float(parts[1]), float(parts[2]), float(parts[3]), float(parts[4]), float(parts[5])))
    return detections


def post_json(base_url: str, endpoint: str, token: str, payload: dict[str, Any]) -> bool:
    request = urllib.request.Request(
        f"{base_url}{endpoint}",
        data=json.dumps(payload).encode('utf-8'),
        headers={'Content-Type': 'application/json', 'X-Device-Token': token},
        method='POST',
    )
    try:
        with urllib.request.urlopen(request, timeout=1.5) as response:
            return 200 <= response.status < 300
    except (urllib.error.URLError, TimeoutError):
        return False


class ScenarioLogic:
    def __init__(self, device_id: str) -> None:
        self.device_id = device_id
        self.person_frames = 0
        self.package_frames = 0
        self.package_present = False
        self.package_absent = 0
        self.motion_only_frames = 0
        self.emitted: dict[str, int] = {}

    def _event(self, frame: dict[str, str], kind: str, confidence: float, suppressed: bool, reasons: list[str], severity: str = 'info') -> dict[str, Any]:
        frame_id = int(frame['frame_id'])
        timestamp_ms = int(frame['timestamp_ms'])
        return {
            'id': f'{self.device_id}-{kind}-{timestamp_ms}-{frame_id}',
            'device_id': self.device_id,
            'type': kind,
            'severity': severity,
            'timestamp_ms': timestamp_ms,
            'confidence': round(confidence, 4),
            'suppressed': suppressed,
            'reasons': reasons,
            'explanation': '; '.join(reasons),
            'confidence_history': [round(max(0.0, confidence - 0.04), 3), round(max(0.0, confidence - 0.01), 3), round(confidence, 3)],
            'snapshot_ref': f'demo/snapshots/{kind}.svg',
            'clip_ref': f'demo/clips/{kind}.mp4',
            'processing_latency_ms': round(random.uniform(2.0, 7.5), 4),
            'metadata': {'scenario': frame['scenario'], 'frame_id': frame_id, 'motion_score': float(frame['motion_score'])},
        }

    def process(self, frame: dict[str, str]) -> list[dict[str, Any]]:
        detections = parse_detections(frame.get('detections', ''))
        motion = float(frame['motion_score'])
        frame_id = int(frame['frame_id'])
        events: list[dict[str, Any]] = []
        best_person = max((d.confidence for d in detections if d.label == 'person'), default=0.0)
        best_package = max((d.confidence for d in detections if d.label == 'package' and 0.52 <= d.center[0] <= 0.85 and 0.55 <= d.center[1] <= 0.91), default=0.0)

        if best_person >= 0.72:
            self.person_frames += 1
            if self.person_frames == 3 and 'person_at_door' not in self.emitted:
                self.emitted['person_at_door'] = frame_id
                events.append(self._event(frame, 'person_at_door', best_person, False, ['Person detected in entry zone with stable confidence above threshold', 'Temporal smoothing confirmed detection across recent frames'], 'medium'))
            elif self.person_frames == 14 and 'visitor_linger' not in self.emitted:
                self.emitted['visitor_linger'] = frame_id
                events.append(self._event(frame, 'visitor_linger', best_person, False, ['Person remained in entry zone beyond linger threshold', 'Linger state persisted across multiple frames'], 'high'))
            elif self.person_frames in {8, 22}:
                events.append(self._event(frame, 'duplicate_event_suppressed', best_person, True, ['Duplicate person event suppressed because a similar alert was emitted inside the dedupe window']))
        elif 0.0 < best_person < 0.72 and frame_id % 6 == 0:
            events.append(self._event(frame, 'low_confidence_motion_suppressed', best_person, True, ['Person-like detection did not meet confidence threshold', 'Alert withheld until confidence is stable across frames']))
        else:
            if self.person_frames > 0 and best_person < 0.4:
                self.person_frames = 0

        if best_package >= 0.68:
            self.package_frames += 1
            self.package_absent = 0
            if self.package_frames == 5 and not self.package_present:
                self.package_present = True
                events.append(self._event(frame, 'package_delivered', best_package, False, ['Package-sized object entered the configured package zone', 'Object persisted across consecutive frames before alerting'], 'high'))
        else:
            self.package_frames = 0
            if self.package_present:
                self.package_absent += 1
                if self.package_absent == 16:
                    self.package_present = False
                    events.append(self._event(frame, 'package_removed', 0.88, False, ['Previously tracked package disappeared from package zone', 'Removal confirmed after absence persisted for multiple frames'], 'high'))

        if motion >= 0.18 and best_person < 0.72 and best_package < 0.68:
            self.motion_only_frames += 1
            if self.motion_only_frames in {4, 18}:
                events.append(self._event(frame, 'motion_without_detection_suppressed', motion, True, ['Motion detected but no qualifying person or package detection was stable enough to alert']))
        elif motion < 0.1:
            self.motion_only_frames = 0
        return events


def run(args: argparse.Namespace) -> None:
    base_url = os.getenv('PORCHLIGHT_BACKEND_URL', args.backend_url).rstrip('/')
    token = os.getenv('PORCHLIGHT_AUTH_TOKEN', args.token)
    device_id = os.getenv('PORCHLIGHT_DEVICE_ID', args.device_id)
    logic = ScenarioLogic(device_id)
    queue: list[tuple[str, dict[str, Any]]] = []

    post_json(base_url, '/devices', token, {'id': device_id, 'name': 'Simulator Front Porch', 'location': 'Entry Door', 'firmware_version': 'simulator-0.1.0'})

    with open(args.scenario, newline='', encoding='utf-8') as f:
        rows = list(csv.DictReader(f))

    start = time.perf_counter()
    for frame in rows:
        frame_id = int(frame['frame_id'])
        offline = args.offline_start <= frame_id < args.offline_end
        for event in logic.process(frame):
            if offline or not post_json(base_url, '/events', token, event):
                queue.append(('/events', event))
        if frame_id % 15 == 0:
            metric = {
                'device_id': device_id,
                'timestamp_ms': int(frame['timestamp_ms']),
                'fps': args.speed,
                'cpu_load_1m': round(0.24 + random.random() * 0.2, 3),
                'memory_rss_mb': round(54 + random.random() * 8, 2),
                'queue_depth': len(queue),
                'dropped_frames': 0,
                'network_status': 'offline' if offline else 'online',
                'uptime_seconds': round(time.perf_counter() - start, 2),
            }
            if offline or not post_json(base_url, f'/devices/{device_id}/metrics', token, metric):
                queue.append((f'/devices/{device_id}/metrics', metric))
        if not offline and queue:
            still_queued: list[tuple[str, dict[str, Any]]] = []
            for endpoint, payload in queue:
                if not post_json(base_url, endpoint, token, payload):
                    still_queued.append((endpoint, payload))
            queue = still_queued
        if args.speed > 0:
            time.sleep(1.0 / args.speed)

    for endpoint, payload in list(queue):
        if post_json(base_url, endpoint, token, payload):
            queue.remove((endpoint, payload))
    print(json.dumps({'device_id': device_id, 'frames': len(rows), 'remaining_queue_depth': len(queue)}, indent=2))


def main() -> None:
    parser = argparse.ArgumentParser(description='Porchlight demo device simulator')
    parser.add_argument('--scenario', default='demo-assets/scenarios/entry_sequence.frames.csv')
    parser.add_argument('--backend-url', default='http://127.0.0.1:8000')
    parser.add_argument('--token', default='dev-device-token')
    parser.add_argument('--device-id', default='porch-sim-001')
    parser.add_argument('--speed', type=float, default=25.0, help='Frames per second; 0 disables sleeping')
    parser.add_argument('--offline-start', type=int, default=92)
    parser.add_argument('--offline-end', type=int, default=118)
    run(parser.parse_args())


if __name__ == '__main__':
    main()
