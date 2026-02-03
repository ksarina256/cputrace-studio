#pragma once
#include <string>
#include <vector>
#include <cstdint> 

struct MapRegion {
    uint64_t start;
    uint64_t end;
    std::string path;
};

class SymbolResolver {
public:
    explicit SymbolResolver(int pid);
    std::string resolve(uint64_t addr);

private:
    int pid;
    std::vector<MapRegion> regions;
    void load_maps();
};