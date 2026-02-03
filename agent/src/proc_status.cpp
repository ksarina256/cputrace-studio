#include "proc_status.h"
#include <fstream>
#include <sstream>

ProcStatus read_proc_status(int pid) {
  ProcStatus ps;
  std::ifstream file("/proc/" + std::to_string(pid) + "/status");
  std::string line;

  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string key;
    iss >> key;

    if (key == "Name:") iss >> ps.name;
    else if (key == "VmRSS:") iss >> ps.vm_rss_kb;
    else if (key == "VmSize:") iss >> ps.vm_size_kb;
    else if (key == "Threads:") iss >> ps.threads;
  }

  return ps;
}
