#include <iostream>
#include <string>

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

  std::cout << "Target PID: " << pid_str
            << " | duration: " << duration_str
            << "s | out: " << out_path << "\n";

  // Week 2: implement sampling + JSON output
  return 0;
}
EOF
