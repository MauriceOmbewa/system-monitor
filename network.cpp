#include "header.h"

// Get all network interfaces with their IPv4 addresses
vector<NetworkInterface> getNetworkInterfaces() {
    vector<NetworkInterface> interfaces;
    struct ifaddrs *ifaddr, *ifa;
    
    // Get all network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        return interfaces;
    }
    
    // Iterate through all interfaces
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        // Only consider IPv4 addresses
        if (ifa->ifa_addr->sa_family == AF_INET) {
            NetworkInterface interface;
            interface.name = ifa->ifa_name;
            
            // Get IPv4 address
            char address_buffer[INET_ADDRSTRLEN];
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), address_buffer, INET_ADDRSTRLEN);
            interface.ipv4_address = address_buffer;
            
            // Get MAC address
            interface.mac_address = getMacAddress(interface.name);
            
            // Check if interface is up
            interface.is_up = isInterfaceUp(interface.name);
            
            // Get interface type
            interface.type = getInterfaceType(interface.name);
            
            interfaces.push_back(interface);
        }
    }
    
    freeifaddrs(ifaddr);
    return interfaces;
}

// Get MAC address for a network interface
string getMacAddress(const string& interface_name) {
    string mac_address = "";
    
    // Try to read from /sys/class/net/[interface]/address
    string path = "/sys/class/net/" + interface_name + "/address";
    ifstream mac_file(path);
    
    if (mac_file.is_open()) {
        getline(mac_file, mac_address);
    } else {
        // Fallback method using ioctl
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1) {
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
            
            if (ioctl(fd, SIOCGIFHWADDR, &ifr) != -1) {
                char mac[18];
                unsigned char* mac_ptr = (unsigned char*)ifr.ifr_hwaddr.sa_data;
                
                snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                         mac_ptr[0], mac_ptr[1], mac_ptr[2],
                         mac_ptr[3], mac_ptr[4], mac_ptr[5]);
                
                mac_address = mac;
            }
            
            close(fd);
        }
    }
    
    return mac_address;
}

// Check if a network interface is up
bool isInterfaceUp(const string& interface_name) {
    bool is_up = false;
    
    // Try to read from /sys/class/net/[interface]/operstate
    string path = "/sys/class/net/" + interface_name + "/operstate";
    ifstream state_file(path);
    
    if (state_file.is_open()) {
        string state;
        getline(state_file, state);
        is_up = (state == "up");
    } else {
        // Fallback method using ioctl
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1) {
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
            
            if (ioctl(fd, SIOCGIFFLAGS, &ifr) != -1) {
                is_up = (ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING);
            }
            
            close(fd);
        }
    }
    
    return is_up;
}

// Get the type of a network interface
string getInterfaceType(const string& interface_name) {
    // Check common interface name prefixes to determine type
    if (interface_name.substr(0, 2) == "lo") {
        return "Loopback";
    } else if (interface_name.substr(0, 3) == "eth") {
        return "Ethernet";
    } else if (interface_name.substr(0, 4) == "wlan") {
        return "Wireless";
    } else if (interface_name.substr(0, 3) == "wlp") {
        return "Wireless";
    } else if (interface_name.substr(0, 3) == "enp") {
        return "Ethernet";
    } else if (interface_name.substr(0, 3) == "tun") {
        return "VPN Tunnel";
    } else if (interface_name.substr(0, 3) == "tap") {
        return "TAP";
    } else if (interface_name.substr(0, 4) == "bond") {
        return "Bond";
    } else if (interface_name.substr(0, 5) == "bridge") {
        return "Bridge";
    } else if (interface_name.substr(0, 4) == "virbr") {
        return "Virtual Bridge";
    } else if (interface_name.substr(0, 4) == "docker") {
        return "Docker";
    } else {
        // Try to determine type from /sys/class/net/[interface]/type
        string path = "/sys/class/net/" + interface_name + "/type";
        ifstream type_file(path);
        
        if (type_file.is_open()) {
            int type_num;
            type_file >> type_num;
            
            // ARPHRD_ETHER = 1, ARPHRD_LOOPBACK = 772
            if (type_num == 1) {
                return "Ethernet";
            } else if (type_num == 772) {
                return "Loopback";
            }
        }
        
        return "Unknown";
    }
}

// Get network statistics for a specific interface
NetworkStats getNetworkStats(const string& interface_name) {
    NetworkStats stats;
    stats.interface_name = interface_name;
    stats.rx_bytes = 0;
    stats.rx_packets = 0;
    stats.tx_bytes = 0;
    stats.tx_packets = 0;
    stats.rx_speed = 0.0f;
    stats.tx_speed = 0.0f;
    
    // Read from /proc/net/dev
    ifstream net_dev("/proc/net/dev");
    if (!net_dev.is_open()) {
        return stats;
    }
    
    string line;
    // Skip header lines
    getline(net_dev, line); // Inter-|   Receive                                                |
    getline(net_dev, line); // face |bytes    packets errs drop fifo frame compressed multicast|
    
    // Find the interface
    while (getline(net_dev, line)) {
        // Remove leading spaces
        line.erase(0, line.find_first_not_of(" \t"));
        
        // Extract interface name (before the colon)
        size_t colon_pos = line.find(':');
        if (colon_pos == string::npos) continue;
        
        string if_name = line.substr(0, colon_pos);
        if (if_name != interface_name) continue;
        
        // Parse the statistics
        stringstream ss(line.substr(colon_pos + 1));
        unsigned long rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
        unsigned long tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
        
        ss >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> rx_compressed >> rx_multicast
           >> tx_bytes >> tx_packets >> tx_errs >> tx_drop >> tx_fifo >> tx_colls >> tx_carrier >> tx_compressed;
        
        stats.rx_bytes = rx_bytes;
        stats.rx_packets = rx_packets;
        stats.tx_bytes = tx_bytes;
        stats.tx_packets = tx_packets;
        
        break;
    }
    
    return stats;
}

// Update network traffic graphs
void updateNetworkGraph(NetworkGraph& rx_graph, NetworkGraph& tx_graph, const string& interface_name) {
    static float lastUpdateTime = 0.0f;
    static unsigned long last_rx_bytes = 0;
    static unsigned long last_tx_bytes = 0;
    
    float currentTime = ImGui::GetTime();
    float deltaTime = currentTime - lastUpdateTime;
    
    // Update at the specified FPS rate
    if (deltaTime >= 1.0f / rx_graph.fps) {
        NetworkStats stats = getNetworkStats(interface_name);
        
        // Calculate speeds (bytes per second)
        if (last_rx_bytes > 0 && last_tx_bytes > 0 && deltaTime > 0) {
            float rx_speed = (stats.rx_bytes - last_rx_bytes) / deltaTime;
            float tx_speed = (stats.tx_bytes - last_tx_bytes) / deltaTime;
            
            // Add values to graphs (convert to KB/s for better visualization)
            rx_graph.addValue(rx_speed / 1024.0f);
            tx_graph.addValue(tx_speed / 1024.0f);
            
            // Update stats object with calculated speeds
            stats.rx_speed = rx_speed;
            stats.tx_speed = tx_speed;
        }
        
        // Store current values for next calculation
        last_rx_bytes = stats.rx_bytes;
        last_tx_bytes = stats.tx_bytes;
        lastUpdateTime = currentTime;
    }
}

// Get process name from PID
string getProcessNameFromPid(int pid) {
    if (pid <= 0) return "";
    
    // Try to read from /proc/[pid]/comm
    string comm_path = "/proc/" + to_string(pid) + "/comm";
    ifstream comm_file(comm_path);
    if (comm_file.is_open()) {
        string name;
        getline(comm_file, name);
        return name;
    }
    
    // Try to read from /proc/[pid]/cmdline
    string cmdline_path = "/proc/" + to_string(pid) + "/cmdline";
    ifstream cmdline_file(cmdline_path);
    if (cmdline_file.is_open()) {
        string cmdline;
        getline(cmdline_file, cmdline);
        
        // Extract the base command name (remove path and arguments)
        if (!cmdline.empty()) {
            // Replace null bytes with spaces
            replace(cmdline.begin(), cmdline.end(), '\0', ' ');
            
            // Extract the command name (without path)
            size_t slash_pos = cmdline.find_last_of('/');
            if (slash_pos != string::npos) {
                cmdline = cmdline.substr(slash_pos + 1);
            }
            
            // Extract just the command (without arguments)
            size_t space_pos = cmdline.find(' ');
            if (space_pos != string::npos) {
                cmdline = cmdline.substr(0, space_pos);
            }
            
            return cmdline;
        }
    }
    
    return "";
}

// Get active network connections
vector<NetworkConnection> getActiveConnections() {
    vector<NetworkConnection> connections;
    
    // Read TCP connections from /proc/net/tcp
    ifstream tcp_file("/proc/net/tcp");
    if (tcp_file.is_open()) {
        string line;
        getline(tcp_file, line); // Skip header
        
        while (getline(tcp_file, line)) {
            stringstream ss(line);
            string sl, local_addr, rem_addr, st, tx_queue, rx_queue, tr, tm_when, retrnsmt, uid, timeout, inode;
            
            ss >> sl >> local_addr >> rem_addr >> st >> tx_queue >> rx_queue >> tr >> tm_when >> retrnsmt >> uid >> timeout >> inode;
            
            NetworkConnection conn;
            conn.protocol = "TCP";
            
            // Parse local address (hex format: AAAAAAAA:PPPP)
            if (local_addr.length() >= 9) {
                unsigned int addr = stoul(local_addr.substr(0, 8), nullptr, 16);
                unsigned int port = stoul(local_addr.substr(9), nullptr, 16);
                
                char ip_str[INET_ADDRSTRLEN];
                struct in_addr in_addr;
                in_addr.s_addr = addr;
                inet_ntop(AF_INET, &in_addr, ip_str, INET_ADDRSTRLEN);
                
                conn.local_address = string(ip_str) + ":" + to_string(port);
            }
            
            // Parse remote address
            if (rem_addr.length() >= 9) {
                unsigned int addr = stoul(rem_addr.substr(0, 8), nullptr, 16);
                unsigned int port = stoul(rem_addr.substr(9), nullptr, 16);
                
                char ip_str[INET_ADDRSTRLEN];
                struct in_addr in_addr;
                in_addr.s_addr = addr;
                inet_ntop(AF_INET, &in_addr, ip_str, INET_ADDRSTRLEN);
                
                conn.remote_address = string(ip_str) + ":" + to_string(port);
            }
            
            // Parse state
            int state_num = stoi(st, nullptr, 16);
            switch (state_num) {
                case 1: conn.state = "ESTABLISHED"; break;
                case 2: conn.state = "SYN_SENT"; break;
                case 3: conn.state = "SYN_RECV"; break;
                case 4: conn.state = "FIN_WAIT1"; break;
                case 5: conn.state = "FIN_WAIT2"; break;
                case 6: conn.state = "TIME_WAIT"; break;
                case 7: conn.state = "CLOSE"; break;
                case 8: conn.state = "CLOSE_WAIT"; break;
                case 9: conn.state = "LAST_ACK"; break;
                case 10: conn.state = "LISTEN"; break;
                case 11: conn.state = "CLOSING"; break;
                default: conn.state = "UNKNOWN"; break;
            }
            
            // Find process using the inode
            conn.pid = 0;
            conn.process_name = "";
            
            // Simple approach: just add the connection without PID lookup for now
            connections.push_back(conn);
        }
    }
    
    return connections;
}
