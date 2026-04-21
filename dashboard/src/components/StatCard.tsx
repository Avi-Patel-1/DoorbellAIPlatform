type Props = {
  label: string
  value: string | number
  detail?: string
}

export default function StatCard({ label, value, detail }: Props) {
  return (
    <div className="card p-5">
      <div className="text-sm font-semibold text-slate-500">{label}</div>
      <div className="mt-2 text-3xl font-bold tracking-tight text-ink">{value}</div>
      {detail && <div className="mt-2 text-sm text-slate-500">{detail}</div>}
    </div>
  )
}
