#pragma once
#include <fstream>
#include <string>
#include <iomanip>

struct JsonWriter {
    std::ofstream out;
    bool first = true;

    explicit JsonWriter(const std::string& path) : out(path) {
        out << std::fixed << std::setprecision(2);
    }

    bool is_open() const { return out.is_open(); }

    void begin_object() { out << "{\n"; }
    void end_object()   { out << "\n}\n"; }

    void key(const std::string& k) {
        if (!first) out << ",\n";
        first = false;
        out << "  \"" << k << "\": ";
    }

    void str(const std::string& v) { out << "\"" << escape(v) << "\""; }
    void num(long long v)          { out << v; }
    void dbl(double v)             { out << v; }

private:
    static std::string escape(const std::string& s) {
        std::string r;
        for (char c : s) {
            if      (c == '"')  r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else if (c == '\r') r += "\\r";
            else if (c == '\t') r += "\\t";
            else                r += c;
        }
        return r;
    }
};
