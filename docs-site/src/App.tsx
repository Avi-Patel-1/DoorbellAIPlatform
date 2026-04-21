import { useEffect, useState } from 'react'

type Benchmark = {
  edge_replay?: Record<string, number | string | boolean>
  queue_reconnect?: Record<string, number | string | boolean>
  api_timings?: Record<string, number | string | boolean>
}

const nav = ['Features', 'Architecture', 'Demo', 'Benchmarks', 'API', 'Setup', 'FAQ']

function MetricCard({ label, value, note }: { label: string; value: string; note: string }) {
  return <div className="metric"><span>{label}</span><strong>{value}</strong><p>{note}</p></div>
}

function Section({ id, eyebrow, title, children }: { id: string; eyebrow: string; title: string; children: React.ReactNode }) {
  return <section id={id} className="section"><div className="eyebrow">{eyebrow}</div><h2>{title}</h2>{children}</section>
}

export default function App() {
  const [benchmark, setBenchmark] = useState<Benchmark>({})
  useEffect(() => {
    fetch('./data/benchmark-latest.json').then(r => r.ok ? r.json() : {}).then(setBenchmark).catch(() => setBenchmark({}))
  }, [])
  const edge = benchmark.edge_replay || {}
  const queue = benchmark.queue_reconnect || {}

  return (
    <div>
      <header className="topbar">
        <a className="brand" href="#top">Porchlight</a>
        <nav>{nav.map(item => <a key={item} href={`#${item.toLowerCase()}`}>{item}</a>)}</nav>
      </header>
      <main id="top">
        <section className="hero">
          <div className="heroText">
            <div className="eyebrow">Linux edge runtime · local intelligence · reliable demo</div>
            <h1>Edge AI smart doorbell and intelligent event detection platform.</h1>
            <p>Porchlight turns noisy entryway motion into explainable, device-friendly events. It performs event understanding on a Linux edge runtime, stores decisions locally, queues while offline, and syncs only meaningful history and telemetry upstream.</p>
            <div className="actions"><a href="#setup">Start locally</a><a href="#architecture" className="secondary">View architecture</a></div>
          </div>
          <div className="heroPanel"><img src="./assets/dashboard-overview.svg" alt="Porchlight dashboard overview" /></div>
        </section>

        <section className="metricsGrid">
          <MetricCard label="Replay FPS" value={typeof edge.fps === 'number' ? edge.fps.toFixed(2) : 'Run benchmark'} note="Measured by the local replay harness" />
          <MetricCard label="Alerts" value={`${edge.meaningful_alerts ?? '—'} / ${edge.suppressed_alerts ?? '—'}`} note="Meaningful versus suppressed decisions" />
          <MetricCard label="P95 latency" value={typeof edge.p95_processing_latency_ms === 'number' ? `${edge.p95_processing_latency_ms.toFixed(5)} ms` : '—'} note="Edge frame processing p95" />
          <MetricCard label="Queue recovery" value={queue.flush_success ? 'Pass' : 'Run benchmark'} note="Synthetic offline queue flush scenario" />
        </section>

        <Section id="features" eyebrow="Product behavior" title="Built for connected-property device demos">
          <div className="cards three">
            <article><h3>On-device event understanding</h3><p>Motion gating, modular inference, temporal smoothing, package-zone logic, and person linger detection run before any upstream publish.</p></article>
            <article><h3>Nuisance-alert suppression</h3><p>Cooldowns, dedupe windows, quiet-hours policy, confidence gates, and explainable suppressed events make the alert stream reviewable.</p></article>
            <article><h3>Operations-grade visibility</h3><p>The dashboard highlights event history, queue health, device telemetry, benchmark results, and runtime configuration.</p></article>
          </div>
        </Section>

        <Section id="architecture" eyebrow="System design" title="Edge-first architecture">
          <p className="lead">The edge service is intentionally independent of the backend: it can keep detecting, storing, and queueing when the network is down.</p>
          <img className="diagram" src="./assets/architecture.svg" alt="Porchlight architecture diagram" />
          <img className="diagram" src="./assets/event-flow.svg" alt="Porchlight event flow diagram" />
        </Section>

        <Section id="demo" eyebrow="Replay story" title="A demo that works without special hardware">
          <div className="timeline">
            {['Device starts and heartbeats begin','Person approaches and stable confidence triggers an alert','Visitor lingers and severity increases','Low-confidence motion is suppressed','Package persists in configured zone','Network goes offline and the local queue grows','Connectivity returns and the queue flushes'].map((step, index) => <div key={step}><span>{index + 1}</span><p>{step}</p></div>)}
          </div>
        </Section>

        <Section id="benchmarks" eyebrow="Measured artifacts" title="Reproducible benchmark report">
          <p className="lead">Metrics are generated from the bundled replay scenario and queue-recovery simulation. The charts below are produced by the repository scripts.</p>
          <div className="charts"><img src="./assets/latency_chart.svg" alt="Latency benchmark chart" /><img src="./assets/alert_mix.svg" alt="Alert mix benchmark chart" /></div>
          <pre><code>make benchmark
make assets</code></pre>
        </Section>

        <Section id="api" eyebrow="Backend" title="Cloud-style API surface">
          <div className="apiTable">
            <div><strong>/events</strong><span>Ingest and filter edge events, including suppressed decisions.</span></div>
            <div><strong>/devices</strong><span>Register and inspect devices, status, firmware, and health rollups.</span></div>
            <div><strong>/devices/:id/metrics</strong><span>Store FPS, queue depth, CPU, memory, and connectivity telemetry.</span></div>
            <div><strong>/benchmarks</strong><span>Persist benchmark runs for dashboard review.</span></div>
            <div><strong>/exports/events.csv</strong><span>Download event history for analysis or review.</span></div>
            <div><strong>/ws/events</strong><span>Live dashboard updates over WebSocket.</span></div>
          </div>
        </Section>

        <Section id="setup" eyebrow="Quick start" title="Run locally">
          <pre><code>make edge
make benchmark
make demo</code></pre>
          <p className="lead">For a no-Docker path, run <code>make demo-lite</code>. For Raspberry Pi deployment, build the edge runtime on-device or cross-compile, install the systemd unit, and use <code>config/edge.pi.yaml</code>.</p>
        </Section>

        <Section id="faq" eyebrow="Notes" title="FAQ and tradeoffs">
          <div className="cards two">
            <article><h3>Why lightweight replay mode?</h3><p>It keeps the project reviewable on any laptop while preserving the same edge logic, local storage, queueing, backend ingestion, and dashboard flow used by live mode.</p></article>
            <article><h3>What is full inference mode?</h3><p>The C++ runtime exposes an inference interface. The default demo uses bundled frame detections; production builds can swap in an ONNX Runtime detector.</p></article>
            <article><h3>Why store suppressed events?</h3><p>Suppressed decisions are valuable for tuning thresholds and proving nuisance-alert reduction without hiding edge behavior.</p></article>
            <article><h3>What changes for production?</h3><p>Add signed device provisioning, TLS, model packaging, hardware watchdogs, OTA updates, and privacy-aware clip retention.</p></article>
          </div>
        </Section>
      </main>
      <footer>Porchlight · Edge AI Smart Doorbell and Intelligent Event Detection Platform</footer>
    </div>
  )
}
