// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
// lib to read from file
#include <fstream>
// for the name of the computer and the logged in user
#ifdef _WIN32
#include <windows.h>
#include <lmcons.h>  // For UNLEN
#else
#include <unistd.h>
#include <limits.h>
#endif
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Process
{
    int pid;
    string name;
    char state;
    long long int vsize;      // Virtual memory size in bytes
    long long int rss;        // Resident set size in bytes
    long long int utime;      // User time
    long long int stime;      // System time
    float cpu_usage;          // CPU usage percentage
    float memory_usage;       // Memory usage percentage
    int ppid;                 // Parent process ID
    int priority;             // Process priority
    
    // Get state as string
    string getStateString() const {
        switch (state) {
            case 'R': return "Running";
            case 'S': return "Sleeping";
            case 'D': return "Disk Sleep";
            case 'T': return "Stopped";
            case 't': return "Tracing";
            case 'Z': return "Zombie";
            case 'X': return "Dead";
            default: return string(1, state);
        }
    }
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct FanInfo {
    bool status;
    int speed;
    int level;
};

struct Graph
{
    static const int MAX_VALUES = 100;
    float values[MAX_VALUES];
    int values_offset;
    bool paused;
    float fps;
    float scale;
    
    Graph() : values_offset(0), paused(false), fps(30.0f), scale(100.0f) {
        memset(values, 0, sizeof(values));
    }
    
    void addValue(float value) {
        if (!paused) {
            values[values_offset] = value;
            values_offset = (values_offset + 1) % MAX_VALUES;
        }
    }
};

typedef Graph CPUGraph;
typedef Graph FanGraph;
typedef Graph ThermalGraph;

struct Networks
{
    vector<IP4> ip4s;
};

struct TX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int frame;
    int compressed;
    int multicast;
};

struct RX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int colls;
    int carrier;
    int compressed;
};

// student TODO : system stats
string CPUinfo();
const char *getOsName();
string getUsername();
string getHostname();
map<string, int> getProcessCounts();
float getCPUUsage();
long getSystemUptime();
vector<float> getLoadAverage();
float getCPUTemperature();
void updateCPUGraph(CPUGraph& graph);
int getCPUCoreCount();
bool getFanStatus();
int getFanSpeed();
int getFanLevel();
FanInfo getFanInfo();
void updateFanGraph(FanGraph& graph);
void updateThermalGraph(ThermalGraph& graph);

// student TODO : memory and processes
struct MemoryInfo {
    unsigned long total_ram;
    unsigned long free_ram;
    unsigned long used_ram;
    unsigned long total_swap;
    unsigned long free_swap;
    unsigned long used_swap;
};

struct DiskInfo {
    unsigned long total_space;
    unsigned long free_space;
    unsigned long used_space;
    string mount_point;
};

MemoryInfo getMemoryInfo();
DiskInfo getDiskInfo(const string& path = "/");
vector<DiskInfo> getAllDisks();
string formatSize(unsigned long size_in_bytes);
float getMemoryUsagePercentage();
float getSwapUsagePercentage();
float getDiskUsagePercentage(const string& path = "/");

// Process related functions
Process getProcessInfo(int pid);
vector<Process> getAllProcesses();
void updateProcessCpuUsage(vector<Process>& processes);
string getProcessName(int pid);
bool killProcess(int pid);

// student TODO : network

#endif
