#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

static int perf_event_open(perf_event_attr* attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return static_cast<int>(syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags));
}

int main() {
  perf_event_attr pe;
  std::memset(&pe, 0, sizeof(pe));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(pe);
  pe.config = PERF_COUNT_HW_CPU_CYCLES;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  // pid=0 means "this process"
  int fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    std::cerr << "perf_event_open failed: " << std::strerror(errno) << "\n";
    std::cerr << "If you're on WSL, perf may be restricted. Try: sudo ./perf_hello\n";
    return 1;
  }

  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

  // Burn CPU so we have something to count
  volatile long x = 0;
  for (long i = 0; i < 150000000; i++) x += i;

  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  long long cycles = 0;
  if (read(fd, &cycles, sizeof(cycles)) == -1) {
    std::cerr << "read failed: " << std::strerror(errno) << "\n";
    close(fd);
    return 1;
  }

  close(fd);
  std::cout << "CPU cycles: " << cycles << "\n";
  return 0;
}
