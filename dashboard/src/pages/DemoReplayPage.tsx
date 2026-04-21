const steps = [
  ['Device boot', 'Edge runtime opens the replay source, creates SQLite tables, and begins health sampling.'],
  ['Person approaches', 'Temporal smoothing waits for stable person confidence before emitting person_at_door.'],
  ['Visitor lingers', 'The linger timer crosses the configured threshold and raises severity.'],
  ['Noise suppressed', 'Low-confidence person-like detections and motion-only frames are stored as suppressed decisions.'],
  ['Package delivered', 'A package-sized object persists in the configured zone before alerting.'],
  ['Network interruption', 'Outbound event payloads remain in the local queue while the backend is unavailable.'],
  ['Recovery', 'Queued events flush with bounded retry/backoff once connectivity returns.'],
]

export default function DemoReplayPage() {
  return (
    <div className="space-y-5">
      <div>
        <h1 className="text-2xl font-bold tracking-tight text-ink">Demo replay</h1>
        <p className="mt-1 text-sm text-slate-500">A deterministic story used by the simulator, edge runtime, and benchmark harness.</p>
      </div>
      <div className="card p-5">
        <div className="relative space-y-5 border-l border-line pl-6">
          {steps.map(([title, body], index) => (
            <div key={title} className="relative">
              <div className="absolute -left-[33px] top-1 flex h-5 w-5 items-center justify-center rounded-full border border-line bg-white text-xs font-bold text-accent">{index + 1}</div>
              <h2 className="font-bold text-ink">{title}</h2>
              <p className="mt-1 text-sm text-slate-600">{body}</p>
            </div>
          ))}
        </div>
      </div>
      <div className="card p-5">
        <h2 className="text-lg font-bold text-ink">Run the story locally</h2>
        <pre className="mt-4 overflow-auto rounded-xl bg-slate-950 p-4 text-sm text-slate-100">make demo{"\n"}# or for a no-Docker replay{"\n"}make demo-lite</pre>
      </div>
    </div>
  )
}
