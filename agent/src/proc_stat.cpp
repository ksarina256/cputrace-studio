#include "proc_stat.h"
#include <fstream>
#include <sstream>
#include <sys/time.h>

bool read_process_cpu_snap(int pid, ProcessCpuSnap& out) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f.is_open()) return false;

    std::string line;
    std::getline(f, line);

    auto rp = line.rfind(')');
    if (rp == std::string::npos) return false;

    std::istringstream iss(line.substr(rp + 2));
    std::string state;
    long ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    unsigned long utime, stime;

    iss >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid
        >> flags >> minflt >> cminflt >> majflt >> cmajflt
        >> utime >> stime;

    out.utime = utime;
    out.stime = stime;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    out.timestamp_us = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
    return true;
}

void read_system_cpu_snap(SystemCpuSnap& out) {
    std::ifstream f("/proc/stat");
    std::string tag;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    f >> tag >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    out.idle  = idle + iowait;
    out.total = user + nice + system + idle + iowait + irq + softirq + steal;
}

double compute_cpu_percent(
    const ProcessCpuSnap& s1, const ProcessCpuSnap& s2,
    const SystemCpuSnap& sys1, const SystemCpuSnap& sys2)
{
    double proc_delta   = (double)((s2.utime + s2.stime) - (s1.utime + s1.stime));
    double system_delta = (double)(sys2.total - sys1.total);
    if (system_delta <= 0.0) return -1.0;
    return (proc_delta / system_delta) * 100.0;
}
