export function LoadingState({ label = 'Loading data…' }: { label?: string }) {
  return <div className="card p-8 text-center text-sm text-slate-500">{label}</div>
}

export function EmptyState({ label }: { label: string }) {
  return <div className="rounded-2xl border border-dashed border-line bg-white p-8 text-center text-sm text-slate-500">{label}</div>
}

export function ErrorState({ error }: { error: unknown }) {
  return <div className="rounded-2xl border border-red-100 bg-red-50 p-5 text-sm text-red-700">{error instanceof Error ? error.message : 'Unexpected error'}</div>
}
