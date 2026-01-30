#pragma once
#include <fstream>
#include <string>

struct JsonWriter {
  std::ofstream out;
  bool first = true;

  explicit JsonWriter(const std::string& path) : out(path) {}

  void begin_object() { out << "{\n"; }
  void end_object() { out << "\n}\n"; }

  void key(const std::string& k) {
    if (!first) out << ",\n";
    first = false;
    out << "  \"" << k << "\": ";
  }

  void str(const std::string& v) { out << "\"" << v << "\""; }
  void num(long long v) { out << v; }
};
