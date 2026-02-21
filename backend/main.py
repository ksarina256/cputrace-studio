import json
from fastapi import FastAPI, UploadFile, File, HTTPException, Depends
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.orm import Session
from typing import List

from database import Base, engine, get_db
from models import Trace
from schemas import TraceSummary, TraceDetail

# Create tables on startup
Base.metadata.create_all(bind=engine)

app = FastAPI(title="CPUTrace API", version="0.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/health")
def health():
    return {"status": "ok", "version": "0.1.0"}

@app.post("/api/traces", response_model=TraceSummary)
async def upload_trace(file: UploadFile = File(...), db: Session = Depends(get_db)):
    if not file.filename.endswith(".json"):
        raise HTTPException(400, "Only .json files accepted")

    content = await file.read()
    try:
        data = json.loads(content)
    except json.JSONDecodeError:
        raise HTTPException(400, "Invalid JSON")

    # Check required fields
    for field in ("session_id", "metadata", "summary"):
        if field not in data:
            raise HTTPException(400, f"Missing field: {field}")

    # Check for duplicate session
    existing = db.query(Trace).filter(
        Trace.session_id == data["session_id"]
    ).first()
    if existing:
        raise HTTPException(409, f"session_id {data['session_id']} already exists")

    trace = Trace(
        session_id   = data["session_id"],
        process_name = data["metadata"].get("process_name", "unknown"),
        pid          = data["metadata"].get("pid", 0),
        duration_sec = data.get("duration_seconds", 0),
        cpu_avg      = data["summary"].get("cpu_avg_percent", 0.0),
        cpu_max      = data["summary"].get("cpu_max_percent", 0.0),
        sample_count = data["summary"].get("sample_count", 0),
        start_time   = data.get("start_time", ""),
        raw_json     = content.decode("utf-8"),
    )
    db.add(trace)
    db.commit()
    db.refresh(trace)
    return trace

@app.get("/api/traces", response_model=List[TraceSummary])
def list_traces(skip: int = 0, limit: int = 20, db: Session = Depends(get_db)):
    return db.query(Trace).order_by(Trace.created_at.desc()).offset(skip).limit(limit).all()

@app.get("/api/traces/{trace_id}", response_model=TraceDetail)
def get_trace(trace_id: int, db: Session = Depends(get_db)):
    trace = db.query(Trace).filter(Trace.id == trace_id).first()
    if not trace:
        raise HTTPException(404, "Trace not found")
    return trace
