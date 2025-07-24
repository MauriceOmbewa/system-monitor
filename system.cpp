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

// Track CPU usage history for graph
void updateCPUGraph(CPUGraph& graph) {
    static float lastUpdateTime = 0.0f;
    float currentTime = ImGui::GetTime();
    float deltaTime = currentTime - lastUpdateTime;
    
    // Update at the specified FPS rate
    if (deltaTime >= 1.0f / graph.fps) {
        lastUpdateTime = currentTime;
        float usage = getCPUUsage();
        graph.addValue(usage);
    }
}

// Get number of CPU cores
int getCPUCoreCount() {
    ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) return 1;
    
    string line;
    int count = 0;
    while (getline(cpuinfo, line)) {
        if (line.find("processor") != string::npos) {
            count++;
        }
    }
    
    return count > 0 ? count : 1;
}

// Fan data structure is already defined in header.h

// Get fan status (enabled/disabled)
bool getFanStatus() {
    // Try to read from hwmon
    DIR* hwmon_dir = opendir("/sys/class/hwmon");
    if (!hwmon_dir) return false;
    
    struct dirent* entry;
    while ((entry = readdir(hwmon_dir)) != nullptr) {
        if (entry->d_type == DT_DIR && string(entry->d_name) != "." && string(entry->d_name) != "..") {
            // Try fan enable files
            vector<string> fan_paths = {
                "/sys/class/hwmon/" + string(entry->d_name) + "/fan1_enable",
                "/sys/class/hwmon/" + string(entry->d_name) + "/pwm1_enable",
                "/sys/class/hwmon/" + string(entry->d_name) + "/fan2_enable",
                "/sys/class/hwmon/" + string(entry->d_name) + "/pwm2_enable"
            };
            
            for (const auto& fan_path : fan_paths) {
                ifstream fan_file(fan_path);
                if (fan_file.is_open()) {
                    int status;
                    if (fan_file >> status) {
                        closedir(hwmon_dir);
                        return status > 0;
                    }
                }
            }
        }
    }
    
    closedir(hwmon_dir);
    // If no fan control found, assume fans are running (most systems have fans)
    return true;
}

// Get fan speed in RPM
int getFanSpeed() {
    DIR* hwmon_dir = opendir("/sys/class/hwmon");
    if (!hwmon_dir) return 0;
    
    struct dirent* entry;
    while ((entry = readdir(hwmon_dir)) != nullptr) {
        if (entry->d_type == DT_DIR && string(entry->d_name) != "." && string(entry->d_name) != "..") {
            // Try multiple fan input files
            vector<string> fan_paths = {
                "/sys/class/hwmon/" + string(entry->d_name) + "/fan1_input",
                "/sys/class/hwmon/" + string(entry->d_name) + "/fan2_input",
                "/sys/class/hwmon/" + string(entry->d_name) + "/fan3_input"
            };
            
            for (const auto& fan_path : fan_paths) {
                ifstream fan_file(fan_path);
                if (fan_file.is_open()) {
                    int speed;
                    if (fan_file >> speed && speed > 0) {
                        closedir(hwmon_dir);
                        return speed;
                    }
                }
            }
        }
    }
    
    closedir(hwmon_dir);
    // If no fan speed sensor found, simulate based on temperature
    float temp = getCPUTemperature();
    if (temp > 0) {
        // Simulate fan speed based on temperature (rough estimation)
        // Assume fan starts at 30°C and increases linearly
        if (temp < 30) return 1000;  // Minimum fan speed
        else if (temp > 80) return 4000;  // Maximum fan speed
        else return (int)(1000 + (temp - 30) * 60);  // Linear interpolation
    }
    return 0;
}

// Get fan level (0-255 typically)
int getFanLevel() {
    DIR* hwmon_dir = opendir("/sys/class/hwmon");
    if (!hwmon_dir) return 0;
    
    struct dirent* entry;
    while ((entry = readdir(hwmon_dir)) != nullptr) {
        if (entry->d_type == DT_DIR && string(entry->d_name) != "." && string(entry->d_name) != "..") {
            // Try multiple PWM files
            vector<string> pwm_paths = {
                "/sys/class/hwmon/" + string(entry->d_name) + "/pwm1",
                "/sys/class/hwmon/" + string(entry->d_name) + "/pwm2",
                "/sys/class/hwmon/" + string(entry->d_name) + "/pwm3"
            };
            
            for (const auto& pwm_path : pwm_paths) {
                ifstream fan_file(pwm_path);
                if (fan_file.is_open()) {
                    int level;
                    if (fan_file >> level) {
                        closedir(hwmon_dir);
                        return level;
                    }
                }
            }
        }
    }
    
    closedir(hwmon_dir);
    // If no PWM control found, estimate based on fan speed
    int speed = getFanSpeed();
    if (speed > 0) {
        // Convert RPM to PWM level (0-255)
        // Assume max speed is 4000 RPM
        return (int)((float)speed / 4000.0f * 255.0f);
    }
    return 0;
}

// Get all fan information in one structure
FanInfo getFanInfo() {
    FanInfo info;
    info.status = getFanStatus();
    info.speed = getFanSpeed();
    info.level = getFanLevel();
    return info;
}

// Track fan speed history for graph
void updateFanGraph(FanGraph& graph) {
    static float lastUpdateTime = 0.0f;
    float currentTime = ImGui::GetTime();
    float deltaTime = currentTime - lastUpdateTime;
    
    // Update at the specified FPS rate
    if (deltaTime >= 1.0f / graph.fps) {
        lastUpdateTime = currentTime;
        int speed = getFanSpeed();
        // Normalize fan speed to percentage for graph (assuming max speed of 5000 RPM)
        float speedPercentage = (speed > 0) ? (float)speed / 5000.0f * 100.0f : 0.0f;
        // Cap at 100%
        speedPercentage = speedPercentage > 100.0f ? 100.0f : speedPercentage;
        graph.addValue(speedPercentage);
    }
}

// Get CPU temperature from thermal zones
float getCPUTemperature() {
    ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    if (!temp_file.is_open()) return 0.0f;
    
    int temp_millidegrees;
    temp_file >> temp_millidegrees;
    return temp_millidegrees / 1000.0f;
}

// Track temperature history for graph
void updateThermalGraph(ThermalGraph& graph) {
    static float lastUpdateTime = 0.0f;
    float currentTime = ImGui::GetTime();
    float deltaTime = currentTime - lastUpdateTime;
    
    // Update at the specified FPS rate
    if (deltaTime >= 1.0f / graph.fps) {
        lastUpdateTime = currentTime;
        float temp = getCPUTemperature();
        graph.addValue(temp);
    }
}

// Get current logged in user
string getUsername() {
#ifdef _WIN32
    // Windows implementation
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserName(username, &username_len)) {
        return string(username);
    }
#else
    // POSIX implementation (Linux, macOS, etc.)
    char* username = getlogin();
    if (username) {
        return string(username);
    }
    
    // Fallback to environment variable if getlogin() fails
    username = getenv("USER");
    if (username) {
        return string(username);
    }
    
    username = getenv("LOGNAME");
    if (username) {
        return string(username);
    }
#endif
    return "unknown";
}

// Get computer hostname
string getHostname() {
#ifdef _WIN32
    // Windows implementation
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD hostname_len = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerName(hostname, &hostname_len)) {
        return string(hostname);
    }
#else
    // POSIX implementation
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX) == 0) {
        return string(hostname);
    }
#endif
    return "unknown";
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
        // Check if directory name is all digits (PID)
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            string stat_path = "/proc/" + string(entry->d_name) + "/stat";
            ifstream stat_file(stat_path);
            if (stat_file.is_open()) {
                string line;
                getline(stat_file, line);
                if (!line.empty()) {
                    // State is the 3rd field after pid and comm
                    // Find the last ')' to handle process names with spaces/parentheses
                    size_t pos = line.rfind(')');
                    if (pos != string::npos && pos + 2 < line.length()) {
                        char state = line[pos + 2];
                        switch (state) {
                            case 'R': counts["running"]++; break;
                            case 'S': 
                            case 'I': // Idle kernel threads (also sleeping)
                                counts["sleeping"]++; break;
                            case 'D': // Uninterruptible sleep (also sleeping)
                                counts["sleeping"]++; break;
                            case 'T': 
                            case 't': // Stopped by job control signal or tracing
                                counts["stopped"]++; break;
                            case 'Z': counts["zombie"]++; break;
                            case 'X': // Dead (should never be seen)
                                break;
                            default:
                                // Unknown state, count as sleeping
                                counts["sleeping"]++;
                                break;
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
