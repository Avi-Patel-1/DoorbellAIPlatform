import { useEffect, useState } from 'react'
import { api } from '../api'
import Sparkline from '../components/Sparkline'
import { EmptyState, ErrorState, LoadingState } from '../components/State'
import StatusBadge from '../components/StatusBadge'
import type { Device, DeviceMetric } from '../types'
import { number } from '../utils'

export default function DevicesPage() {
  const [devices, setDevices] = useState<Device[]>([])
  const [metrics, setMetrics] = useState<Record<string, DeviceMetric[]>>({})
  const [error, setError] = useState<unknown>(null)
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    api.devices().then(async ds => {
      setDevices(ds)
      const entries = await Promise.all(ds.map(async d => [d.id, await api.deviceMetrics(d.id)] as const))
      setMetrics(Object.fromEntries(entries))
    }).catch(setError).finally(() => setLoading(false))
  }, [])

  if (error) return <ErrorState error={error} />
  if (loading) return <LoadingState />
  if (!devices.length) return <EmptyState label="No devices have registered yet. Start the simulator or edge runtime to populate this page." />

  return (
    <div className="space-y-5">
      <div>
        <h1 className="text-2xl font-bold tracking-tight text-ink">Devices</h1>
        <p className="mt-1 text-sm text-slate-500">Runtime health, queue depth, and connectivity state for each edge device.</p>
      </div>
      <div className="grid gap-4">
        {devices.map(device => {
          const deviceMetrics = metrics[device.id] || []
          return (
            <div key={device.id} className="card p-5">
              <div className="flex flex-wrap items-start justify-between gap-4">
                <div>
                  <div className="flex items-center gap-3"><h2 className="text-lg font-bold text-ink">{device.name}</h2><StatusBadge value={device.status} /></div>
                  <div className="mt-1 text-sm text-slate-500">{device.id} · {device.location} · {device.firmware_version}</div>
                </div>
                <div className="text-right text-sm text-slate-500">Last seen<br /><span className="font-semibold text-ink">{device.last_seen ? new Date(device.last_seen).toLocaleString() : 'Never'}</span></div>
              </div>
              <div className="mt-5 grid gap-4 md:grid-cols-4">
                <div className="rounded-2xl bg-slate-50 p-4"><div className="text-sm text-slate-500">Events</div><div className="mt-1 text-2xl font-bold text-ink">{device.event_counts.events || 0}</div></div>
                <div className="rounded-2xl bg-slate-50 p-4"><div className="text-sm text-slate-500">Queue</div><div className="mt-1 text-2xl font-bold text-ink">{device.latest_metric?.queue_depth ?? 0}</div></div>
                <div className="rounded-2xl bg-slate-50 p-4"><div className="text-sm text-slate-500">FPS</div><div className="mt-1 text-2xl font-bold text-ink">{number(device.latest_metric?.fps ?? 0)}</div></div>
                <div className="rounded-2xl bg-slate-50 p-4"><div className="text-sm text-slate-500">Memory</div><div className="mt-1 text-2xl font-bold text-ink">{number(device.latest_metric?.memory_rss_mb ?? 0)} MB</div></div>
              </div>
              <div className="mt-5 grid gap-4 md:grid-cols-2">
                <div><div className="mb-2 text-sm font-semibold text-slate-600">FPS trend</div><Sparkline values={deviceMetrics.map(m => m.fps)} /></div>
                <div><div className="mb-2 text-sm font-semibold text-slate-600">Queue trend</div><Sparkline values={deviceMetrics.map(m => m.queue_depth)} /></div>
              </div>
            </div>
          )
        })}
      </div>
    </div>
  )
}
