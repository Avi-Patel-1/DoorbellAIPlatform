type Props = {
  values: number[]
  height?: number
}

export default function Sparkline({ values, height = 48 }: Props) {
  if (!values.length) return <div className="h-12 rounded-xl bg-slate-50" />
  const max = Math.max(...values, 1)
  const min = Math.min(...values)
  const range = Math.max(max - min, 0.001)
  const width = Math.max(120, values.length * 12)
  const points = values.map((value, index) => {
    const x = (index / Math.max(values.length - 1, 1)) * width
    const y = height - ((value - min) / range) * (height - 8) - 4
    return `${x.toFixed(1)},${y.toFixed(1)}`
  }).join(' ')
  return (
    <svg viewBox={`0 0 ${width} ${height}`} className="h-12 w-full overflow-visible">
      <polyline points={points} fill="none" stroke="currentColor" strokeWidth="3" strokeLinecap="round" strokeLinejoin="round" className="text-accent" />
    </svg>
  )
}
