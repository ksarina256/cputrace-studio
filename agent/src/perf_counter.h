#pragma once
#include <cstdint>

struct PerfCounterResult {
  bool ok = false;
  long long cycles = 0;
  int err = 0;
};

PerfCounterResult measure_cpu_cycles_for_pid(int pid, int duration_ms);
