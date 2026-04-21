#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import math
import resource
import statistics
import subprocess
import time
import urllib.request
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    idx = min(len(ordered) - 1, int(math.ceil((p / 100.0) * len(ordered)) - 1))
    return ordered[idx]


def run_edge(edge_binary: Path, scenario: Path, output_dir: Path) -> dict[str, Any]:
    events_path = output_dir / 'latest_events.jsonl'
    stats_path = output_dir / 'edge_stats.json'
    config_path = ROOT / 'config' / 'edge.demo.yaml'
    cmd = [
        str(edge_binary),
        '--config', str(config_path),
        '--no-publish',
        '--output-json', str(events_path),
        '--benchmark-json', str(stats_path),
    ]
    start = time.perf_counter()
    completed = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True, check=False)
    elapsed = time.perf_counter() - start
    if completed.returncode != 0:
        raise RuntimeError(f'edge runtime failed:\nSTDOUT:\n{completed.stdout}\nSTDERR:\n{completed.stderr}')
    stats = json.loads(stats_path.read_text(encoding='utf-8'))
    stats['wall_time_seconds'] = round(elapsed, 6)
    stats['events_jsonl'] = str(events_path.resolve().relative_to(ROOT))
    return stats


def python_replay_baseline(scenario: Path, thresholds: dict[str, float]) -> dict[str, Any]:
    meaningful = 0
    suppressed = 0
    latencies: list[float] = []
    package_frames = 0
    package_present = False
    person_frames = 0
    emitted_person = False
    emitted_linger = False
    emitted_package = False
    package_absent = 0
    start = time.perf_counter()
    with scenario.open(newline='', encoding='utf-8') as f:
        rows = list(csv.DictReader(f))
    for row in rows:
        frame_start = time.perf_counter_ns()
        detections = []
        for token in row['detections'].split(';'):
            if not token:
                continue
            parts = token.split('@')
            if len(parts) == 6:
                detections.append((parts[0], float(parts[1]), float(parts[2]), float(parts[3]), float(parts[4]), float(parts[5])))
        best_person = max((d[1] for d in detections if d[0] == 'person'), default=0.0)
        best_package = max((d[1] for d in detections if d[0] == 'package'), default=0.0)
        motion = float(row['motion_score'])
        if best_person >= thresholds['person_threshold']:
            person_frames += 1
            if person_frames >= 3 and not emitted_person:
                meaningful += 1
                emitted_person = True
            elif person_frames >= 20 and not emitted_linger:
                meaningful += 1
                emitted_linger = True
            elif person_frames in {6, 12}:
                suppressed += 1
        elif best_person > 0:
            suppressed += 1
        else:
            if best_person < 0.4:
                person_frames = 0
        if best_package >= thresholds['package_threshold']:
            package_frames += 1
            package_absent = 0
            if package_frames >= 5 and not package_present and not emitted_package:
                meaningful += 1
                package_present = True
                emitted_package = True
        else:
            package_frames = 0
            if package_present:
                package_absent += 1
                if package_absent >= 16:
                    meaningful += 1
                    package_present = False
        if motion >= thresholds['motion_threshold'] and best_person < thresholds['person_threshold'] and best_package < thresholds['package_threshold']:
            if int(row['frame_id']) % 18 == 0:
                suppressed += 1
        latencies.append((time.perf_counter_ns() - frame_start) / 1_000_000.0)
    elapsed = time.perf_counter() - start
    return {
        'frames': len(rows),
        'meaningful_alerts': meaningful,
        'suppressed_alerts': suppressed,
        'avg_processing_latency_ms': statistics.mean(latencies) if latencies else 0.0,
        'p95_processing_latency_ms': percentile(latencies, 95),
        'fps': len(rows) / elapsed if elapsed > 0 else 0.0,
    }


def threshold_sweeps(scenario: Path) -> list[dict[str, Any]]:
    profiles = {
        'sensitive': {'motion_threshold': 0.12, 'person_threshold': 0.62, 'package_threshold': 0.60},
        'balanced': {'motion_threshold': 0.18, 'person_threshold': 0.72, 'package_threshold': 0.68},
        'low_noise': {'motion_threshold': 0.26, 'person_threshold': 0.80, 'package_threshold': 0.76},
    }
    results = []
    for name, thresholds in profiles.items():
        metrics = python_replay_baseline(scenario, thresholds)
        results.append({'profile': name, **{k: round(v, 5) if isinstance(v, float) else v for k, v in metrics.items()}})
    return results


def queue_reconnect_simulation() -> dict[str, Any]:
    queue = []
    peak = 0
    offline_start = time.perf_counter()
    for tick in range(80):
        offline = 18 <= tick < 44
        if tick % 4 == 0:
            queue.append({'tick': tick, 'payload': 'event' if tick % 8 == 0 else 'metric'})
        if not offline and queue:
            queue.clear()
        peak = max(peak, len(queue))
    recovery_seconds = time.perf_counter() - offline_start
    return {'peak_queue_depth': peak, 'final_queue_depth': len(queue), 'recovery_loop_seconds': round(recovery_seconds, 6), 'flush_success': len(queue) == 0}


def api_timings(base_url: str) -> dict[str, Any]:
    endpoints = ['/health', '/overview']
    samples: dict[str, list[float]] = {endpoint: [] for endpoint in endpoints}
    for endpoint in endpoints:
        for _ in range(5):
            start = time.perf_counter()
            try:
                with urllib.request.urlopen(f'{base_url}{endpoint}', timeout=0.7) as response:
                    response.read()
                samples[endpoint].append((time.perf_counter() - start) * 1000.0)
            except Exception:
                return {'available': False}
    return {
        'available': True,
        'health_avg_ms': round(statistics.mean(samples['/health']), 3),
        'overview_avg_ms': round(statistics.mean(samples['/overview']), 3),
    }


def write_svg_bar_chart(path: Path, title: str, data: list[tuple[str, float]], unit: str = '') -> None:
    width, height = 760, 340
    max_value = max((v for _, v in data), default=1.0) or 1.0
    bars = []
    left = 120
    chart_w = 560
    bar_h = 34
    gap = 18
    for i, (label, value) in enumerate(data):
        y = 78 + i * (bar_h + gap)
        w = (value / max_value) * chart_w
        bars.append(f'<text x="24" y="{y+22}" font-size="14" fill="#334155">{label}</text>')
        bars.append(f'<rect x="{left}" y="{y}" width="{w:.1f}" height="{bar_h}" rx="8" fill="#2563eb" opacity="0.88"/>')
        bars.append(f'<text x="{left + w + 10:.1f}" y="{y+22}" font-size="13" fill="#0f172a" font-weight="700">{value:.3f}{unit}</text>')
    path.write_text(f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
  <rect width="100%" height="100%" rx="20" fill="#ffffff"/>
  <text x="24" y="38" font-family="Inter,Arial,sans-serif" font-size="22" font-weight="700" fill="#102033">{title}</text>
  <g font-family="Inter,Arial,sans-serif">{''.join(bars)}</g>
</svg>''', encoding='utf-8')


def markdown_summary(metrics: dict[str, Any]) -> str:
    edge = metrics['edge_replay']
    queue = metrics['queue_reconnect']
    api = metrics['api_timings']
    lines = [
        '# Porchlight benchmark summary',
        '',
        'Generated by `benchmarks/scripts/run_benchmarks.py` from bundled replay and synthetic network scenarios.',
        '',
        '| Metric | Value |',
        '|---|---:|',
        f"| Replay frames | {edge['frames']} |",
        f"| Replay FPS | {edge['fps']:.3f} |",
        f"| Average processing latency | {edge['avg_processing_latency_ms']:.5f} ms |",
        f"| P95 processing latency | {edge['p95_processing_latency_ms']:.5f} ms |",
        f"| Meaningful alerts | {edge['meaningful_alerts']} |",
        f"| Suppressed decisions | {edge['suppressed_alerts']} |",
        f"| Queue peak during reconnect simulation | {queue['peak_queue_depth']} |",
        f"| Queue final depth | {queue['final_queue_depth']} |",
        '',
        '## Threshold sweep',
        '',
        '| Profile | Meaningful | Suppressed | FPS | Avg latency (ms) |',
        '|---|---:|---:|---:|---:|',
    ]
    for row in metrics['threshold_sweeps']:
        lines.append(f"| {row['profile']} | {row['meaningful_alerts']} | {row['suppressed_alerts']} | {row['fps']:.3f} | {row['avg_processing_latency_ms']:.5f} |")
    lines.append('')
    if api.get('available'):
        lines.append(f"API timing samples were available: `/health` average {api['health_avg_ms']} ms, `/overview` average {api['overview_avg_ms']} ms.")
    else:
        lines.append('API timing samples were skipped because the backend was not reachable during this run.')
    return '\n'.join(lines) + '\n'


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--edge-binary', type=Path, default=ROOT / 'edge-runtime/build/porchlight_edge')
    parser.add_argument('--scenario', type=Path, default=ROOT / 'demo-assets/scenarios/entry_sequence.frames.csv')
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'benchmarks/outputs')
    parser.add_argument('--api-base-url', default='http://127.0.0.1:8000')
    args = parser.parse_args()
    output_dir = args.output_dir if args.output_dir.is_absolute() else ROOT / args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)
    scenario = args.scenario if args.scenario.is_absolute() else ROOT / args.scenario

    if args.edge_binary.exists():
        edge_metrics = run_edge(args.edge_binary, scenario, output_dir)
    else:
        edge_metrics = python_replay_baseline(scenario, {'motion_threshold': 0.18, 'person_threshold': 0.72, 'package_threshold': 0.68})
        edge_metrics['source'] = 'python-fallback'

    metrics = {
        'generated_at_unix_ms': int(time.time() * 1000),
        'scenario': str(scenario.relative_to(ROOT)),
        'edge_replay': edge_metrics,
        'threshold_sweeps': threshold_sweeps(scenario),
        'queue_reconnect': queue_reconnect_simulation(),
        'api_timings': api_timings(args.api_base_url),
        'process_resource_usage': {
            'max_rss_kb': resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
        },
    }

    latest = output_dir / 'latest.json'
    latest.write_text(json.dumps(metrics, indent=2), encoding='utf-8')
    (output_dir / 'summary.md').write_text(markdown_summary(metrics), encoding='utf-8')
    write_svg_bar_chart(output_dir / 'latency_chart.svg', 'Replay latency', [
        ('Average', float(edge_metrics['avg_processing_latency_ms'])),
        ('P95', float(edge_metrics['p95_processing_latency_ms'])),
    ], ' ms')
    write_svg_bar_chart(output_dir / 'alert_mix.svg', 'Alert decisions', [
        ('Meaningful', float(edge_metrics['meaningful_alerts'])),
        ('Suppressed', float(edge_metrics['suppressed_alerts'])),
    ])
    print(json.dumps(metrics, indent=2))


if __name__ == '__main__':
    main()
