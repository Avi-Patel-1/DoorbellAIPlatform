# Event intelligence

The event layer avoids alerting on every motion frame. It combines:

- motion gating
- temporal confidence history
- entry-zone and package-zone checks
- person-linger timers
- package delivered and removed persistence windows
- dedupe and cooldown windows
- quiet-hours and home/away policy
- explainable event records

Each event stores `reasons`, a human-readable `explanation`, `confidence_history`, local media references, processing latency, and raw metadata.
