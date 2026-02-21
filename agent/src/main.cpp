#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cstdlib>
#include <ctime>

#include "json_writer.h"
#include "proc_status.h"
#include "proc_stat.h"
#include "perf_counter.h"
#include "symbol_resolver.h"

static void print_usage() {
    std::cerr << "CPUTrace Agent v0.1\n"
                 "Usage:\n"
                 "  cputrace --pid <pid> --duration <seconds> --out <file.json> [--interval <ms>]\n";
}

static bool read_arg(int argc, char** argv,
                     const std::string& key, std::string& out) {
    for (int i = 1; i < argc - 1; i++) {
        if (std::string(argv[i]) == key) {
            out = argv[i + 1];
            return true;
        }
    }
    return false;
}

static std::string iso8601_now() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return buf;
}

static std::string make_session_id() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "trace-%Y%m%d-%H%M%S", std::gmtime(&t));
    return buf;
}

struct TimedSample {
    double timestamp_sec;
    double cpu_percent;
    long   vm_rss_kb;
    int    threads;
};

int main(int argc, char** argv) {
    std::string pid_str, dur_str, out_path, interval_str = "100";

    if (!read_arg(argc, argv, "--pid",      pid_str) ||
        !read_arg(argc, argv, "--duration", dur_str)  ||
        !read_arg(argc, argv, "--out",      out_path)) {
        print_usage();
        return 2;
    }
    read_arg(argc, argv, "--interval", interval_str);

    int pid          = std::stoi(pid_str);
    int duration_sec = std::stoi(dur_str);
    int interval_ms  = std::stoi(interval_str);

    std::cerr << "[CPUTrace] pid=" << pid
              << "  duration=" << duration_sec << "s"
              << "  interval=" << interval_ms << "ms\n";

    // optional perf symbol sampling (may fail on WSL without sudo)
    auto perf = sample_cpu_for_pid(pid, duration_sec * 1000, 1000);
    if (!perf.ok) {
        std::cerr << "[CPUTrace] Symbol sampling disabled (perf unavailable).\n";
    }

    std::string session_id = make_session_id();
    std::string start_time = iso8601_now();
    ProcStatus  ps_initial = read_proc_status(pid);
    auto        wall_start = std::chrono::steady_clock::now();
    auto        deadline   = wall_start + std::chrono::seconds(duration_sec);

    ProcessCpuSnap cpu_prev;
    SystemCpuSnap  sys_prev;
    if (!read_process_cpu_snap(pid, cpu_prev)) {
        std::cerr << "[CPUTrace] Cannot read /proc/" << pid << "/stat\n";
        return 1;
    }
    read_system_cpu_snap(sys_prev);

    std::vector<TimedSample> samples;

    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

        ProcessCpuSnap cpu_curr;
        SystemCpuSnap  sys_curr;
        if (!read_process_cpu_snap(pid, cpu_curr)) {
            std::cerr << "\n[CPUTrace] Process exited early.\n";
            break;
        }
        read_system_cpu_snap(sys_curr);

        double pct = compute_cpu_percent(cpu_prev, cpu_curr, sys_prev, sys_curr);
        ProcStatus ps = read_proc_status(pid);

        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - wall_start).count();

        TimedSample s;
        s.timestamp_sec = elapsed;
        s.cpu_percent   = (pct < 0.0) ? 0.0 : pct;
        s.vm_rss_kb     = ps.vm_rss_kb;
        s.threads       = ps.threads;
        samples.push_back(s);

        std::cerr << "\r[" << std::fixed << std::setprecision(1) << elapsed << "s] "
                  << "CPU: " << std::setprecision(1) << s.cpu_percent << "%  "
                  << "RSS: " << ps.vm_rss_kb << " kB    " << std::flush;

        cpu_prev = cpu_curr;
        sys_prev = sys_curr;
    }
    std::cerr << "\n";

    double actual_duration = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();

    double cpu_sum = 0.0, cpu_max = 0.0;
    for (auto& s : samples) {
        cpu_sum += s.cpu_percent;
        if (s.cpu_percent > cpu_max) cpu_max = s.cpu_percent;
    }
    double cpu_avg = samples.empty() ? 0.0 : cpu_sum / samples.size();

    SymbolResolver resolver(pid);

    // write JSON
    JsonWriter jw(out_path);
    jw.begin_object();
    jw.key("session_id");       jw.str(session_id);
    jw.key("start_time");       jw.str(start_time);
    jw.key("duration_seconds"); jw.num((long long)actual_duration);

    jw.key("metadata");
    jw.out << "{\n"
           << "    \"pid\": "             << pid                  << ",\n"
           << "    \"process_name\": \""  << ps_initial.name      << "\",\n"
           << "    \"threads\": "         << ps_initial.threads   << ",\n"
           << "    \"vm_rss_kb\": "       << ps_initial.vm_rss_kb << "\n"
           << "  }";
    jw.first = false;

    jw.key("summary");
    jw.out << std::fixed << std::setprecision(2)
           << "{\n"
           << "    \"cpu_avg_percent\": " << cpu_avg         << ",\n"
           << "    \"cpu_max_percent\": " << cpu_max         << ",\n"
           << "    \"sample_count\": "    << samples.size()  << "\n"
           << "  }";

    jw.key("samples");
    jw.out << "[\n";
    for (size_t i = 0; i < samples.size(); i++) {
        jw.out << "    {\"t\": " << std::setprecision(3) << samples[i].timestamp_sec
               << ", \"cpu_pct\": " << std::setprecision(2) << samples[i].cpu_percent
               << ", \"rss_kb\": "  << samples[i].vm_rss_kb
               << ", \"threads\": " << samples[i].threads << "}";
        if (i + 1 < samples.size()) jw.out << ",";
        jw.out << "\n";
    }
    jw.out << "  ]";

    jw.key("symbol_samples");
    jw.out << "[\n";
    for (size_t i = 0; i < perf.samples.size(); i++) {
        std::string func = resolver.resolve(perf.samples[i].ip);
        if (func == "??") func = "unknown";
        jw.out << "    {\"ip\": " << perf.samples[i].ip
               << ", \"func\": \"" << func << "\"}";
        if (i + 1 < perf.samples.size()) jw.out << ",";
        jw.out << "\n";
    }
    jw.out << "  ]";

    jw.end_object();

    std::cout << "[CPUTrace] Wrote " << samples.size()
              << " samples to " << out_path << "\n"
              << "[CPUTrace] avg CPU: " << cpu_avg
              << "%  max: " << cpu_max << "%\n";
    return 0;
}
