import { useEffect, useState } from 'react'
import { api } from '../api'
import { ErrorState, LoadingState } from '../components/State'
import type { SettingsProfile } from '../types'

function ConfigRow({ label, value }: { label: string; value: unknown }) {
  return (
    <div className="flex items-center justify-between border-b border-slate-100 py-3">
      <div className="text-sm font-semibold text-slate-600">{label}</div>
      <div className="text-sm font-mono text-ink">{typeof value === 'object' ? JSON.stringify(value) : String(value)}</div>
    </div>
  )
}

export default function SettingsPage() {
  const [profiles, setProfiles] = useState<SettingsProfile[] | null>(null)
  const [active, setActive] = useState<SettingsProfile | null>(null)
  const [error, setError] = useState<unknown>(null)
  const [saved, setSaved] = useState(false)

  useEffect(() => {
    api.settings().then(p => { setProfiles(p); setActive(p[0] || null) }).catch(setError)
  }, [])

  if (error) return <ErrorState error={error} />
  if (!profiles || !active) return <LoadingState />

  const updateNumber = (key: string, value: number) => {
    setActive({ ...active, config: { ...active.config, [key]: value } })
    setSaved(false)
  }
  const save = () => api.saveSettings(active).then(p => { setActive(p); setSaved(true) })

  return (
    <div className="space-y-5">
      <div>
        <h1 className="text-2xl font-bold tracking-tight text-ink">Settings</h1>
        <p className="mt-1 text-sm text-slate-500">Tune confidence gates, quiet hours, home mode, cooldowns, and package zone policy.</p>
      </div>
      <section className="grid gap-4 lg:grid-cols-2">
        <div className="card p-5">
          <div className="flex items-center justify-between">
            <h2 className="text-lg font-bold text-ink">Threshold profile</h2>
            {saved && <span className="badge bg-green-50 text-success">Saved</span>}
          </div>
          <div className="mt-5 space-y-5">
            {['motion_threshold', 'person_threshold', 'package_threshold', 'linger_seconds', 'dedupe_window_seconds'].map(key => (
              <label key={key} className="block">
                <div className="mb-2 flex items-center justify-between text-sm"><span className="font-semibold text-slate-600">{key.replaceAll('_', ' ')}</span><span className="font-mono text-ink">{String(active.config[key])}</span></div>
                <input className="w-full" type="range" min="0" max={key.includes('seconds') ? '30' : '1'} step={key.includes('seconds') ? '0.5' : '0.01'} value={Number(active.config[key] ?? 0)} onChange={e => updateNumber(key, Number(e.target.value))} />
              </label>
            ))}
          </div>
          <button onClick={save} className="mt-6 rounded-xl bg-accent px-4 py-2 text-sm font-semibold text-white">Save profile</button>
        </div>
        <div className="card p-5">
          <h2 className="text-lg font-bold text-ink">Current runtime config</h2>
          <div className="mt-4">{Object.entries(active.config).map(([key, value]) => <ConfigRow key={key} label={key} value={value} />)}</div>
        </div>
      </section>
      <section className="card p-5">
        <h2 className="text-lg font-bold text-ink">Package zone preview</h2>
        <p className="mt-1 text-sm text-slate-500">The shaded zone is the configured region where package persistence is evaluated.</p>
        <div className="mt-5 aspect-video max-w-3xl rounded-2xl border border-line bg-gradient-to-b from-slate-100 to-white p-6">
          <div className="relative h-full rounded-xl border border-slate-300 bg-white">
            <div className="absolute left-[52%] top-[55%] h-[36%] w-[33%] rounded-xl border-2 border-blue-500 bg-blue-100/50" />
            <div className="absolute bottom-4 left-6 rounded-xl bg-slate-900/75 px-3 py-2 text-xs font-semibold text-white">Entry camera field of view</div>
          </div>
        </div>
      </section>
    </div>
  )
}
