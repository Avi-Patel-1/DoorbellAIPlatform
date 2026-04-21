export default function StatusBadge({ value }: { value: string }) {
  const cls = value === 'online' ? 'bg-green-50 text-success' : value === 'offline' ? 'bg-red-50 text-danger' : 'bg-amber-50 text-warning'
  return <span className={`badge ${cls}`}>{value}</span>
}
