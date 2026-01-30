#include "perf_counter.h"

#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

static int perf_event_open(perf_event_attr* attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return static_cast<int>(syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags));
}

PerfCounterResult measure_cpu_cycles_for_pid(int pid, int duration_ms) {
  PerfCounterResult res;

  perf_event_attr pe;
  std::memset(&pe, 0, sizeof(pe));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(pe);
  pe.config = PERF_COUNT_HW_CPU_CYCLES;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  // Attach to target pid; cpu=-1 means "any CPU"
  int fd = perf_event_open(&pe, pid, -1, -1, 0);
  if (fd == -1) {
    res.ok = false;
    res.err = errno;
    return res;
  }

  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

  // Wait while the process runs
  usleep(duration_ms * 1000);

  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  long long cycles = 0;
  if (read(fd, &cycles, sizeof(cycles)) == -1) {
    res.ok = false;
    res.err = errno;
    close(fd);
    return res;
  }

  close(fd);
  res.ok = true;
  res.cycles = cycles;
  return res;
}
