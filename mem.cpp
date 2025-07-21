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

// Get disk usage information
DiskInfo getDiskInfo(const string& path) {
    DiskInfo info = {0};
    struct statvfs disk_info;
    
    if (statvfs(path.c_str(), &disk_info) != 0) {
        return info; // Return zeros on error
    }
    
    info.mount_point = path;
    info.total_space = disk_info.f_blocks * disk_info.f_frsize;
    info.free_space = disk_info.f_bfree * disk_info.f_frsize;
    info.used_space = info.total_space - info.free_space;
    
    return info;
}

// Get information for all mounted disks
vector<DiskInfo> getAllDisks() {
    vector<DiskInfo> disks;
    
    // Common mount points to check
    vector<string> mount_points = {"/", "/home", "/boot", "/usr", "/var"};
    
    for (const auto& mount : mount_points) {
        DiskInfo info = getDiskInfo(mount);
        if (info.total_space > 0) { // Only add valid disks
            disks.push_back(info);
        }
    }
    
    return disks;
}

// Format size in bytes to human-readable format (KB, MB, GB)
string formatSize(unsigned long size_in_bytes) {
    float size = size_in_bytes;
    string units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    
    while (size >= 1024 && unit_index < 4) {
        size /= 1024;
        unit_index++;
    }
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit_index].c_str());
    return string(buffer);
}

// Get RAM usage percentage
float getMemoryUsagePercentage() {
    MemoryInfo info = getMemoryInfo();
    if (info.total_ram == 0) return 0.0f;
    return (float)info.used_ram * 100.0f / info.total_ram;
}

// Get SWAP usage percentage
float getSwapUsagePercentage() {
    MemoryInfo info = getMemoryInfo();
    if (info.total_swap == 0) return 0.0f;
    return (float)info.used_swap * 100.0f / info.total_swap;
}

// Get disk usage percentage
float getDiskUsagePercentage(const string& path) {
    DiskInfo info = getDiskInfo(path);
    if (info.total_space == 0) return 0.0f;
    return (float)info.used_space * 100.0f / info.total_space;
}
