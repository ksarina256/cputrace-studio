#pragma once
#include <vector>
#include <cstdint>

struct CpuSample {
    uint64_t ip;
    uint64_t timestamp;
};

struct PerfCounterResult {
    bool ok = false;
    long long total_cycles = 0;
    std::vector<CpuSample> samples;
    int err = 0;
};

PerfCounterResult sample_cpu_for_pid(int pid, int duration_ms, int sample_rate_hz = 1000);
