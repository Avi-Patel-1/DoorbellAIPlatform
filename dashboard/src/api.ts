import type { BenchmarkRun, Device, DeviceMetric, EventRecord, Overview, SettingsProfile } from './types'

const API_BASE = import.meta.env.VITE_API_BASE_URL || 'http://localhost:8000'

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${API_BASE}${path}`, {
    headers: { 'Content-Type': 'application/json', ...(init?.headers || {}) },
    ...init,
  })
  if (!response.ok) {
    const detail = await response.text()
    throw new Error(`${response.status} ${response.statusText}: ${detail}`)
  }
  return response.json() as Promise<T>
}

export const api = {
  baseUrl: API_BASE,
  overview: () => request<Overview>('/overview'),
  events: (params: URLSearchParams) => request<EventRecord[]>(`/events?${params.toString()}`),
  event: (id: string) => request<EventRecord>(`/events/${id}`),
  labelEvent: (id: string, label: string | null) => request<EventRecord>(`/events/${id}/label`, { method: 'PATCH', body: JSON.stringify({ label }) }),
  devices: () => request<Device[]>('/devices'),
  deviceMetrics: (deviceId: string) => request<DeviceMetric[]>(`/devices/${deviceId}/metrics`),
  benchmarks: () => request<BenchmarkRun[]>('/benchmarks'),
  settings: () => request<SettingsProfile[]>('/settings'),
  saveSettings: (profile: SettingsProfile) => request<SettingsProfile>(`/settings/${profile.id}`, { method: 'PUT', body: JSON.stringify(profile) }),
  exportJsonUrl: `${API_BASE}/exports/events.json`,
  exportCsvUrl: `${API_BASE}/exports/events.csv`,
}
