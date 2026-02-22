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
            </>
          )}
        </main>
      </div>
    </div>
  );
}
