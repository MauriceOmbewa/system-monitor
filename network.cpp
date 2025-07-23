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
