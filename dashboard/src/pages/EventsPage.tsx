import { useEffect, useMemo, useState } from 'react'
import { Link } from '../router'
import { api } from '../api'
import EventBadge from '../components/EventBadge'
import { EmptyState, ErrorState, LoadingState } from '../components/State'
import type { EventRecord } from '../types'
import { formatTime, number, percent } from '../utils'

export default function EventsPage() {
  const [events, setEvents] = useState<EventRecord[]>([])
  const [query, setQuery] = useState('')
  const [suppressionFilter, setSuppressionFilter] = useState('all')
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<unknown>(null)

  const params = useMemo(() => {
    const p = new URLSearchParams({ limit: '200' })
    if (query) p.set('q', query)
    if (suppressionFilter !== 'all') p.set('suppressed', suppressionFilter)
    return p
  }, [query, suppressionFilter])

  useEffect(() => {
    setLoading(true)
    api.events(params).then(setEvents).catch(setError).finally(() => setLoading(false))
  }, [params])

  return (
    <div className="space-y-5">
      <div className="flex flex-col justify-between gap-3 md:flex-row md:items-end">
        <div>
          <h1 className="text-2xl font-bold tracking-tight text-ink">Event timeline</h1>
          <p className="mt-1 text-sm text-slate-500">Filter alerts, suppressed decisions, and raw explanations.</p>
        </div>
        <div className="flex gap-2">
          <input className="input w-64" placeholder="Search explanation or type" value={query} onChange={e => setQuery(e.target.value)} />
          <select className="input" value={suppressionFilter} onChange={e => setSuppressionFilter(e.target.value)}>
            <option value="all">All</option>
            <option value="false">Meaningful</option>
            <option value="true">Suppressed</option>
          </select>
        </div>
      </div>

      {error ? <ErrorState error={error} /> : null}
      {loading && <LoadingState />}
      {!loading && !events.length && <EmptyState label="No events match the current filters." />}
      {!loading && events.length > 0 && (
        <div className="card overflow-hidden">
          <table className="w-full border-collapse">
            <thead className="bg-slate-50">
              <tr>
                <th className="table-head px-5 py-3">Event</th>
                <th className="table-head py-3">Confidence</th>
                <th className="table-head py-3">Latency</th>
                <th className="table-head py-3">Device</th>
                <th className="table-head py-3">Time</th>
              </tr>
            </thead>
            <tbody>
              {events.map(event => (
                <tr key={event.id} className="hover:bg-slate-50">
                  <td className="table-cell pl-5">
                    <Link to={`/events/${event.id}`} className="font-semibold text-ink hover:text-accent"><EventBadge type={event.type} suppressed={event.suppressed} /></Link>
                    <div className="mt-2 max-w-2xl text-sm text-slate-600">{event.explanation}</div>
                  </td>
                  <td className="table-cell font-semibold text-ink">{percent(event.confidence)}</td>
                  <td className="table-cell text-slate-600">{number(event.processing_latency_ms, 3)} ms</td>
                  <td className="table-cell text-slate-600">{event.device_id}</td>
                  <td className="table-cell text-slate-600">{formatTime(event.timestamp_ms)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  )
}
