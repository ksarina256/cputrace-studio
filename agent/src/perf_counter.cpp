#include "perf_counter.h"
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <iostream>

static int perf_event_open(perf_event_attr* attr, pid_t pid,
                           int cpu, int group_fd, unsigned long flags) {
    return static_cast<int>(
        syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags));
}

static int read_perf_paranoid() {
    std::ifstream f("/proc/sys/kernel/perf_event_paranoid");
    int val = 2;
    if (f.is_open()) f >> val;
    return val;
}

PerfCounterResult sample_cpu_for_pid(int pid, int duration_ms, int sample_rate_hz) {
    PerfCounterResult res;

    int paranoid = read_perf_paranoid();
    if (paranoid > 1) {
        std::cerr << "[perf] perf_event_paranoid=" << paranoid
                  << " — symbol sampling disabled.\n"
                  << "[perf] Fix: echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid\n";
        res.ok  = false;
        res.err = EACCES;
        return res;
    }

    perf_event_attr pe;
    std::memset(&pe, 0, sizeof(pe));
    pe.type           = PERF_TYPE_HARDWARE;
    pe.size           = sizeof(pe);
    pe.config         = PERF_COUNT_HW_CPU_CYCLES;
    pe.sample_freq    = static_cast<uint64_t>(sample_rate_hz);
    pe.freq           = 1;
    pe.sample_type    = PERF_SAMPLE_IP | PERF_SAMPLE_TID;
    pe.disabled       = 1;
    pe.exclude_kernel = 1;
    pe.mmap           = 1;

    int fd = perf_event_open(&pe, pid, -1, -1, 0);
    if (fd == -1) {
        res.ok  = false;
        res.err = errno;
        return res;
    }

    const size_t DATA_PAGES = 8;
    const size_t mmap_size  = (1 + DATA_PAGES) * 4096;
    void* ring_buf = mmap(nullptr, mmap_size,
                          PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ring_buf == MAP_FAILED) {
        close(fd);
        res.ok  = false;
        res.err = errno;
        return res;
    }

    ioctl(fd, PERF_EVENT_IOC_RESET,  0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    usleep(static_cast<useconds_t>(duration_ms) * 1000UL);
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    read(fd, &res.total_cycles, sizeof(res.total_cycles));

    auto* meta     = reinterpret_cast<struct perf_event_mmap_page*>(ring_buf);
    uint64_t head  = meta->data_head;
    uint64_t tail  = meta->data_tail;
    auto* data     = reinterpret_cast<unsigned char*>(ring_buf) + 4096;
    const size_t data_size = DATA_PAGES * 4096;

    while (tail < head) {
        size_t offset = tail % data_size;
        auto* hdr = reinterpret_cast<struct perf_event_header*>(&data[offset]);
        if (hdr->size == 0) break;
        if (hdr->type == PERF_RECORD_SAMPLE) {
            uint64_t ip;
            std::memcpy(&ip, &data[(offset + sizeof(perf_event_header)) % data_size], sizeof(ip));
            res.samples.push_back({ip, 0});
        }
        tail += hdr->size;
    }

    munmap(ring_buf, mmap_size);
    close(fd);
    res.ok = true;
    return res;
}
