# CPUTrace Studio

> Lightweight CPU profiler for Linux — captures per-process CPU usage with <1% overhead and visualizes it in a web dashboard.

Built as a portfolio project targeting AMD internship applications, demonstrating systems programming in C++, REST API design, and React visualization.

![Dashboard](docs/dashboard.png)

## Architecture
```
┌─────────────────┐     trace.json    ┌──────────────────┐     HTTP     ┌─────────────────┐
│   C++ Agent     │ ───────────────►  │  FastAPI Backend │ ◄─────────► │ React Dashboard │
│  (cputrace CLI) │                   │  (SQLite store)  │             │  (Recharts UI)  │
└─────────────────┘                   └──────────────────┘             └─────────────────┘
   /proc/[pid]/stat                     POST /api/traces                  CPU timeline
   perf_event_open                      GET  /api/traces                  Memory chart
```

## Features

- **Zero-instrumentation monitoring** — reads `/proc/[pid]/stat` directly, no code changes needed
- **<1% CPU overhead** — safe to run in production
- **Real-time terminal output** — live CPU% and RSS display while sampling
- **Structured JSON output** — session ID, metadata, per-sample time series
- **Web dashboard** — upload traces, view CPU timeline and memory charts
- **REST API** — store and query traces via FastAPI + SQLite

## Quick Start

### 1. Build the C++ Agent
```bash
# Prerequisites: GCC 13+, CMake 3.16+
cmake -S agent -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### 2. Start the Backend
```bash
cd backend
pip install -r requirements.txt
python3 -m uvicorn main:app --port 8000
```

### 3. Start the Dashboard
```bash
cd frontend
npm install
npm run dev
# Open http://localhost:5173
```

### 4. Profile a Process
```bash
# Start a target process
./build/test_app &
PID=$!

# Profile it for 10 seconds
./build/cputrace --pid $PID --duration 10 --out trace.json --interval 100

# Upload to dashboard
curl -X POST http://localhost:8000/api/traces -F "file=@trace.json"
```

## Output Format
```json
{
  "session_id": "trace-20260221-050852",
  "start_time": "2026-02-21T05:08:52Z",
  "duration_seconds": 10,
  "metadata": {
    "pid": 1234,
    "process_name": "my_app",
    "threads": 4,
    "vm_rss_kb": 45320
  },
  "summary": {
    "cpu_avg_percent": 73.2,
    "cpu_max_percent": 98.1,
    "sample_count": 100
  },
  "samples": [
    { "t": 0.1, "cpu_pct": 72.4, "rss_kb": 45320, "threads": 4 }
  ]
}
```

## Tech Stack

| Layer | Technology | Why |
|-------|-----------|-----|
| Agent | C++17 | Zero-overhead systems programming |
| OS APIs | `/proc/[pid]/stat`, `perf_event_open` | Native Linux CPU monitoring |
| Backend | FastAPI + SQLAlchemy | Fast async Python API |
| Database | SQLite | Zero-config, embeddable |
| Frontend | React + TypeScript + Vite | Type-safe, fast dev experience |
| Charts | Recharts | Composable React chart library |

## Project Structure
```
cputrace-studio/
├── agent/                  # C++ monitoring agent
│   ├── src/
│   │   ├── main.cpp        # CLI entry point + sampling loop
│   │   ├── proc_stat.cpp   # /proc CPU sampling (no root needed)
│   │   ├── proc_status.cpp # Process metadata reader
│   │   ├── perf_counter.cpp# perf_event_open symbol sampler
│   │   └── symbol_resolver.cpp # addr2line ELF symbol lookup
│   └── CMakeLists.txt
├── backend/                # FastAPI REST API
│   ├── main.py             # API endpoints
│   ├── models.py           # SQLAlchemy ORM models
│   ├── schemas.py          # Pydantic request/response schemas
│   └── database.py         # SQLite connection
└── frontend/               # React dashboard
    └── src/
        ├── App.tsx         # Main dashboard component
        └── api.ts          # Backend API client
```

## Roadmap

- [ ] AMD Zen-specific PMU counters via `perf_event_open`
- [ ] Anomaly detection (Z-score based CPU spike alerts)
- [ ] Multi-process monitoring
- [ ] CI/CD regression detection (fail build if CPU > threshold)
- [ ] Flame graph visualization

## License

Apache 2.0
