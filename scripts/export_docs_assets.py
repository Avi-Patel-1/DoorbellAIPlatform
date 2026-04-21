#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def svg(path: Path, body: str, width: int = 1200, height: int = 720) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">{body}</svg>', encoding='utf-8')


def architecture(path: Path) -> None:
    boxes = [
        (70, 120, 220, 110, 'Edge runtime', 'Capture · inference · event logic'),
        (380, 120, 220, 110, 'Local storage', 'SQLite events + outbound queue'),
        (690, 120, 220, 110, 'Backend API', 'FastAPI ingest + history'),
        (980, 120, 150, 110, 'Dashboard', 'Review + health'),
        (70, 410, 220, 110, 'Replay / mock source', 'Laptop demo without hardware'),
        (380, 410, 220, 110, 'Benchmark harness', 'Latency · FPS · queue recovery'),
        (690, 410, 220, 110, 'Docs site', 'GitHub Pages report viewer'),
    ]
    items = ['<rect width="100%" height="100%" rx="28" fill="#f6f8fb"/>', '<text x="70" y="66" font-size="34" font-weight="800" fill="#102033" font-family="Inter,Arial">Porchlight architecture</text>']
    for x, y, w, h, title, sub in boxes:
        items.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="20" fill="#ffffff" stroke="#dbe3ec"/>')
        items.append(f'<text x="{x+22}" y="{y+42}" font-size="19" font-weight="700" fill="#102033" font-family="Inter,Arial">{title}</text>')
        items.append(f'<text x="{x+22}" y="{y+72}" font-size="14" fill="#64748b" font-family="Inter,Arial">{sub}</text>')
    arrows = [(290,175,380,175),(600,175,690,175),(910,175,980,175),(180,410,180,230),(490,410,490,230),(800,410,800,230)]
    for x1,y1,x2,y2 in arrows:
        items.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#2563eb" stroke-width="3" marker-end="url(#a)"/>')
    defs = '<defs><marker id="a" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto"><path d="M0,0 L0,6 L9,3 z" fill="#2563eb"/></marker></defs>'
    svg(path, defs + ''.join(items))


def dashboard_mock(path: Path, metrics: dict) -> None:
    edge = metrics.get('edge_replay', {})
    cards = [
        ('Replay FPS', f"{edge.get('fps', 0):.2f}" if isinstance(edge.get('fps'), (int, float)) else '—'),
        ('Meaningful', str(edge.get('meaningful_alerts', '—'))),
        ('Suppressed', str(edge.get('suppressed_alerts', '—'))),
        ('P95 latency', f"{edge.get('p95_processing_latency_ms', 0):.4f} ms" if isinstance(edge.get('p95_processing_latency_ms'), (int, float)) else '—'),
    ]
    parts = ['<rect width="100%" height="100%" rx="28" fill="#f6f8fb"/>', '<rect x="36" y="36" width="1128" height="652" rx="26" fill="#ffffff" stroke="#dbe3ec"/>', '<text x="72" y="92" font-size="30" font-weight="800" fill="#102033" font-family="Inter,Arial">Porchlight console</text>']
    for i, (label, value) in enumerate(cards):
        x = 72 + i * 270
        parts.append(f'<rect x="{x}" y="130" width="235" height="126" rx="18" fill="#f8fafc" stroke="#dbe3ec"/>')
        parts.append(f'<text x="{x+20}" y="168" font-size="14" font-weight="700" fill="#64748b" font-family="Inter,Arial">{label}</text>')
        parts.append(f'<text x="{x+20}" y="216" font-size="30" font-weight="800" fill="#102033" font-family="Inter,Arial">{value}</text>')
    rows = [('person_at_door','Person detected in entry zone with stable confidence'),('visitor_linger','Person remained near entry beyond linger threshold'),('package_delivered','Package entered configured package zone'),('motion_without_detection_suppressed','Motion-only event stored as suppressed decision')]
    parts.append('<text x="72" y="316" font-size="22" font-weight="800" fill="#102033" font-family="Inter,Arial">Event timeline</text>')
    for i, (kind, body) in enumerate(rows):
        y = 346 + i * 74
        parts.append(f'<rect x="72" y="{y}" width="980" height="56" rx="14" fill="#ffffff" stroke="#e5edf5"/>')
        parts.append(f'<text x="96" y="{y+24}" font-size="14" font-weight="800" fill="#2563eb" font-family="Inter,Arial">{kind}</text>')
        parts.append(f'<text x="96" y="{y+44}" font-size="13" fill="#64748b" font-family="Inter,Arial">{body}</text>')
    svg(path, ''.join(parts))


def event_flow(path: Path) -> None:
    steps = ['Frame capture', 'Motion gate', 'Inference backend', 'Temporal smoothing', 'Suppression policy', 'SQLite queue', 'Backend ingest', 'Dashboard']
    parts = ['<rect width="100%" height="100%" rx="28" fill="#ffffff"/>', '<text x="60" y="70" font-size="32" font-weight="800" fill="#102033" font-family="Inter,Arial">Event decision flow</text>']
    for i, step in enumerate(steps):
        x = 60 + (i % 4) * 280
        y = 140 + (i // 4) * 220
        parts.append(f'<rect x="{x}" y="{y}" width="220" height="110" rx="20" fill="#f8fafc" stroke="#dbe3ec"/>')
        parts.append(f'<text x="{x+22}" y="{y+60}" font-size="18" font-weight="700" fill="#102033" font-family="Inter,Arial">{step}</text>')
        if i < len(steps) - 1:
            nx = 60 + ((i+1) % 4) * 280
            ny = 140 + ((i+1) // 4) * 220
            if i == 3:
                parts.append(f'<line x1="{x+110}" y1="{y+110}" x2="{nx+110}" y2="{ny}" stroke="#2563eb" stroke-width="3" marker-end="url(#a)"/>')
            else:
                parts.append(f'<line x1="{x+220}" y1="{y+55}" x2="{nx}" y2="{ny+55}" stroke="#2563eb" stroke-width="3" marker-end="url(#a)"/>')
    defs = '<defs><marker id="a" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto"><path d="M0,0 L0,6 L9,3 z" fill="#2563eb"/></marker></defs>'
    svg(path, defs + ''.join(parts))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--benchmark', default='benchmarks/outputs/latest.json')
    args = parser.parse_args()
    benchmark_path = ROOT / args.benchmark
    metrics = json.loads(benchmark_path.read_text(encoding='utf-8')) if benchmark_path.exists() else {'edge_replay': {}}
    public_assets = ROOT / 'docs-site' / 'public' / 'assets'
    public_data = ROOT / 'docs-site' / 'public' / 'data'
    public_assets.mkdir(parents=True, exist_ok=True)
    public_data.mkdir(parents=True, exist_ok=True)
    architecture(public_assets / 'architecture.svg')
    event_flow(public_assets / 'event-flow.svg')
    dashboard_mock(public_assets / 'dashboard-overview.svg', metrics)
    for name in ['latency_chart.svg', 'alert_mix.svg']:
        src = ROOT / 'benchmarks' / 'outputs' / name
        if src.exists():
            shutil.copy2(src, public_assets / name)
    (public_data / 'benchmark-latest.json').write_text(json.dumps(metrics, indent=2), encoding='utf-8')
    reports = ROOT / 'reports' / 'generated'
    reports.mkdir(parents=True, exist_ok=True)
    shutil.copy2(public_assets / 'architecture.svg', reports / 'architecture.svg')
    shutil.copy2(public_assets / 'dashboard-overview.svg', reports / 'dashboard-overview.svg')
    print(f'Wrote docs assets to {public_assets}')


if __name__ == '__main__':
    main()
