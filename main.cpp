#include "header.h"
#include <SDL.h>

// Global graph instances
CPUGraph g_cpuGraph;
FanGraph g_fanGraph;
ThermalGraph g_thermalGraph;
NetworkGraph g_rxGraph;
NetworkGraph g_txGraph;

// Process monitoring alerts
struct ProcessAlert {
    int pid;
    string name;
    float cpu_threshold;
    float memory_threshold;
    bool cpu_alert_active;
    bool memory_alert_active;
    
    ProcessAlert(int p, const string& n, float cpu, float mem)
        : pid(p), name(n), cpu_threshold(cpu), memory_threshold(mem),
          cpu_alert_active(false), memory_alert_active(false) {}
};

vector<ProcessAlert> g_process_alerts;

/*
NOTE : You are free to change the code as you wish, the main objective is to make the
       application work and pass the audit.

       It will be provided the main function with the following functions :

       - `void systemWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the system window on your screen
       - `void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the memory and processes window on your screen
       - `void networkWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the network window on your screen
*/

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h> // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h> // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h> // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h> // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE      // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h> // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE        // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h> // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// systemWindow, display information for the system monitorization
void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // System Information
    ImGui::Text("System Information");
    ImGui::Separator();
    
    ImGui::Text("OS: %s", getOsName());
    ImGui::Text("User: %s", getUsername().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("CPU: %s", CPUinfo().c_str());
    ImGui::Text("CPU Cores: %d", getCPUCoreCount());
    ImGui::Text("CPU Usage: %.1f%%", getCPUUsage());
    
    long uptime = getSystemUptime();
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int minutes = (uptime % 3600) / 60;
    ImGui::Text("Uptime: %dd %dh %dm", days, hours, minutes);
    
    vector<float> loads = getLoadAverage();
    ImGui::Text("Load: %.2f %.2f %.2f", loads[0], loads[1], loads[2]);
    ImGui::Text("CPU Temp: %.1f°C", getCPUTemperature());
    
    // Tabbed section for CPU, Fan, and Thermal
    ImGui::Spacing();
    ImGui::Separator();
    
    if (ImGui::BeginTabBar("SystemTabs")) {
        // CPU Tab
        if (ImGui::BeginTabItem("CPU")) {
            ImGui::Text("CPU Usage Graph");
            
            // Play/Pause button
            if (ImGui::Button(g_cpuGraph.paused ? "Play" : "Pause")) {
                g_cpuGraph.paused = !g_cpuGraph.paused;
            }
            
            // FPS slider
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::SliderFloat("FPS", &g_cpuGraph.fps, 1.0f, 60.0f, "%.1f");
            
            // Y-axis scale slider
            ImGui::SetNextItemWidth(120);
            ImGui::SliderFloat("Scale", &g_cpuGraph.scale, 10.0f, 200.0f, "%.1f");
            
            // Update CPU graph data
            updateCPUGraph(g_cpuGraph);
            
            // Plot the CPU usage graph
            ImGui::PlotLines("##cpuusage", g_cpuGraph.values, CPUGraph::MAX_VALUES, 
                            g_cpuGraph.values_offset, 
                            ("CPU Usage: " + to_string((int)g_cpuGraph.values[g_cpuGraph.values_offset == 0 ? CPUGraph::MAX_VALUES - 1 : g_cpuGraph.values_offset - 1]) + "%").c_str(), 
                            0.0f, g_cpuGraph.scale, ImVec2(ImGui::GetContentRegionAvail().x, 80));
            
            ImGui::EndTabItem();
        }
        
        // Fan Tab
        if (ImGui::BeginTabItem("Fan")) {
            ImGui::Text("Fan Information");
            
            // Get fan info
            FanInfo fanInfo = getFanInfo();
            
            // Display fan status
            ImGui::Text("Status: %s", fanInfo.status ? "Active" : "Inactive");
            ImGui::Text("Speed: %d RPM", fanInfo.speed);
            ImGui::Text("Level: %d", fanInfo.level);
            
            ImGui::Separator();
            ImGui::Text("Fan Speed Graph");
            
            // Play/Pause button
            if (ImGui::Button(g_fanGraph.paused ? "Play" : "Pause")) {
                g_fanGraph.paused = !g_fanGraph.paused;
            }
            
            // FPS slider
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::SliderFloat("FPS##fan", &g_fanGraph.fps, 1.0f, 60.0f, "%.1f");
            
            // Y-axis scale slider
            ImGui::SetNextItemWidth(120);
            ImGui::SliderFloat("Scale##fan", &g_fanGraph.scale, 10.0f, 200.0f, "%.1f");
            
            // Update fan graph data
            updateFanGraph(g_fanGraph);
            
            // Plot the fan speed graph
            ImGui::PlotLines("##fanspeed", g_fanGraph.values, FanGraph::MAX_VALUES, 
                            g_fanGraph.values_offset, 
                            ("Fan Speed: " + to_string((int)g_fanGraph.values[g_fanGraph.values_offset == 0 ? FanGraph::MAX_VALUES - 1 : g_fanGraph.values_offset - 1]) + "%").c_str(), 
                            0.0f, g_fanGraph.scale, ImVec2(ImGui::GetContentRegionAvail().x, 80));
            
            ImGui::EndTabItem();
        }
        
        // Thermal Tab
        if (ImGui::BeginTabItem("Thermal")) {
            ImGui::Text("Thermal Information");
            
            // Display current temperature
            float currentTemp = getCPUTemperature();
            ImGui::Text("Current Temperature: %.1f°C", currentTemp);
            
            ImGui::Separator();
            ImGui::Text("Temperature Graph");
            
            // Play/Pause button
            if (ImGui::Button(g_thermalGraph.paused ? "Play" : "Pause")) {
                g_thermalGraph.paused = !g_thermalGraph.paused;
            }
            
            // FPS slider
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::SliderFloat("FPS##thermal", &g_thermalGraph.fps, 1.0f, 60.0f, "%.1f");
            
            // Y-axis scale slider
            ImGui::SetNextItemWidth(120);
            ImGui::SliderFloat("Scale##thermal", &g_thermalGraph.scale, 10.0f, 100.0f, "%.1f");
            
            // Update thermal graph data
            updateThermalGraph(g_thermalGraph);
            
            // Plot the temperature graph
            ImGui::PlotLines("##tempgraph", g_thermalGraph.values, ThermalGraph::MAX_VALUES, 
                            g_thermalGraph.values_offset, 
                            ("Temperature: " + to_string((int)g_thermalGraph.values[g_thermalGraph.values_offset == 0 ? ThermalGraph::MAX_VALUES - 1 : g_thermalGraph.values_offset - 1]) + "°C").c_str(), 
                            0.0f, g_thermalGraph.scale, ImVec2(ImGui::GetContentRegionAvail().x, 80));
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::Spacing();
    ImGui::Text("Process Information");
    ImGui::Separator();
    
    map<string, int> processes = getProcessCounts();
    ImGui::Text("Running: %d", processes["running"]);
    ImGui::Text("Sleeping: %d", processes["sleeping"]);
    ImGui::Text("Stopped: %d", processes["stopped"]);
    ImGui::Text("Zombie: %d", processes["zombie"]);
    
    int total = processes["running"] + processes["sleeping"] + processes["stopped"] + processes["zombie"];
    ImGui::Text("Total: %d", total);

    ImGui::End();
}

// memoryProcessesWindow, display information for the memory and processes information
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Memory Information Section
    ImGui::Text("Memory Information");
    ImGui::Separator();
    
    // Get memory info
    MemoryInfo mem_info = getMemoryInfo();
    
    // RAM usage
    float ram_percentage = getMemoryUsagePercentage();
    ImGui::Text("RAM Usage: %.1f%%", ram_percentage);
    ImGui::Text("Total: %s", formatSize(mem_info.total_ram).c_str());
    ImGui::Text("Used: %s", formatSize(mem_info.used_ram).c_str());
    ImGui::Text("Free: %s", formatSize(mem_info.free_ram).c_str());
    
    // RAM progress bar
    ImGui::ProgressBar(ram_percentage / 100.0f, ImVec2(-1, 0), 
                       (formatSize(mem_info.used_ram) + " / " + formatSize(mem_info.total_ram)).c_str());
    
    ImGui::Spacing();
    
    // SWAP usage
    float swap_percentage = getSwapUsagePercentage();
    ImGui::Text("SWAP Usage: %.1f%%", swap_percentage);
    ImGui::Text("Total: %s", formatSize(mem_info.total_swap).c_str());
    ImGui::Text("Used: %s", formatSize(mem_info.used_swap).c_str());
    ImGui::Text("Free: %s", formatSize(mem_info.free_swap).c_str());
    
    // SWAP progress bar
    ImGui::ProgressBar(swap_percentage / 100.0f, ImVec2(-1, 0), 
                       (formatSize(mem_info.used_swap) + " / " + formatSize(mem_info.total_swap)).c_str());
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Disk usage section
    ImGui::Text("Disk Usage");
    ImGui::Separator();
    
    // Get all disks
    vector<DiskInfo> disks = getAllDisks();
    
    for (const auto& disk : disks) {
        float disk_percentage = getDiskUsagePercentage(disk.mount_point);
        ImGui::Text("%s: %.1f%%", disk.mount_point.c_str(), disk_percentage);
        ImGui::Text("Total: %s", formatSize(disk.total_space).c_str());
        ImGui::Text("Used: %s", formatSize(disk.used_space).c_str());
        ImGui::Text("Free: %s", formatSize(disk.free_space).c_str());
        
        // Disk progress bar
        ImGui::ProgressBar(disk_percentage / 100.0f, ImVec2(-1, 0), 
                           (formatSize(disk.used_space) + " / " + formatSize(disk.total_space)).c_str());
        
        ImGui::Spacing();
    }
    
    // Process Table Section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Process Table");
    ImGui::Separator();
    
    // Display active alerts
    if (!g_process_alerts.empty()) {
        ImGui::Text("Active Alerts:");
        for (const auto& alert : g_process_alerts) {
            if (alert.cpu_alert_active || alert.memory_alert_active) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s (PID: %d): ", alert.name.c_str(), alert.pid);
                
                if (alert.cpu_alert_active) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "CPU > %.1f%% ", alert.cpu_threshold);
                }
                
                if (alert.memory_alert_active) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Memory > %.1f%%", alert.memory_threshold);
                }
            }
        }
        ImGui::Separator();
    }
    
    // Process view options
    static bool tree_view = false;
    
    // Process statistics summary
    map<string, int> proc_stats = getProcessCounts();
    int total_procs = proc_stats["running"] + proc_stats["sleeping"] + proc_stats["stopped"] + proc_stats["zombie"];
    ImGui::Text("Total: %d | Running: %d | Sleeping: %d | Stopped: %d | Zombie: %d", 
               total_procs, proc_stats["running"], proc_stats["sleeping"], 
               proc_stats["stopped"], proc_stats["zombie"]);
    
    // Process filter
    static char filter_text[128] = "";
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::InputText("##filter", filter_text, IM_ARRAYSIZE(filter_text));
    
    // Get all processes and update CPU usage
    static vector<Process> processes;
    static float last_update_time = 0.0f;
    float current_time = ImGui::GetTime();
    
    // Update process list every second
    if (current_time - last_update_time >= 1.0f) {
        processes = getAllProcesses();
        updateProcessCpuUsage(processes);
        last_update_time = current_time;
        
        // Check process alerts
        for (auto& alert : g_process_alerts) {
            bool found = false;
            for (const auto& proc : processes) {
                if (proc.pid == alert.pid) {
                    found = true;
                    
                    // Check CPU threshold
                    if (proc.cpu_usage > alert.cpu_threshold && !alert.cpu_alert_active) {
                        alert.cpu_alert_active = true;
                        // In a real application, you might show a notification or log this event
                    } else if (proc.cpu_usage <= alert.cpu_threshold) {
                        alert.cpu_alert_active = false;
                    }
                    
                    // Check memory threshold
                    if (proc.memory_usage > alert.memory_threshold && !alert.memory_alert_active) {
                        alert.memory_alert_active = true;
                        // In a real application, you might show a notification or log this event
                    } else if (proc.memory_usage <= alert.memory_threshold) {
                        alert.memory_alert_active = false;
                    }
                    
                    break;
                }
            }
            
            // Process no longer exists
            if (!found) {
                // In a real application, you might want to remove the alert or mark it as inactive
            }
        }
    }
    
    // Process table
    ImGui::BeginChild("ProcessTable", ImVec2(0, 300), true);
    
    // Sorting options
    static int sort_column = 0; // 0=PID, 1=Name, 2=State, 3=CPU%, 4=Memory%
    static bool sort_ascending = true;
    
    // Table headers with sorting
    ImGui::Columns(6, "ProcessTableColumns");
    
    // PID column header
    if (ImGui::Selectable("PID")) {
        if (sort_column == 0) sort_ascending = !sort_ascending;
        else { sort_column = 0; sort_ascending = true; }
    }
    ImGui::NextColumn();
    
    // Name column header
    if (ImGui::Selectable("Name")) {
        if (sort_column == 1) sort_ascending = !sort_ascending;
        else { sort_column = 1; sort_ascending = true; }
    }
    ImGui::NextColumn();
    
    // State column header
    if (ImGui::Selectable("State")) {
        if (sort_column == 2) sort_ascending = !sort_ascending;
        else { sort_column = 2; sort_ascending = true; }
    }
    ImGui::NextColumn();
    
    // Priority column header
    if (ImGui::Selectable("Priority")) {
        if (sort_column == 3) sort_ascending = !sort_ascending;
        else { sort_column = 3; sort_ascending = true; }
    }
    ImGui::NextColumn();
    
    // CPU% column header
    if (ImGui::Selectable("CPU%")) {
        if (sort_column == 4) sort_ascending = !sort_ascending;
        else { sort_column = 4; sort_ascending = true; }
    }
    ImGui::NextColumn();
    
    // Memory% column header
    if (ImGui::Selectable("Memory%")) {
        if (sort_column == 5) sort_ascending = !sort_ascending;
        else { sort_column = 5; sort_ascending = true; }
    }
    ImGui::NextColumn();
    
    ImGui::Separator();
    
    // Sort processes based on selected column
    vector<Process> sorted_processes = processes;
    sort(sorted_processes.begin(), sorted_processes.end(), 
        [sort_column, sort_ascending](const Process& a, const Process& b) {
            switch (sort_column) {
                case 0: // PID
                    return sort_ascending ? (a.pid < b.pid) : (a.pid > b.pid);
                case 1: // Name
                    return sort_ascending ? (a.name < b.name) : (a.name > b.name);
                case 2: // State
                    return sort_ascending ? (a.state < b.state) : (a.state > b.state);
                case 3: // Priority
                    return sort_ascending ? (a.priority < b.priority) : (a.priority > b.priority);
                case 4: // CPU%
                    return sort_ascending ? (a.cpu_usage < b.cpu_usage) : (a.cpu_usage > b.cpu_usage);
                case 5: // Memory%
                    return sort_ascending ? (a.memory_usage < b.memory_usage) : (a.memory_usage > b.memory_usage);
                default:
                    return false;
            }
        });
    
    // Store selected PIDs
    static set<int> selected_pids;
    
    // Table rows
    string filter(filter_text);
    
    if (tree_view) {
        // Tree view - show processes in a hierarchical structure
        map<int, vector<int>> process_tree = buildProcessTree();
        map<int, Process> pid_to_process;
        
        // Create a map for quick process lookup
        for (const auto& proc : sorted_processes) {
            pid_to_process[proc.pid] = proc;
        }
        
        // Find root processes (ppid = 0 or 1)
        vector<int> root_pids;
        for (const auto& proc : sorted_processes) {
            if (proc.ppid <= 1) {
                root_pids.push_back(proc.pid);
            }
        }
        
        // Helper function to recursively display process tree
        function<void(int, int)> displayProcessTree = [&](int pid, int depth) {
            // Skip if process not found
            if (pid_to_process.find(pid) == pid_to_process.end()) return;
            
            const Process& proc = pid_to_process[pid];
            
            // Apply filter if any
            bool show_process = true;
            if (!filter.empty()) {
                show_process = (proc.name.find(filter) != string::npos || 
                               to_string(proc.pid).find(filter) != string::npos);
            }
            
            if (show_process) {
                // Indent based on depth
                for (int i = 0; i < depth; i++) {
                    ImGui::Text("  "); ImGui::SameLine();
                }
                
                // Check if this process is selected
                bool is_selected = selected_pids.find(proc.pid) != selected_pids.end();
                
                // Allow row selection
                char row_label[32];
                sprintf(row_label, "%d##%d", proc.pid, proc.pid);
                
                // Handle multi-selection with Ctrl key
                ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns;
                if (ImGui::Selectable(row_label, is_selected, flags)) {
                    if (ImGui::GetIO().KeyCtrl) {
                        // Toggle selection with Ctrl
                        if (is_selected) selected_pids.erase(proc.pid);
                        else selected_pids.insert(proc.pid);
                    } else {
                        // Single selection without Ctrl
                        selected_pids.clear();
                        selected_pids.insert(proc.pid);
                    }
                }
                
                ImGui::NextColumn();
                ImGui::Text("%s", proc.name.c_str()); ImGui::NextColumn();
                ImGui::Text("%s", proc.getStateString().c_str()); ImGui::NextColumn();
                ImGui::Text("%d", proc.priority); ImGui::NextColumn();
                ImGui::Text("%.1f", proc.cpu_usage); ImGui::NextColumn();
                ImGui::Text("%.1f", proc.memory_usage); ImGui::NextColumn();
            }
            
            // Recursively display children
            if (process_tree.find(pid) != process_tree.end()) {
                for (int child_pid : process_tree[pid]) {
                    displayProcessTree(child_pid, depth + 1);
                }
            }
        };
        
        // Display the tree starting from root processes
        for (int root_pid : root_pids) {
            displayProcessTree(root_pid, 0);
        }
    } else {
        // Flat view - show all processes in a list
        for (const auto& proc : sorted_processes) {
            // Apply filter if any
            if (!filter.empty() && 
                proc.name.find(filter) == string::npos && 
                to_string(proc.pid).find(filter) == string::npos) {
                continue;
            }
            
            // Check if this process is selected
            bool is_selected = selected_pids.find(proc.pid) != selected_pids.end();
            
            // Allow row selection
            char row_label[32];
            sprintf(row_label, "%d##%d", proc.pid, proc.pid);
            
            // Handle multi-selection with Ctrl key
            ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns;
            if (ImGui::Selectable(row_label, is_selected, flags)) {
                if (ImGui::GetIO().KeyCtrl) {
                    // Toggle selection with Ctrl
                    if (is_selected) selected_pids.erase(proc.pid);
                    else selected_pids.insert(proc.pid);
                } else {
                    // Single selection without Ctrl
                    selected_pids.clear();
                    selected_pids.insert(proc.pid);
                }
            }
            
            ImGui::NextColumn();
            ImGui::Text("%s", proc.name.c_str()); ImGui::NextColumn();
            ImGui::Text("%s", proc.getStateString().c_str()); ImGui::NextColumn();
            ImGui::Text("%d", proc.priority); ImGui::NextColumn();
            ImGui::Text("%.1f", proc.cpu_usage); ImGui::NextColumn();
            ImGui::Text("%.1f", proc.memory_usage); ImGui::NextColumn();
        }
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();
    
    // Process view options
    ImGui::Checkbox("Tree View", &tree_view);
    
    ImGui::SameLine();
    
    // Process actions
    if (ImGui::Button("Refresh")) {
        processes = getAllProcesses();
        updateProcessCpuUsage(processes);
        last_update_time = current_time;
    }
    
    ImGui::SameLine();
    
    if (!selected_pids.empty()) {
        if (ImGui::Button("Kill Selected Process(es)")) {
            for (int pid : selected_pids) {
                if (killProcess(pid)) {
                    // Process killed successfully
                } else {
                    // Failed to kill process
                }
            }
            // Refresh process list immediately after kill
            processes = getAllProcesses();
            updateProcessCpuUsage(processes);
            selected_pids.clear();
        }
        
        ImGui::SameLine();
        
        if (selected_pids.size() == 1) {
            if (ImGui::Button("Details")) {
                ImGui::OpenPopup("Process Details");
            }
        }
    }
    
    // Process alerts
    if (!selected_pids.empty() && selected_pids.size() == 1) {
        ImGui::SameLine();
        if (ImGui::Button("Add Alert")) {
            ImGui::OpenPopup("Add Process Alert");
        }
    }
    
    // Add process alert popup
    if (ImGui::BeginPopup("Add Process Alert")) {
        static float cpu_threshold = 50.0f;
        static float memory_threshold = 50.0f;
        
        if (selected_pids.size() == 1) {
            int pid = *selected_pids.begin();
            string name;
            
            // Find the selected process name
            for (const auto& p : processes) {
                if (p.pid == pid) {
                    name = p.name;
                    break;
                }
            }
            
            ImGui::Text("Add Alert for Process: %s (PID: %d)", name.c_str(), pid);
            ImGui::Separator();
            
            ImGui::SliderFloat("CPU Threshold (%)", &cpu_threshold, 0.0f, 100.0f);
            ImGui::SliderFloat("Memory Threshold (%)", &memory_threshold, 0.0f, 100.0f);
            
            if (ImGui::Button("Add Alert")) {
                // Check if alert already exists for this PID
                bool exists = false;
                for (auto& alert : g_process_alerts) {
                    if (alert.pid == pid) {
                        alert.cpu_threshold = cpu_threshold;
                        alert.memory_threshold = memory_threshold;
                        exists = true;
                        break;
                    }
                }
                
                if (!exists) {
                    g_process_alerts.emplace_back(pid, name, cpu_threshold, memory_threshold);
                }
                
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    
    // Process details popup
    if (ImGui::BeginPopup("Process Details")) {
        if (selected_pids.size() == 1) {
            int pid = *selected_pids.begin();
            Process proc;
            
            // Find the selected process
            for (const auto& p : processes) {
                if (p.pid == pid) {
                    proc = p;
                    break;
                }
            }
            
            ImGui::Text("Process Details");
            ImGui::Separator();
            ImGui::Text("PID: %d", proc.pid);
            ImGui::Text("Name: %s", proc.name.c_str());
            ImGui::Text("State: %s", proc.getStateString().c_str());
            ImGui::Text("Parent PID: %d", proc.ppid);
            ImGui::Text("Priority: %d", proc.priority);
            ImGui::Text("CPU Usage: %.2f%%", proc.cpu_usage);
            ImGui::Text("Memory Usage: %.2f%%", proc.memory_usage);
            ImGui::Text("Virtual Memory: %s", formatSize(proc.vsize).c_str());
            ImGui::Text("Resident Memory: %s", formatSize(proc.rss).c_str());
            
            // Priority adjustment
            static int new_priority = 0;
            if (ImGui::SliderInt("New Priority", &new_priority, -20, 19)) {
                // Slider moved
            }
            
            if (ImGui::Button("Set Priority")) {
                if (setProcessPriority(pid, new_priority)) {
                    // Priority set successfully
                    proc.priority = new_priority;
                }
            }
            
            ImGui::Separator();
            
            // Child processes
            vector<Process> children = getProcessChildren(pid);
            if (!children.empty()) {
                ImGui::Text("Child Processes:");
                for (const auto& child : children) {
                    ImGui::Text("PID: %d, Name: %s", child.pid, child.name.c_str());
                }
            } else {
                ImGui::Text("No child processes");
            }
        }
        ImGui::EndPopup();
    }
    
    ImGui::End();
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Network Interfaces Section
    ImGui::Text("Network Interfaces");
    ImGui::Separator();
    
    // Get all network interfaces
    static vector<NetworkInterface> interfaces;
    static float last_update_time = 0.0f;
    float current_time = ImGui::GetTime();
    
    // Update network interfaces every 5 seconds
    if (current_time - last_update_time >= 5.0f || interfaces.empty()) {
        interfaces = getNetworkInterfaces();
        last_update_time = current_time;
    }
    
    // Refresh button
    if (ImGui::Button("Refresh")) {
        interfaces = getNetworkInterfaces();
        last_update_time = current_time;
    }
    
    ImGui::Spacing();
    
    // Display interfaces in a table
    if (ImGui::BeginTable("NetworkInterfaces", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        // Table headers
        ImGui::TableSetupColumn("Interface");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("IPv4 Address");
        ImGui::TableSetupColumn("MAC Address");
        ImGui::TableHeadersRow();
        
        // Table rows
        for (const auto& interface : interfaces) {
            ImGui::TableNextRow();
            
            // Interface name
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(interface.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                // Select this interface for traffic monitoring
                static string selected_interface = interface.name;
                selected_interface = interface.name;
            }
            
            // Interface type
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", interface.type.c_str());
            
            // Interface status
            ImGui::TableSetColumnIndex(2);
            if (interface.is_up) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Up");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Down");
            }
            
            // IPv4 address
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", interface.ipv4_address.c_str());
            
            // MAC address
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", interface.mac_address.c_str());
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Network Traffic Section
    ImGui::Text("Network Traffic");
    ImGui::Separator();
    
    // Select interface for monitoring
    static string selected_interface = "";
    if (interfaces.size() > 0 && selected_interface.empty()) {
        // Default to first non-loopback interface, or loopback if that's all we have
        for (const auto& interface : interfaces) {
            if (interface.type != "Loopback") {
                selected_interface = interface.name;
                break;
            }
        }
        
        if (selected_interface.empty()) {
            selected_interface = interfaces[0].name;
        }
    }
    
    // Interface selection combo
    if (ImGui::BeginCombo("Interface", selected_interface.c_str())) {
        for (const auto& interface : interfaces) {
            bool is_selected = (selected_interface == interface.name);
            if (ImGui::Selectable(interface.name.c_str(), is_selected)) {
                selected_interface = interface.name;
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Get network stats for selected interface
    NetworkStats stats = getNetworkStats(selected_interface);
    
    // Display current traffic stats
    ImGui::Text("Received: %s (%lu packets)", formatSize(stats.rx_bytes).c_str(), stats.rx_packets);
    ImGui::Text("Sent: %s (%lu packets)", formatSize(stats.tx_bytes).c_str(), stats.tx_packets);
    
    // Network traffic graphs
    ImGui::Spacing();
    ImGui::Text("Traffic Graphs (KB/s)");
    
    // Graph controls
    if (ImGui::Button(g_rxGraph.paused ? "Play" : "Pause")) {
        g_rxGraph.paused = !g_rxGraph.paused;
        g_txGraph.paused = g_rxGraph.paused;
    }
    
    // FPS slider
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderFloat("FPS##net", &g_rxGraph.fps, 1.0f, 60.0f, "%.1f")) {
        g_txGraph.fps = g_rxGraph.fps;
    }
    
    // Y-axis scale slider
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderFloat("Scale##net", &g_rxGraph.scale, 10.0f, 1000.0f, "%.1f")) {
        g_txGraph.scale = g_rxGraph.scale;
    }
    
    // Update network graphs
    updateNetworkGraph(g_rxGraph, g_txGraph, selected_interface);
    
    // Plot the RX graph
    ImGui::PlotLines("##rxgraph", g_rxGraph.values, NetworkGraph::MAX_VALUES, 
                    g_rxGraph.values_offset, 
                    ("RX: " + to_string((int)g_rxGraph.values[g_rxGraph.values_offset == 0 ? NetworkGraph::MAX_VALUES - 1 : g_rxGraph.values_offset - 1]) + " KB/s").c_str(), 
                    0.0f, g_rxGraph.scale, ImVec2(ImGui::GetContentRegionAvail().x, 80));
    
    // Plot the TX graph
    ImGui::PlotLines("##txgraph", g_txGraph.values, NetworkGraph::MAX_VALUES, 
                    g_txGraph.values_offset, 
                    ("TX: " + to_string((int)g_txGraph.values[g_txGraph.values_offset == 0 ? NetworkGraph::MAX_VALUES - 1 : g_txGraph.values_offset - 1]) + " KB/s").c_str(), 
                    0.0f, g_txGraph.scale, ImVec2(ImGui::GetContentRegionAvail().x, 80));
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Network Connections and Ports Section
    if (ImGui::BeginTabBar("NetworkTabs")) {
        // Active Connections Tab
        if (ImGui::BeginTabItem("Connections")) {
            ImGui::Text("Active Network Connections");
            
            // Get active connections
            static vector<NetworkConnection> connections;
            static float last_conn_update = 0.0f;
            
            // Update connections every 3 seconds
            if (current_time - last_conn_update >= 3.0f || connections.empty()) {
                connections = getActiveConnections();
                last_conn_update = current_time;
            }
            
            // Refresh button
            if (ImGui::Button("Refresh Connections")) {
                connections = getActiveConnections();
                last_conn_update = current_time;
            }
            
            ImGui::Spacing();
            
            // Display connections in a table
            if (ImGui::BeginTable("Connections", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Protocol");
                ImGui::TableSetupColumn("Local Address");
                ImGui::TableSetupColumn("Remote Address");
                ImGui::TableSetupColumn("State");
                ImGui::TableHeadersRow();
                
                for (const auto& conn : connections) {
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", conn.protocol.c_str());
                    
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", conn.local_address.c_str());
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", conn.remote_address.c_str());
                    
                    ImGui::TableSetColumnIndex(3);
                    if (conn.state == "ESTABLISHED") {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", conn.state.c_str());
                    } else if (conn.state == "LISTEN") {
                        ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "%s", conn.state.c_str());
                    } else {
                        ImGui::Text("%s", conn.state.c_str());
                    }
                }
                
                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }
        
        // Listening Ports Tab
        if (ImGui::BeginTabItem("Ports")) {
            ImGui::Text("Listening Ports");
            
            // Get listening ports
            static vector<PortInfo> ports;
            static float last_port_update = 0.0f;
            
            // Update ports every 5 seconds
            if (current_time - last_port_update >= 5.0f || ports.empty()) {
                ports = getListeningPorts();
                last_port_update = current_time;
            }
            
            // Refresh button
            if (ImGui::Button("Refresh Ports")) {
                ports = getListeningPorts();
                last_port_update = current_time;
            }
            
            ImGui::Spacing();
            
            // Display ports in a table
            if (ImGui::BeginTable("Ports", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Port");
                ImGui::TableSetupColumn("Protocol");
                ImGui::TableSetupColumn("State");
                ImGui::TableHeadersRow();
                
                for (const auto& port : ports) {
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", port.port);
                    
                    ImGui::TableSetColumnIndex(1);
                    if (port.protocol == "TCP") {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", port.protocol.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "%s", port.protocol.c_str());
                    }
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", port.state.c_str());
                }
                
                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

// Main code
int main(int, char **)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char *name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // render bindings
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // background color
    // note : you are free to change the style of the application
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        {
            ImVec2 mainDisplay = io.DisplaySize;
            memoryProcessesWindow("== Memory and Processes ==",
                                  ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 30),
                                  ImVec2((mainDisplay.x / 2) + 10, 10));
            // --------------------------------------
            systemWindow("== System ==",
                         ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 30),
                         ImVec2(10, 10));
            // --------------------------------------
            networkWindow("== Network ==",
                          ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 60),
                          ImVec2(10, (mainDisplay.y / 2) + 50));
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
