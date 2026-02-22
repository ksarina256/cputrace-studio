#pragma once
#include <string>

struct ProcStatus {
  std::string name;
  long vm_rss_kb = 0;
  long vm_size_kb = 0;
  int threads = 0;
};

ProcStatus read_proc_status(int pid);
