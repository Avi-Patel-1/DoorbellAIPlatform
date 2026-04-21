export type EventRecord = {
  id: string
  device_id: string
  type: string
  severity: string
  timestamp_ms: number
  confidence: number
  suppressed: boolean
  reasons: string[]
  explanation: string
  confidence_history: number[]
  snapshot_ref: string
  clip_ref: string
  processing_latency_ms: number
  metadata: Record<string, unknown>
  review_label?: string | null
  created_at: string
}

export type DeviceMetric = {
  id: number
  device_id: string
  timestamp_ms: number
  fps: number
  cpu_load_1m: number
  memory_rss_mb: number
  queue_depth: number
  dropped_frames: number
  network_status: string
  uptime_seconds: number
  created_at: string
}

export type Device = {
  id: string
  name: string
  location: string
  status: string
  firmware_version: string
  last_seen: string | null
  created_at: string
  latest_metric: DeviceMetric | null
  event_counts: Record<string, number>
}

export type Overview = {
  devices_online: number
  devices_total: number
  events_today: number
  meaningful_events_today: number
  suppressed_events_today: number
  avg_processing_latency_ms: number
  queue_depth: number
  recent_events: EventRecord[]
  health_trend: DeviceMetric[]
}

export type BenchmarkRun = {
  id: number
  name: string
  created_at: string
  metrics: Record<string, number | string | boolean>
}

export type SettingsProfile = {
  id: string
  name: string
  config: Record<string, unknown>
  updated_at: string
}
