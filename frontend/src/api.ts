import axios from "axios";

const BASE = "http://localhost:8000";

export interface TraceSummary {
  id: number;
  session_id: string;
  process_name: string;
  pid: number;
  duration_sec: number;
  cpu_avg: number;
  cpu_max: number;
  sample_count: number;
  start_time: string;
  created_at: string;
}

export interface TraceDetail extends TraceSummary {
  raw_json: string;
}

export interface CpuSample {
  t: number;
  cpu_pct: number;
  rss_kb: number;
  threads: number;
}

export const uploadTrace = (file: File) => {
  const form = new FormData();
  form.append("file", file);
  return axios.post<TraceSummary>(`${BASE}/api/traces`, form);
};

export const listTraces = () =>
  axios.get<TraceSummary[]>(`${BASE}/api/traces`);

export const getTrace = (id: number) =>
  axios.get<TraceDetail>(`${BASE}/api/traces/${id}`);
