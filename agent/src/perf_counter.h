#pragma once
#include <vector>
#include <cstdint>

struct CpuSample {
    uint64_t ip; // Instruction Pointer
    uint64_t timestamp;
};

struct PerfCounterResult {
    bool ok = false;
    long long total_cycles = 0;
    std::vector<CpuSample> samples;
    int err = 0;
};

// duration_ms: how long to run
// sample_rate_hz: how many times per second to capture the IP (e.g., 1000)
PerfCounterResult sample_cpu_for_pid(int pid, int duration_ms, int sample_rate_hz = 1000);