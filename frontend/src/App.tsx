import { useState, useEffect, useCallback } from "react";
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, ReferenceLine } from "recharts";
import { uploadTrace, listTraces, getTrace } from "./api";
import type { TraceSummary, CpuSample } from "./api";
import "./App.css";

export default function App() {
  const [traces, setTraces]         = useState<TraceSummary[]>([]);
  const [selected, setSelected]     = useState<TraceSummary | null>(null);
  const [samples, setSamples]       = useState<CpuSample[]>([]);
  const [uploading, setUploading]   = useState(false);
  const [error, setError]           = useState("");

  const fetchTraces = useCallback(async () => {
    const res = await listTraces();
    setTraces(res.data);
  }, []);

  useEffect(() => { fetchTraces(); }, [fetchTraces]);

  const handleUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setUploading(true);
    setError("");
    try {
      await uploadTrace(file);
      await fetchTraces();
    } catch (err: any) {
      setError(err.response?.data?.detail ?? "Upload failed");
    } finally {
      setUploading(false);
      e.target.value = "";
    }
  };

  const handleSelect = async (trace: TraceSummary) => {
    setSelected(trace);
    const res = await getTrace(trace.id);
    const raw = JSON.parse(res.data.raw_json);
    setSamples(raw.samples ?? []);
  };


  const computeStats = (data: CpuSample[]) => {
    if (data.length === 0) return null;
    const sorted = [...data].map(s => s.cpu_pct).sort((a, b) => a - b);
    const p50 = sorted[Math.floor(sorted.length * 0.50)];
    const p95 = sorted[Math.floor(sorted.length * 0.95)];
    const p99 = sorted[Math.floor(sorted.length * 0.99)];
    const mean = sorted.reduce((a, b) => a + b, 0) / sorted.length;
    const stddev = Math.sqrt(sorted.reduce((a, b) => a + Math.pow(b - mean, 2), 0) / sorted.length);
    const spikes = data.filter(s => s.cpu_pct > mean + 2 * stddev);
    return { p50, p95, p99, mean, stddev, spikes };
  };
  const cpuColor = (pct: number) =>
    pct > 80 ? "#ef4444" : pct > 50 ? "#f59e0b" : "#22c55e";

  return (
    <div className="app">
      <header>
        <h1>⚡ CPUTrace Studio</h1>
        <label className={`upload-btn ${uploading ? "disabled" : ""}`}>
          {uploading ? "Uploading..." : "Upload trace.json"}
          <input type="file" accept=".json" onChange={handleUpload} hidden />
        </label>
      </header>

      {error && <div className="error">{error}</div>}

      <div className="layout">
        {/* Trace list */}
        <aside>
          <h2>Traces</h2>
          {traces.length === 0 && <p className="muted">No traces yet</p>}
          {traces.map(t => (
            <div
              key={t.id}
              className={`trace-card ${selected?.id === t.id ? "active" : ""}`}
              onClick={() => handleSelect(t)}
            >
              <div className="trace-name">{t.process_name}</div>
              <div className="trace-meta">PID {t.pid} · {t.duration_sec}s</div>
              <div className="trace-cpu">
                avg <span style={{ color: cpuColor(t.cpu_avg) }}>
                  {t.cpu_avg.toFixed(1)}%
                </span>
                {" "}max <span style={{ color: cpuColor(t.cpu_max) }}>
                  {t.cpu_max.toFixed(1)}%
                </span>
              </div>
            </div>
          ))}
        </aside>

        {/* Chart panel */}
        <main>
          {!selected && (
            <div className="empty">
              Select a trace to view the CPU timeline
            </div>
          )}
          {selected && (
            <>
              <div className="panel-header">
                <h2>{selected.process_name} <span className="muted">PID {selected.pid}</span></h2>
                <div className="stats">
                  <span>avg <b>{selected.cpu_avg.toFixed(1)}%</b></span>
                  <span>max <b>{selected.cpu_max.toFixed(1)}%</b></span>
                  <span>samples <b>{selected.sample_count}</b></span>
                  <span>duration <b>{selected.duration_sec}s</b></span>
                </div>
              </div>
              <ResponsiveContainer width="100%" height={320}>
                <LineChart data={samples} margin={{ top: 8, right: 24, bottom: 8, left: 0 }}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
                  <XAxis
                    dataKey="t"
                    tickFormatter={v => `${v.toFixed(1)}s`}
                    stroke="#94a3b8"
                  />
                  <YAxis domain={[0, 100]} unit="%" stroke="#94a3b8" />
                  <Tooltip
                    formatter={(v: number) => [`${v.toFixed(2)}%`, "CPU"]}
                    labelFormatter={l => `t=${Number(l).toFixed(2)}s`}
                    contentStyle={{ background: "#1e293b", border: "1px solid #334155" }}
                  />
                  <ReferenceLine y={80} stroke="#ef4444" strokeDasharray="4 4" label={{ value: "80%", fill: "#ef4444", fontSize: 11 }} />
                  <ReferenceLine y={50} stroke="#f59e0b" strokeDasharray="4 4" label={{ value: "50%", fill: "#f59e0b", fontSize: 11 }} />
                  <Line
                    type="monotone"
                    dataKey="cpu_pct"
                    stroke="#38bdf8"
                    strokeWidth={2}
                    dot={false}
                    isAnimationActive={false}
                  />
                </LineChart>
              </ResponsiveContainer>

              <h3>Memory (RSS)</h3>
              <ResponsiveContainer width="100%" height={180}>
                <LineChart data={samples} margin={{ top: 4, right: 24, bottom: 4, left: 0 }}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
                  <XAxis dataKey="t" tickFormatter={v => `${v.toFixed(1)}s`} stroke="#94a3b8" />
                  <YAxis stroke="#94a3b8" tickFormatter={v => `${(v/1024).toFixed(0)}MB`} />
                  <Tooltip
                    formatter={(v: number) => [`${(v/1024).toFixed(2)} MB`, "RSS"]}
                    contentStyle={{ background: "#1e293b", border: "1px solid #334155" }}
                  />
                  <Line type="monotone" dataKey="rss_kb" stroke="#a78bfa" strokeWidth={2} dot={false} isAnimationActive={false} />
                </LineChart>
              </ResponsiveContainer>

              {(() => {
                const stats = computeStats(samples);
                if (!stats) return null;
                return (
                  <>
                    <h3>Statistics</h3>
                    <div className="stats-grid">
                      <div className="stat-box">
                        <div className="stat-label">P50</div>
                        <div className="stat-value" style={{color: cpuColor(stats.p50)}}>{stats.p50.toFixed(1)}%</div>
                      </div>
                      <div className="stat-box">
                        <div className="stat-label">P95</div>
                        <div className="stat-value" style={{color: cpuColor(stats.p95)}}>{stats.p95.toFixed(1)}%</div>
                      </div>
                      <div className="stat-box">
                        <div className="stat-label">P99</div>
                        <div className="stat-value" style={{color: cpuColor(stats.p99)}}>{stats.p99.toFixed(1)}%</div>
                      </div>
                      <div className="stat-box">
                        <div className="stat-label">Std Dev</div>
                        <div className="stat-value">{stats.stddev.toFixed(2)}%</div>
                      </div>
                      <div className="stat-box">
                        <div className="stat-label">Spikes</div>
                        <div className="stat-value" style={{color: stats.spikes.length > 0 ? "#ef4444" : "#22c55e"}}>
                          {stats.spikes.length}
                        </div>
                      </div>
                    </div>
                    {stats.spikes.length > 0 && (
                      <div className="spike-warning">
                        ⚠ {stats.spikes.length} CPU spike{stats.spikes.length > 1 ? "s" : ""} detected
                        ({">"}{ (stats.mean + 2 * stats.stddev).toFixed(1)}% threshold)
                      </div>
                    )}
                  </>
                );
              })()}
            </>
          )}
        </main>
      </div>
    </div>
  );
}
