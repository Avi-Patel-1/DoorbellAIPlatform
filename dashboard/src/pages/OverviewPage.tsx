import { useEffect, useState } from 'react'
import { Link } from '../router'
import { api } from '../api'
import EventBadge from '../components/EventBadge'
import Sparkline from '../components/Sparkline'
import StatCard from '../components/StatCard'
import { ErrorState, LoadingState } from '../components/State'
import type { Overview } from '../types'
import { formatTime, number, percent } from '../utils'

export default function OverviewPage() {
  const [data, setData] = useState<Overview | null>(null)
  const [error, setError] = useState<unknown>(null)

  useEffect(() => {
    let active = true
    const load = () => api.overview().then(d => active && setData(d)).catch(e => active && setError(e))
    load()
    const id = window.setInterval(load, 5000)
    return () => { active = false; window.clearInterval(id) }
  }, [])

  if (error) return <ErrorState error={error} />
  if (!data) return <LoadingState />

  const fpsValues = data.health_trend.map(m => m.fps)
  const queueValues = data.health_trend.map(m => m.queue_depth)

  return (
    <div className="space-y-6">
      <section className="grid gap-4 md:grid-cols-2 xl:grid-cols-4">
        <StatCard label="Devices online" value={`${data.devices_online}/${data.devices_total}`} detail="Heartbeat-derived status" />
        <StatCard label="Events today" value={data.events_today} detail={`${data.meaningful_events_today} meaningful, ${data.suppressed_events_today} suppressed`} />
        <StatCard label="Avg latency" value={`${number(data.avg_processing_latency_ms, 2)} ms`} detail="Edge processing latency" />
        <StatCard label="Queue depth" value={data.queue_depth} detail="Peak local outbound queue" />
      </section>

      <section className="grid gap-4 lg:grid-cols-2">
        <div className="card p-5">
          <div className="flex items-start justify-between">
            <div>
              <h2 className="text-lg font-bold text-ink">Recent device health</h2>
              <p className="text-sm text-slate-500">Replay FPS and connectivity queue depth</p>
            </div>
            <span className="badge bg-green-50 text-success">Live</span>
          </div>
          <div className="mt-6 grid gap-5 md:grid-cols-2">
            <div>
              <div className="mb-2 text-sm font-semibold text-slate-600">FPS</div>
              <Sparkline values={fpsValues} />
            </div>
            <div>
              <div className="mb-2 text-sm font-semibold text-slate-600">Queue depth</div>
              <Sparkline values={queueValues} />
            </div>
          </div>
        </div>
        <div className="card p-5">
          <h2 className="text-lg font-bold text-ink">Alert quality</h2>
          <p className="text-sm text-slate-500">Suppression is stored as first-class event history for review.</p>
          <div className="mt-5 grid grid-cols-2 gap-3">
            <div className="rounded-2xl bg-green-50 p-4">
              <div className="text-sm font-semibold text-green-700">Meaningful</div>
              <div className="mt-2 text-3xl font-bold text-green-900">{data.meaningful_events_today}</div>
            </div>
            <div className="rounded-2xl bg-slate-100 p-4">
              <div className="text-sm font-semibold text-slate-600">Suppressed</div>
              <div className="mt-2 text-3xl font-bold text-slate-900">{data.suppressed_events_today}</div>
            </div>
          </div>
          <div className="mt-4 text-sm text-slate-500">False-alert reduction preview: {data.events_today ? percent(data.suppressed_events_today / data.events_today) : '0%'}</div>
        </div>
      </section>

      <section className="card overflow-hidden">
        <div className="flex items-center justify-between border-b border-line p-5">
          <div>
            <h2 className="text-lg font-bold text-ink">Recent event timeline</h2>
            <p className="text-sm text-slate-500">Meaningful and suppressed decisions with explanations.</p>
          </div>
          <Link className="text-sm font-semibold text-accent" to="/events">View all</Link>
        </div>
        <div className="divide-y divide-slate-100">
          {data.recent_events.map(event => (
            <Link key={event.id} to={`/events/${event.id}`} className="block p-5 hover:bg-slate-50">
              <div className="flex flex-wrap items-center gap-3">
                <EventBadge type={event.type} suppressed={event.suppressed} />
                <span className="text-sm font-semibold text-ink">{percent(event.confidence)}</span>
                <span className="text-sm text-slate-500">{formatTime(event.timestamp_ms)}</span>
              </div>
              <p className="mt-2 text-sm text-slate-600">{event.explanation}</p>
            </Link>
          ))}
        </div>
      </section>
    </div>
  )
}
