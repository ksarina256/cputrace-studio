#include <iostream>
#include <string>
#include <cstdlib>

#include "json_writer.h"
#include "proc_status.h"
#include "perf_counter.h"
#include "symbol_resolver.h"

static void print_usage() {
  std::cout <<
    "CPUTrace (Week 2 MVP)\n"
    "Usage:\n"
    "  cputrace --pid <pid> --duration <seconds> --out <file.json>\n"
    "Options:\n"
    "  --pid       Target process id\n"
    "  --duration  Seconds to sample\n"
    "  --out       Output JSON path\n";
}

static bool read_arg(int argc, char** argv, const std::string& key, std::string& out) {
  for (int i = 1; i < argc - 1; i++) {
    if (argv[i] == key) {
      out = argv[i + 1];
      return true;
    }
  }
  return false;
}

int main(int argc, char** argv) {
  std::string pid_str, duration_str, out_path;

  if (!read_arg(argc, argv, "--pid", pid_str) ||
      !read_arg(argc, argv, "--duration", duration_str) ||
      !read_arg(argc, argv, "--out", out_path)) {
    print_usage();
    return 2;
  }

  long long pid = std::stoll(pid_str);
  long long duration = std::stoll(duration_str);

  // gather process metadata
  ProcStatus ps = read_proc_status(static_cast<int>(pid));
  
  auto perf = sample_cpu_for_pid(static_cast<int>(pid),
                                 static_cast<int>(duration * 1000), 
                                 1000); // 1000Hz sampling

  if (!perf.ok) {
    std::cerr << "perf counter failed (errno=" << perf.err
              << "). Try a PID you own or run with sudo.\n";
  }

  SymbolResolver resolver(static_cast<int>(pid));

  JsonWriter jw(out_path);
  jw.begin_object();

  jw.key("pid"); jw.num(pid);
  jw.key("duration_seconds"); jw.num(duration);
  jw.key("process_name"); jw.str(ps.name);
  jw.key("threads"); jw.num(ps.threads);
  jw.key("vm_rss_kb"); jw.num(ps.vm_rss_kb);
  jw.key("vm_size_kb"); jw.num(ps.vm_size_kb);

  jw.key("total_cycles"); jw.num(perf.ok ? perf.total_cycles : -1);

  jw.key("samples");
  jw.out << "[\n";
  for (size_t i = 0; i < perf.samples.size(); ++i) {
     std::string func_name = resolver.resolve(perf.samples[i].ip);
     if (func_name == "??") func_name = "unknown";
     
     jw.out << "    {\"ip\": " << perf.samples[i].ip 
            << ", \"func\": \"" << func_name << "\"}";
     
     if (i < perf.samples.size() - 1) jw.out << ",";
     jw.out << "\n";
  }
  jw.out << "  ]";

  jw.end_object();

  std::cout << "Wrote " << out_path << "\n";
  return 0;
}