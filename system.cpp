#include "header.h"

// Get CPU usage percentage
float getCPUUsage() {
    static CPUStats prev_stats = {0};
    CPUStats curr_stats = {0};
    
    ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) return 0.0f;
    
    string line;
    getline(stat_file, line);
    
    sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
           &curr_stats.user, &curr_stats.nice, &curr_stats.system, &curr_stats.idle,
           &curr_stats.iowait, &curr_stats.irq, &curr_stats.softirq, &curr_stats.steal,
           &curr_stats.guest, &curr_stats.guestNice);
    
    long long prev_idle = prev_stats.idle + prev_stats.iowait;
    long long curr_idle = curr_stats.idle + curr_stats.iowait;
    
    long long prev_total = prev_stats.user + prev_stats.nice + prev_stats.system + prev_idle;
    long long curr_total = curr_stats.user + curr_stats.nice + curr_stats.system + curr_idle;
    
    long long total_diff = curr_total - prev_total;
    long long idle_diff = curr_idle - prev_idle;
    
    prev_stats = curr_stats;
    
    if (total_diff == 0) return 0.0f;
    return (float)(total_diff - idle_diff) * 100.0f / total_diff;
}

// Get system uptime in seconds
long getSystemUptime() {
    ifstream uptime_file("/proc/uptime");
    if (!uptime_file.is_open()) return 0;
    
    double uptime;
    uptime_file >> uptime;
    return (long)uptime;
}

// Get system load averages
vector<float> getLoadAverage() {
    vector<float> loads(3, 0.0f);
    ifstream loadavg_file("/proc/loadavg");
    if (!loadavg_file.is_open()) return loads;
    
    loadavg_file >> loads[0] >> loads[1] >> loads[2];
    return loads;
}

// Get current logged in user
string getUsername() {
    char* username = getlogin();
    return username ? string(username) : "unknown";
}

// Get computer hostname
string getHostname() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return string(hostname);
}

// Get process counts from /proc/stat
map<string, int> getProcessCounts() {
    map<string, int> counts;
    counts["running"] = 0;
    counts["sleeping"] = 0;
    counts["stopped"] = 0;
    counts["zombie"] = 0;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return counts;
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (isdigit(entry->d_name[0])) {
            string stat_path = "/proc/" + string(entry->d_name) + "/stat";
            ifstream stat_file(stat_path);
            if (stat_file.is_open()) {
                string line;
                getline(stat_file, line);
                if (line.length() > 0) {
                    // State is the 3rd field after pid and comm
                    size_t pos = line.find(") ");
                    if (pos != string::npos && pos + 2 < line.length()) {
                        char state = line[pos + 2];
                        switch (state) {
                            case 'R': counts["running"]++; break;
                            case 'S': counts["sleeping"]++; break;
                            case 'T': counts["stopped"]++; break;
                            case 'Z': counts["zombie"]++; break;
                        }
                    }
                }
            }
        }
    }
    closedir(proc_dir);
    return counts;
}

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}
