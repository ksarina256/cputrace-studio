from pydantic import BaseModel
from typing import Optional
from datetime import datetime

class TraceSummary(BaseModel):
    id: int
    session_id: str
    process_name: str
    pid: int
    duration_sec: int
    cpu_avg: float
    cpu_max: float
    sample_count: int
    start_time: str
    created_at: Optional[datetime] = None

    model_config = {"from_attributes": True}

class TraceDetail(TraceSummary):
    raw_json: str
