import { useEffect, useState } from 'react'
import { Link, useParams } from 'react-router-dom'
import { api } from '../api'
import EventBadge from '../components/EventBadge'
import Sparkline from '../components/Sparkline'
import { ErrorState, LoadingState } from '../components/State'
import type { EventRecord } from '../types'
import { formatTime, number, percent } from '../utils'

export default function EventDetailPage() {
  const { id } = useParams()
  const [event, setEvent] = useState<EventRecord | null>(null)
  const [error, setError] = useState<unknown>(null)

  useEffect(() => {
    if (!id) return
    api.event(id).then(setEvent).catch(setError)
  }, [id])

  if (error) return <ErrorState error={error} />
  if (!event) return <LoadingState />

  const mark = (label: string | null) => api.labelEvent(event.id, label).then(setEvent)

  return (
    <div className="space-y-5">
      <Link to="/events" className="text-sm font-semibold text-accent">← Back to events</Link>
      <div className="card p-6">
        <div className="flex flex-wrap items-start justify-between gap-4">
          <div>
            <EventBadge type={event.type} suppressed={event.suppressed} />
            <h1 className="mt-3 text-2xl font-bold tracking-tight text-ink">{event.type.replaceAll('_', ' ')}</h1>
            <p className="mt-2 max-w-3xl text-slate-600">{event.explanation}</p>
          </div>
          <div className="flex gap-2">
            <button className="rounded-xl border border-line px-3 py-2 text-sm font-semibold text-slate-700 hover:bg-slate-50" onClick={() => mark('false_positive')}>Mark false positive</button>
            <button className="rounded-xl bg-accent px-3 py-2 text-sm font-semibold text-white" onClick={() => mark('reviewed')}>Mark reviewed</button>
          </div>
        </div>
        {event.review_label && <div className="mt-4 badge bg-blue-50 text-accent">Review: {event.review_label}</div>}
      </div>

      <section className="grid gap-4 lg:grid-cols-3">
        <div className="card p-5">
          <div className="text-sm font-semibold text-slate-500">Confidence</div>
          <div className="mt-2 text-3xl font-bold text-ink">{percent(event.confidence)}</div>
          <div className="mt-4"><Sparkline values={event.confidence_history.length ? event.confidence_history : [event.confidence]} /></div>
        </div>
        <div className="card p-5">
          <div className="text-sm font-semibold text-slate-500">Latency</div>
          <div className="mt-2 text-3xl font-bold text-ink">{number(event.processing_latency_ms, 3)} ms</div>
          <div className="mt-2 text-sm text-slate-500">Measured in edge replay loop.</div>
        </div>
        <div className="card p-5">
          <div className="text-sm font-semibold text-slate-500">Device</div>
          <div className="mt-2 text-xl font-bold text-ink">{event.device_id}</div>
          <div className="mt-2 text-sm text-slate-500">{formatTime(event.timestamp_ms)}</div>
        </div>
      </section>

      <section className="grid gap-4 lg:grid-cols-2">
        <div className="card p-5">
          <h2 className="text-lg font-bold text-ink">Decision reasons</h2>
          <ul className="mt-4 space-y-3">
            {event.reasons.map((reason, index) => <li key={index} className="rounded-xl bg-slate-50 p-3 text-sm text-slate-700">{reason}</li>)}
          </ul>
        </div>
        <div className="card p-5">
          <h2 className="text-lg font-bold text-ink">Metadata</h2>
          <pre className="mt-4 max-h-80 overflow-auto rounded-xl bg-slate-950 p-4 text-xs text-slate-100">{JSON.stringify(event.metadata, null, 2)}</pre>
        </div>
      </section>
    </div>
  )
}
