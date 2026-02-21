#include "symbol_resolver.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdint>

SymbolResolver::SymbolResolver(int pid) : pid(pid) { load_maps(); }

void SymbolResolver::load_maps() {
    std::ifstream maps_file("/proc/" + std::to_string(pid) + "/maps");
    std::string line;
    while (std::getline(maps_file, line)) {
        MapRegion region;
        uint64_t offset;
        char perms[5], dev[10], path_buf[4096] = {};
        int inode;
        if (sscanf(line.c_str(), "%lx-%lx %4s %lx %s %d %4095s",
                   &region.start, &region.end, perms, &offset,
                   dev, &inode, path_buf) >= 6) {
            if (perms[2] == 'x' && path_buf[0] != '\0') {
                region.path = path_buf;
                regions.push_back(region);
            }
        }
    }
}

std::string SymbolResolver::resolve(uint64_t addr) {
    for (const auto& region : regions) {
        if (addr >= region.start && addr < region.end) {
            char command[2048];
            uint64_t relative_addr = addr - region.start;
            snprintf(command, sizeof(command),
                     "addr2line -e \"%s\" -f %lx 2>/dev/null | head -n 1",
                     region.path.c_str(), relative_addr);
            FILE* pipe = popen(command, "r");
            if (!pipe) return "??";
            char buffer[512];
            std::string sym = "??";
            if (fgets(buffer, sizeof(buffer), pipe)) {
                sym = buffer;
                if (!sym.empty() && sym.back() == '\n') sym.pop_back();
            }
            pclose(pipe);
            return sym;
        }
    }
    return "??";
}
