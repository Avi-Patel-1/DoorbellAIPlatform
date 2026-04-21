import { createContext, useContext, type MouseEvent, type ReactNode } from 'react'

type RouterValue = {
  path: string
  navigate: (to: string) => void
}

export const RouterContext = createContext<RouterValue>({ path: '/', navigate: () => {} })

export function Link({ to, className, children }: { to: string; className?: string; children: ReactNode }) {
  const { navigate } = useContext(RouterContext)
  const onClick = (event: MouseEvent<HTMLAnchorElement>) => {
    if (event.metaKey || event.ctrlKey || event.shiftKey || event.altKey || event.button !== 0) return
    event.preventDefault()
    navigate(to)
  }
  return <a href={to} className={className} onClick={onClick}>{children}</a>
}

export function NavLink({ to, className, children }: { to: string; className?: string | ((state: { isActive: boolean }) => string); children: ReactNode }) {
  const { path, navigate } = useContext(RouterContext)
  const isActive = to === '/' ? path === '/' : path === to || path.startsWith(`${to}/`)
  const resolvedClassName = typeof className === 'function' ? className({ isActive }) : className
  const onClick = (event: MouseEvent<HTMLAnchorElement>) => {
    if (event.metaKey || event.ctrlKey || event.shiftKey || event.altKey || event.button !== 0) return
    event.preventDefault()
    navigate(to)
  }
  return <a href={to} className={resolvedClassName} onClick={onClick}>{children}</a>
}

export function useParams(): { id?: string } {
  const { path } = useContext(RouterContext)
  const parts = path.split('/').filter(Boolean)
  if (parts[0] === 'events' && parts[1]) return { id: decodeURIComponent(parts[1]) }
  return {}
}
