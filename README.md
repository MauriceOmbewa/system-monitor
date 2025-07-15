# system-monitor
A C++ desktop system monitor using Dear ImGui for the GUI. It visualizes real-time CPU, memory, disk, fan, thermal, and network usage by reading from the Linux /proc and /sys filesystems.

## Prerequisites
- Linux operating system
- GCC compiler
- SDL2 development libraries
- OpenGL development libraries

## Installation

### 1. Clone the repository
```bash
git clone https://github.com/MauriceOmbewa/system-monitor.git
cd system-monitor
```

### 2. Install dependencies
```bash
sudo apt update
sudo apt install -y libsdl2-dev libgl1-mesa-dev build-essential
```

### 3. Build the project
```bash
make
```

### 4. Run the system monitor
```bash
./monitor
```

## Usage
Once running, the system monitor displays:
- Real-time CPU usage and temperature
- Memory usage statistics
- Network activity
- System information

The GUI updates automatically and provides an intuitive interface for monitoring system performance.

## Clean build
To clean compiled files:
```bash
make clean
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
