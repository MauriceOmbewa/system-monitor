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

// Get process name from /proc/[pid]/comm
string getProcessName(int pid) {
    string comm_path = "/proc/" + to_string(pid) + "/comm";
    ifstream comm_file(comm_path);
    if (!comm_file.is_open()) {
        return "";
    }
    
    string name;
    getline(comm_file, name);
    return name;
}

// Get detailed information about a process
Process getProcessInfo(int pid) {
    Process proc;
    proc.pid = pid;
    
    // Get process name
    proc.name = getProcessName(pid);
    if (proc.name.empty()) {
        // Try to get name from stat file if comm file failed
        string stat_path = "/proc/" + to_string(pid) + "/stat";
        ifstream stat_file(stat_path);
        if (stat_file.is_open()) {
            string line;
            getline(stat_file, line);
            
            // Extract name from between parentheses
            size_t open_paren = line.find('(');
            size_t close_paren = line.rfind(')');
            if (open_paren != string::npos && close_paren != string::npos && open_paren < close_paren) {
                proc.name = line.substr(open_paren + 1, close_paren - open_paren - 1);
            }
        }
    }
    
    // Get process state and other info from /proc/[pid]/stat
    string stat_path = "/proc/" + to_string(pid) + "/stat";
    ifstream stat_file(stat_path);
    if (!stat_file.is_open()) {
        return proc;
    }
    
    string line;
    getline(stat_file, line);
    
    // Find the closing parenthesis for the process name
    size_t pos = line.rfind(')');
    if (pos == string::npos) {
        return proc;
    }
    
    // Skip the process name and extract the remaining fields
    string remaining = line.substr(pos + 2); // +2 to skip ') '
    
    // Parse the stat file fields
    char state;
    int ppid, pgrp, session, tty_nr, tpgid, exit_signal, processor;
    unsigned int flags, rt_priority, policy;
    unsigned long minflt, cminflt, majflt, cmajflt, utime, stime, vsize, rss;
    long cutime, cstime, priority, nice, num_threads, itrealvalue, starttime;
    
    sscanf(remaining.c_str(), "%c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %lu %lu",
           &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt, &cminflt, &majflt, &cmajflt,
           &utime, &stime, &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue, &starttime, &vsize);
    
    proc.state = state;
    proc.ppid = ppid;
    proc.priority = priority;
    proc.utime = utime;
    proc.stime = stime;
    proc.vsize = vsize;
    
    // Get RSS (Resident Set Size)
    string status_path = "/proc/" + to_string(pid) + "/status";
    ifstream status_file(status_path);
    if (status_file.is_open()) {
        string status_line;
        while (getline(status_file, status_line)) {
            if (status_line.find("VmRSS:") == 0) {
                // Extract the RSS value in kB
                size_t colon_pos = status_line.find(':');
                if (colon_pos != string::npos) {
                    string rss_str = status_line.substr(colon_pos + 1);
                    // Remove 'kB' and whitespace
                    rss_str.erase(remove_if(rss_str.begin(), rss_str.end(), ::isalpha), rss_str.end());
                    rss_str.erase(remove_if(rss_str.begin(), rss_str.end(), ::isspace), rss_str.end());
                    proc.rss = stoll(rss_str) * 1024; // Convert kB to bytes
                }
                break;
            }
        }
    }
    
    // Calculate memory usage percentage
    MemoryInfo mem_info = getMemoryInfo();
    if (mem_info.total_ram > 0) {
        proc.memory_usage = (float)proc.rss * 100.0f / mem_info.total_ram;
    } else {
        proc.memory_usage = 0.0f;
    }
    
    return proc;
}

// Get all processes
vector<Process> getAllProcesses() {
    vector<Process> processes;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return processes;
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if the directory name is a number (PID)
        if (isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            Process proc = getProcessInfo(pid);
            if (!proc.name.empty()) { // Only add valid processes
                processes.push_back(proc);
            }
        }
    }
    
    closedir(proc_dir);
    return processes;
}

// Update CPU usage for all processes
void updateProcessCpuUsage(vector<Process>& processes) {
    static map<int, pair<long long, long long>> prev_cpu_times; // pid -> (utime, stime)
    static long prev_total_time = 0;
    
    // Get current system uptime
    long uptime = getSystemUptime();
    if (uptime == 0) return;
    
    // Calculate total CPU time
    long total_time = uptime * sysconf(_SC_CLK_TCK);
    long total_time_diff = total_time - prev_total_time;
    if (total_time_diff <= 0) return;
    
    // Update CPU usage for each process
    for (auto& proc : processes) {
        long long curr_proc_time = proc.utime + proc.stime;
        
        // Check if we have previous data for this process
        if (prev_cpu_times.find(proc.pid) != prev_cpu_times.end()) {
            auto& prev_times = prev_cpu_times[proc.pid];
            long long prev_proc_time = prev_times.first + prev_times.second;
            long long proc_time_diff = curr_proc_time - prev_proc_time;
            
            // Calculate CPU usage percentage
            if (proc_time_diff >= 0) {
                proc.cpu_usage = (float)proc_time_diff * 100.0f / total_time_diff;
            }
        }
        
        // Store current times for next update
        prev_cpu_times[proc.pid] = make_pair(proc.utime, proc.stime);
    }
    
    prev_total_time = total_time;
}

// Kill a process by PID
bool killProcess(int pid) {
    if (pid <= 0) return false;
    return (kill(pid, SIGTERM) == 0);
}

// Build a process tree (parent -> children mapping)
map<int, vector<int>> buildProcessTree() {
    map<int, vector<int>> tree;
    vector<Process> processes = getAllProcesses();
    
    // Build the tree
    for (const auto& proc : processes) {
        if (proc.ppid > 0) {
            tree[proc.ppid].push_back(proc.pid);
        }
    }
    
    return tree;
}

// Get all child processes of a given PID
vector<Process> getProcessChildren(int pid) {
    vector<Process> children;
    map<int, vector<int>> tree = buildProcessTree();
    
    // If this process has no children, return empty vector
    if (tree.find(pid) == tree.end()) {
        return children;
    }
    
    // Get all processes
    vector<Process> all_processes = getAllProcesses();
    map<int, Process> pid_to_process;
    for (const auto& proc : all_processes) {
        pid_to_process[proc.pid] = proc;
    }
    
    // Add direct children
    for (int child_pid : tree[pid]) {
        if (pid_to_process.find(child_pid) != pid_to_process.end()) {
            children.push_back(pid_to_process[child_pid]);
        }
    }
    
    return children;
}
