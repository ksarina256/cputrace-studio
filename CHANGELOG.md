# Changelog

## [v0.2.0] - 2026-02-22
### Added
- Statistics panel: P50/P95/P99 percentiles and spike detection
- GitHub Actions CI pipeline for C++ agent, Python backend, React frontend

### Changed
- Polished README with architecture diagram and quick start guide

## [v0.1.0] - 2026-02-21
### Added
- C++ agent: /proc-based CPU sampling with <1% overhead
- FastAPI backend: SQLite trace storage with REST API
- React dashboard: CPU timeline and memory charts
- Full pipeline: cputrace CLI -> JSON -> API -> visualization
