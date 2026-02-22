from sqlalchemy import Column, Integer, String, Float, DateTime, ForeignKey, Text
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from database import Base

class Trace(Base):
    __tablename__ = "traces"

    id            = Column(Integer, primary_key=True, index=True)
    session_id    = Column(String, unique=True, index=True)
    process_name  = Column(String)
    pid           = Column(Integer)
    duration_sec  = Column(Integer)
    cpu_avg       = Column(Float)
    cpu_max       = Column(Float)
    sample_count  = Column(Integer)
    start_time    = Column(String)
    raw_json      = Column(Text)   # store full trace for retrieval
    created_at    = Column(DateTime, server_default=func.now())
