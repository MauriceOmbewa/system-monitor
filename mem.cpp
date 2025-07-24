#include "header.h"

// Get memory information from /proc/meminfo
MemoryInfo getMemoryInfo() {
    MemoryInfo info = {0};
    
    ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return info;
    }
    
    string line;
    unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
    unsigned long buffers = 0, cached = 0, swap_total = 0, swap_free = 0;
    
    while (getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lu kB", &mem_total);
        } else if (line.find("MemFree:") == 0) {
            sscanf(line.c_str(), "MemFree: %lu kB", &mem_free);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lu kB", &mem_available);
        } else if (line.find("Buffers:") == 0) {
            sscanf(line.c_str(), "Buffers: %lu kB", &buffers);
        } else if (line.find("Cached:") == 0) {
            sscanf(line.c_str(), "Cached: %lu kB", &cached);
        } else if (line.find("SwapTotal:") == 0) {
            sscanf(line.c_str(), "SwapTotal: %lu kB", &swap_total);
        } else if (line.find("SwapFree:") == 0) {
            sscanf(line.c_str(), "SwapFree: %lu kB", &swap_free);
        }
    }
    
    // Convert kB to bytes
    info.total_ram = mem_total * 1024;
    info.free_ram = mem_free * 1024;  // Use actual free (not available)
    // Calculate used memory like 'free' command: total - free - buffers - cached
    info.used_ram = (mem_total - mem_free - buffers - cached) * 1024;
    
    info.total_swap = swap_total * 1024;
    info.free_swap = swap_free * 1024;
    info.used_swap = (swap_total - swap_free) * 1024;
    
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
    info.free_space = disk_info.f_bavail * disk_info.f_frsize;  // Available to users
    info.used_space = (disk_info.f_blocks - disk_info.f_bfree) * disk_info.f_frsize;  // Match df calculation
    
    return info;
}

// Get information for all mounted disks
vector<DiskInfo> getAllDisks() {
    vector<DiskInfo> disks;
    set<string> seen_devices; // To avoid duplicates
    
    ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) {
        // Fallback to root filesystem only
        DiskInfo root_info = getDiskInfo("/");
        if (root_info.total_space > 0) {
            disks.push_back(root_info);
        }
        return disks;
    }
    
    string line;
    while (getline(mounts, line)) {
        stringstream ss(line);
        string device, mount_point, fs_type;
        ss >> device >> mount_point >> fs_type;
        
        // Skip virtual filesystems and duplicates
        if (device.find("/dev/") == 0 && 
            fs_type != "tmpfs" && fs_type != "devtmpfs" && 
            fs_type != "sysfs" && fs_type != "proc" &&
            seen_devices.find(device) == seen_devices.end()) {
            
            DiskInfo info = getDiskInfo(mount_point);
            if (info.total_space > 0) {
                info.mount_point = mount_point + " (" + device + ")"; // Show device name
                disks.push_back(info);
                seen_devices.insert(device);
            }
        }
    }
    
    // If no real disks found, add root filesystem
    if (disks.empty()) {
        DiskInfo root_info = getDiskInfo("/");
        if (root_info.total_space > 0) {
            root_info.mount_point = "/";
            disks.push_back(root_info);
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

// Get process priority (nice value)
int getProcessPriority(int pid) {
    // Try to read from /proc/[pid]/stat
    string stat_path = "/proc/" + to_string(pid) + "/stat";
    ifstream stat_file(stat_path);
    if (!stat_file.is_open()) {
        return 0;
    }
    
    string line;
    getline(stat_file, line);
    
    // Find the closing parenthesis for the process name
    size_t pos = line.rfind(')');
    if (pos == string::npos) {
        return 0;
    }
    
    // Skip the process name and extract the remaining fields
    string remaining = line.substr(pos + 2); // +2 to skip ') '
    
    // Parse the stat file fields (nice value is the 19th field)
    vector<string> fields;
    stringstream ss(remaining);
    string field;
    while (ss >> field) {
        fields.push_back(field);
    }
    
    // Nice value is the 18th field (0-indexed)
    if (fields.size() > 18) {
        return stoi(fields[18]);
    }
    
    return 0;
}

// Set process priority (nice value)
bool setProcessPriority(int pid, int priority) {
    // Ensure priority is in valid range (-20 to 19)
    priority = max(-20, min(priority, 19));
    
    // Use setpriority system call
    if (setpriority(PRIO_PROCESS, pid, priority) == 0) {
        return true;
    }
    
    return false;
}
