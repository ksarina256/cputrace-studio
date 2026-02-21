#pragma once
#include <cstdint>

struct ProcessCpuSnap {
    uint64_t utime;
    uint64_t stime;
    uint64_t timestamp_us;
};

struct SystemCpuSnap {
    uint64_t total;
    uint64_t idle;
};

bool read_process_cpu_snap(int pid, ProcessCpuSnap& out);
void read_system_cpu_snap(SystemCpuSnap& out);
double compute_cpu_percent(
    const ProcessCpuSnap& s1, const ProcessCpuSnap& s2,
    const SystemCpuSnap& sys1, const SystemCpuSnap& sys2);
