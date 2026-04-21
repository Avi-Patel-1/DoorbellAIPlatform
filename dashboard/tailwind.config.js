export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  theme: {
    extend: {
      colors: {
        ink: '#102033',
        muted: '#64748b',
        panel: '#ffffff',
        line: '#dbe3ec',
        accent: '#2563eb',
        success: '#15803d',
        warning: '#b45309',
        danger: '#b91c1c'
      },
      boxShadow: {
        card: '0 8px 30px rgba(15, 23, 42, 0.08)'
      }
    },
  },
  plugins: [],
}
