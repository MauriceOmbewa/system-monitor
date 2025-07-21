#include "header.h"

// Get memory information from /proc/meminfo
MemoryInfo getMemoryInfo() {
    MemoryInfo info = {0};
    struct sysinfo sys_info;
    
    if (sysinfo(&sys_info) != 0) {
        return info; // Return zeros on error
    }
    
    // RAM info (convert to bytes)
    info.total_ram = sys_info.totalram * sys_info.mem_unit;
    info.free_ram = sys_info.freeram * sys_info.mem_unit;
    info.used_ram = info.total_ram - info.free_ram;
    
    // SWAP info (convert to bytes)
    info.total_swap = sys_info.totalswap * sys_info.mem_unit;
    info.free_swap = sys_info.freeswap * sys_info.mem_unit;
    info.used_swap = info.total_swap - info.free_swap;
    
    return info;
}
