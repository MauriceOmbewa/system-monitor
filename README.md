# System Monitor

A comprehensive C++ desktop system monitoring application built with Dear ImGui that provides real-time visualization of system performance metrics. The application reads directly from Linux `/proc` and `/sys` filesystems to deliver accurate, low-overhead system monitoring.

![System Monitor](https://img.shields.io/badge/Platform-Linux-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange)
![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

## 🚀 Features

### System Information Window
- **Operating System**: Displays current OS (Linux)
- **User & Hostname**: Shows logged-in user and system hostname
- **CPU Information**: Brand, model, and core count
- **System Uptime**: Days, hours, and minutes since boot
- **Load Average**: 1, 5, and 15-minute load averages
- **Real-time CPU Usage**: Current CPU utilization percentage
- **CPU Temperature**: Thermal monitoring from `/sys/class/thermal`

#### Tabbed Performance Graphs
- **CPU Tab**: Real-time CPU usage graph with customizable FPS and Y-axis scaling
- **Fan Tab**: Fan speed monitoring with RPM display and performance graphs
- **Thermal Tab**: Temperature monitoring with real-time thermal graphs
- **Process Summary**: Running, sleeping, stopped, and zombie process counts

### Memory & Processes Window
- **RAM Monitoring**: Total, used, and free memory with visual progress bars
- **SWAP Monitoring**: Swap space usage with detailed statistics
- **Disk Usage**: Multi-disk support with usage percentages and space information
- **Process Table**: Comprehensive process management with 5 columns:
  - PID (Process ID)
  - Name (Process name)
  - State (Running, Sleeping, etc.)
  - CPU% (CPU usage percentage)
  - Memory% (Memory usage percentage)
- **Process Features**:
  - Multi-selection support (Ctrl+click)
  - Real-time filtering by name or PID
  - Sortable columns
  - Tree view for parent-child relationships
  - Process termination capabilities
  - Priority adjustment
  - Process alerts with thresholds

### Network Window
- **Interface Table**: All network interfaces with type, status, IPv4, and MAC addresses
- **Traffic Statistics**: RX/TX bytes and packets in organized table format
- **Real-time Graphs**: Network traffic visualization (hidden in "Traffic Graphs" tab)
- **Connection Monitoring**: Active TCP connections with state information
- **Port Monitoring**: Listening ports (TCP/UDP) with protocol information

## 🛠️ Technical Architecture

### Core Components

#### 1. Main Application (`main.cpp`)
- **GUI Framework**: Dear ImGui with SDL2 backend and OpenGL3 rendering
- **Window Management**: Three main windows (System, Memory/Processes, Network)
- **Global State**: Graph instances and process alert management
- **Event Loop**: SDL2 event handling with ImGui integration

#### 2. System Monitoring (`system.cpp`)
- **CPU Metrics**: Usage calculation from `/proc/stat`
- **Temperature Monitoring**: Reads from `/sys/class/thermal/thermal_zone0/temp`
- **Fan Control**: Hardware monitoring via `/sys/class/hwmon` with fallback simulation
- **System Information**: OS detection, user info, hostname, uptime
- **Process Counting**: State-based process enumeration from `/proc`

#### 3. Memory & Process Management (`mem.cpp`)
- **Memory Statistics**: Parsing `/proc/meminfo` for RAM/SWAP data
- **Disk Monitoring**: Multi-filesystem support via `statvfs()` and `/proc/mounts`
- **Process Management**: Complete process information from `/proc/[pid]/stat` and `/proc/[pid]/status`
- **Process Tree**: Parent-child relationship mapping
- **Priority Control**: Process nice value adjustment

#### 4. Network Monitoring (`network.cpp`)
- **Interface Detection**: `getifaddrs()` system call for interface enumeration
- **Traffic Statistics**: Real-time parsing of `/proc/net/dev`
- **Connection Tracking**: TCP connection monitoring via `/proc/net/tcp`
- **Port Monitoring**: Listening port detection from `/proc/net/tcp` and `/proc/net/udp`
- **MAC Address Resolution**: Hardware address retrieval

#### 5. Header Definitions (`header.h`)
- **Data Structures**: All system monitoring structures (Process, MemoryInfo, DiskInfo, etc.)
- **Graph Classes**: Template-based graph system for real-time visualization
- **Function Declarations**: Complete API interface

### Data Flow

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   /proc & /sys  │───▶│  C++ Functions   │───▶│   ImGui GUI     │
│   Filesystems   │    │  (Data Parsing)  │    │  (Visualization)│
└─────────────────┘    └──────────────────┘    └─────────────────┘
        ▲                        │                        │
        │                        ▼                        ▼
        │               ┌──────────────────┐    ┌─────────────────┐
        └───────────────│  Update Timers   │    │  User Actions   │
                        │  (1-5 seconds)   │    │  (Click/Select) │
                        └──────────────────┘    └─────────────────┘
```

## 📋 Prerequisites

### System Requirements
- **Operating System**: Linux (Ubuntu 14.04+, or equivalent)
- **Compiler**: GCC 7.0+ or Clang 6.0+
- **Graphics**: OpenGL 3.0+ support

### Dependencies
- **SDL2**: Cross-platform development library
- **OpenGL**: Graphics rendering
- **Build Tools**: Make, GCC/G++

## 🔧 Installation

### Quick Start
```bash
# Clone the repository
git clone https://github.com/MauriceOmbewa/system-monitor.git
cd system-monitor

# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install -y libsdl2-dev libgl1-mesa-dev build-essential

# Build the project
make

# Run the system monitor
./monitor
```

### Alternative Package Managers

#### Fedora/RHEL/CentOS
```bash
sudo dnf install SDL2-devel mesa-libGL-devel gcc-c++ make
```

#### Arch Linux
```bash
sudo pacman -S sdl2 mesa gcc make
```

#### macOS (with Homebrew)
```bash
brew install sdl2
make
```

## 🎮 Usage Guide

### Interface Overview
The application displays three main windows:

1. **System Window** (Top-left): System information and performance graphs
2. **Memory & Processes Window** (Top-right): Memory usage and process management
3. **Network Window** (Bottom): Network interfaces and traffic monitoring

### Key Features

#### Process Management
- **Filter Processes**: Type in the filter box to search by name or PID
- **Multi-Selection**: Hold Ctrl and click to select multiple processes
- **Sort Columns**: Click column headers to sort by PID, Name, State, CPU%, or Memory%
- **Kill Processes**: Select processes and click "Kill Selected Process(es)"
- **Process Details**: Select a single process and click "Details" for comprehensive information
- **Set Alerts**: Monitor processes with CPU/Memory thresholds

#### Performance Monitoring
- **Graph Controls**: Each graph has Play/Pause, FPS adjustment (1-60), and Y-axis scaling
- **Real-time Updates**: All data refreshes automatically (1-5 second intervals)
- **Network Traffic**: Click "Traffic Graphs" tab to view RX/TX visualizations

#### System Information
- **Accurate Data**: All values match standard Linux commands (`top`, `free`, `df`, `ifconfig`)
- **Multi-disk Support**: Automatically detects all mounted filesystems
- **Network Interfaces**: Shows all interfaces with detailed information

## 🏗️ Build System

### Makefile Structure
The project uses a cross-platform Makefile supporting:
- **Linux**: SDL2 + OpenGL
- **macOS**: SDL2 + OpenGL with framework support
- **Windows**: MinGW with SDL2

### Build Targets
```bash
make          # Build the project
make clean    # Remove all compiled files
make all      # Same as make
```

### Compilation Flags
- **Debug**: `-g` for debugging symbols
- **Warnings**: `-Wall -Wformat` for code quality
- **OpenGL Loader**: gl3w (default) with fallback options

## 🤝 Contributing

### Code Structure
When contributing, follow this organization:

```
system-monitor/
├── main.cpp           # GUI application and window management
├── system.cpp         # System information and CPU/thermal monitoring
├── mem.cpp           # Memory, disk, and process management
├── network.cpp       # Network interface and traffic monitoring
├── header.h          # All structure definitions and function declarations
├── Makefile          # Cross-platform build configuration
└── imgui/lib/        # Dear ImGui library files
```

### Adding New Features

1. **System Metrics**: Add functions to `system.cpp` and declare in `header.h`
2. **Memory Features**: Extend `mem.cpp` for new memory/process functionality
3. **Network Features**: Modify `network.cpp` for network-related additions
4. **GUI Elements**: Update appropriate window function in `main.cpp`

### Code Style Guidelines
- **Naming**: Use camelCase for functions, snake_case for variables
- **Comments**: Document complex algorithms and data sources
- **Error Handling**: Always check file operations and system calls
- **Memory Management**: Use RAII principles, avoid raw pointers

### Testing
Ensure your changes pass these validation tests:
- Values match standard Linux commands (`top`, `free`, `df`, `ifconfig`)
- GUI remains responsive during high system load
- Memory usage stays reasonable during extended operation
- All features work without root privileges

### Pull Request Process
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 🐛 Troubleshooting

### Common Issues

#### Build Errors
```bash
# Missing SDL2 development headers
sudo apt install libsdl2-dev

# Missing OpenGL development libraries
sudo apt install libgl1-mesa-dev

# Compiler not found
sudo apt install build-essential
```

#### Runtime Issues
```bash
# Permission denied for /proc files
# Solution: Run without sudo (application designed for user-level access)

# Display issues
# Solution: Ensure X11 forwarding if using SSH, or run locally

# Missing libraries
ldd ./monitor  # Check for missing shared libraries
```

### Performance Considerations
- **Update Intervals**: Graphs update at configurable FPS (1-60)
- **Memory Usage**: Application uses minimal memory (~10-20MB)
- **CPU Overhead**: Monitoring overhead is <1% CPU usage
- **File Descriptors**: Efficiently manages /proc file access

## 📊 Data Sources

### Linux Filesystem Integration
- **`/proc/stat`**: CPU usage statistics
- **`/proc/meminfo`**: Memory and swap information
- **`/proc/net/dev`**: Network interface statistics
- **`/proc/net/tcp`**: TCP connection information
- **`/proc/[pid]/stat`**: Process information
- **`/proc/[pid]/status`**: Detailed process status
- **`/proc/mounts`**: Mounted filesystem information
- **`/sys/class/thermal/`**: Temperature sensors
- **`/sys/class/hwmon/`**: Hardware monitoring (fans)

### System Calls
- **`getifaddrs()`**: Network interface enumeration
- **`statvfs()`**: Filesystem statistics
- **`gethostname()`**: System hostname
- **`getlogin()`**: Current user information
- **`kill()`**: Process termination
- **`setpriority()`**: Process priority adjustment

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2025 MAURICE ODHIAMBO OMBEWA

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

## 🙏 Acknowledgments

- **Dear ImGui**: Excellent immediate mode GUI library
- **SDL2**: Cross-platform development library
- **Linux Kernel**: Comprehensive /proc and /sys filesystem interfaces
- **OpenGL**: Graphics rendering capabilities

## 📞 Support

For questions, issues, or contributions:
- **GitHub Issues**: [Create an issue](https://github.com/MauriceOmbewa/system-monitor/issues)
- **Email**: [Your contact information]
- **Documentation**: This README and inline code comments

---

**Built with ❤️ for Linux system monitoring**
