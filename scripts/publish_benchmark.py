#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import urllib.request
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description='Publish latest benchmark artifact to a running Porchlight backend')
    parser.add_argument('--file', default='benchmarks/outputs/latest.json')
    parser.add_argument('--backend-url', default='http://127.0.0.1:8000')
    args = parser.parse_args()
    metrics = json.loads(Path(args.file).read_text(encoding='utf-8'))
    payload = json.dumps({'name': 'local-replay', 'metrics': metrics['edge_replay']}).encode('utf-8')
    req = urllib.request.Request(f"{args.backend_url.rstrip('/')}/benchmarks", data=payload, headers={'Content-Type': 'application/json'}, method='POST')
    with urllib.request.urlopen(req) as response:
        print(response.read().decode('utf-8'))


if __name__ == '__main__':
    main()
