const labels: Record<string, string> = {
  person_at_door: 'Person',
  visitor_linger: 'Linger',
  package_delivered: 'Package delivered',
  package_removed: 'Package removed',
  duplicate_event_suppressed: 'Duplicate suppressed',
  low_confidence_motion_suppressed: 'Low confidence',
  motion_without_detection_suppressed: 'Motion suppressed',
  repeated_motion: 'Repeated motion',
}

export default function EventBadge({ type, suppressed }: { type: string; suppressed?: boolean }) {
  const cls = suppressed ? 'bg-slate-100 text-slate-600' : type.includes('package') ? 'bg-blue-50 text-accent' : type.includes('linger') ? 'bg-amber-50 text-warning' : 'bg-green-50 text-success'
  return <span className={`badge ${cls}`}>{labels[type] || type.replaceAll('_', ' ')}</span>
}
