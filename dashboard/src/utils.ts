export function formatTime(ms: number): string {
  if (ms < 2_000_000_000_000) {
    // Scenario timestamps are relative milliseconds; keep the replay context visible.
    return `t+${(ms / 1000).toFixed(1)}s`
  }
  return new Date(ms).toLocaleString()
}

export function percent(value: number): string {
  return `${Math.round(value * 100)}%`
}

export function number(value: number, digits = 1): string {
  return value.toFixed(digits)
}
