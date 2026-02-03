#include "perf_counter.h"
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

static int perf_event_open(perf_event_attr* attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return static_cast<int>(syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags));
}

PerfCounterResult sample_cpu_for_pid(int pid, int duration_ms, int sample_rate_hz) {
    PerfCounterResult res;
    perf_event_attr pe;
    std::memset(&pe, 0, sizeof(pe));
    
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(pe);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.sample_freq = sample_rate_hz;
    pe.freq = 1; // Use frequency instead of period
    pe.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.mmap = 1;

    int fd = perf_event_open(&pe, pid, -1, -1, 0);
    if (fd == -1) {
        res.ok = false;
        res.err = errno;
        return res;
    }

    size_t mmap_size = (1 + 8) * 4096;
    void* ring_buffer = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    usleep(duration_ms * 1000);

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    read(fd, &res.total_cycles, sizeof(res.total_cycles));

    struct perf_event_mmap_page* metadata = (struct perf_event_mmap_page*)ring_buffer;
    uint64_t head = metadata->data_head;
    uint64_t tail = metadata->data_tail;
    unsigned char* data = (unsigned char*)ring_buffer + 4096;

    // Very basic parsing of the ring buffer for the JSON output
    while (tail < head) {
        struct {
            struct perf_event_header header;
            uint64_t ip;
            uint32_t pid, tid;
        }* event = (decltype(event))&data[tail % (8 * 4096)];
        
        if (event->header.type == PERF_RECORD_SAMPLE) {
            res.samples.push_back({event->ip, 0});
        }
        tail += event->header.size;
    }

    munmap(ring_buffer, mmap_size);
    close(fd);
    res.ok = true;
    return res;
}