SHELL := /usr/bin/env bash
ROOT := $(shell pwd)
EDGE_BUILD := $(ROOT)/edge-runtime/build

.PHONY: setup edge test edge-test backend-test lint demo demo-lite demo-down benchmark docs docs-build clean seed assets

setup:
	./scripts/setup.sh

edge:
	cmake -S edge-runtime -B $(EDGE_BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(EDGE_BUILD) --parallel

edge-test: edge
	ctest --test-dir $(EDGE_BUILD) --output-on-failure

backend-test:
	cd backend && python -m pytest -q

test: edge-test backend-test

lint:
	cd backend && python -m ruff check app tests || true
	cd dashboard && npm run lint || true

seed:
	cd backend && python -m app.seed --database sqlite:///../runtime-data/backend/porchlight.db

demo:
	./scripts/demo_up.sh

demo-lite: edge
	./scripts/demo_lite.sh

demo-down:
	docker compose down --remove-orphans

benchmark: edge
	python benchmarks/scripts/run_benchmarks.py --edge-binary $(EDGE_BUILD)/porchlight_edge --scenario demo-assets/scenarios/entry_sequence.frames.csv --output-dir benchmarks/outputs

assets:
	python scripts/export_docs_assets.py --benchmark benchmarks/outputs/latest.json

docs-build:
	cd docs-site && npm ci && npm run build

docs: docs-build

clean:
	rm -rf edge-runtime/build backend/.pytest_cache dashboard/node_modules dashboard/dist docs-site/node_modules docs-site/dist runtime-data
