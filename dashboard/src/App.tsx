import { useEffect, useState } from 'react'
import { NavLink, RouterContext } from './router'
import OverviewPage from './pages/OverviewPage'
import EventsPage from './pages/EventsPage'
import EventDetailPage from './pages/EventDetailPage'
import DevicesPage from './pages/DevicesPage'
import SettingsPage from './pages/SettingsPage'
import BenchmarksPage from './pages/BenchmarksPage'
import DemoReplayPage from './pages/DemoReplayPage'

const nav = [
  ['/', 'Overview'],
  ['/events', 'Events'],
  ['/devices', 'Devices'],
  ['/settings', 'Settings'],
  ['/benchmarks', 'Benchmarks'],
  ['/demo', 'Demo replay'],
]

export default function App() {
  const [path, setPath] = useState(window.location.pathname)
  useEffect(() => {
    const onPop = () => setPath(window.location.pathname)
    window.addEventListener('popstate', onPop)
    return () => window.removeEventListener('popstate', onPop)
  }, [])
  const navigate = (to: string) => {
    window.history.pushState(null, '', to)
    setPath(window.location.pathname)
  }
  const page = path.startsWith('/events/')
    ? <EventDetailPage />
    : path.startsWith('/events')
      ? <EventsPage />
      : path.startsWith('/devices')
        ? <DevicesPage />
        : path.startsWith('/settings')
          ? <SettingsPage />
          : path.startsWith('/benchmarks')
            ? <BenchmarksPage />
            : path.startsWith('/demo')
              ? <DemoReplayPage />
              : <OverviewPage />

  return (
    <RouterContext.Provider value={{ path, navigate }}>
      <div className="min-h-screen bg-slate-50">
      <aside className="fixed inset-y-0 left-0 hidden w-64 border-r border-line bg-white px-5 py-6 lg:block">
        <div className="mb-9">
          <div className="text-xl font-bold tracking-tight text-ink">Porchlight</div>
          <div className="mt-1 text-sm text-slate-500">Device intelligence console</div>
        </div>
        <nav className="space-y-1">
          {nav.map(([to, label]) => (
            <NavLink key={to} to={to} className={({ isActive }) => `block rounded-xl px-3 py-2 text-sm font-medium ${isActive ? 'bg-blue-50 text-accent' : 'text-slate-600 hover:bg-slate-50 hover:text-ink'}`}>
              {label}
            </NavLink>
          ))}
        </nav>
        <div className="absolute bottom-6 left-5 right-5 rounded-2xl bg-slate-50 p-4 text-xs text-slate-500">
          Local-first edge runtime with offline queueing, suppression logic, and replayable benchmarks.
        </div>
      </aside>
      <main className="lg:pl-64">
        <header className="sticky top-0 z-10 border-b border-line bg-white/85 px-5 py-4 backdrop-blur lg:px-8">
          <div className="flex items-center justify-between">
            <div>
              <div className="text-sm font-semibold text-slate-500">Front entry operations</div>
              <div className="text-lg font-bold text-ink">Alert review and device health</div>
            </div>
            <a className="rounded-xl border border-line px-3 py-2 text-sm font-semibold text-slate-700 hover:bg-slate-50" href="/exports/events.csv">Export CSV</a>
          </div>
        </header>
        <div className="px-5 py-6 lg:px-8">
          {page}
        </div>
      </main>
      </div>
    </RouterContext.Provider>
  )
}
