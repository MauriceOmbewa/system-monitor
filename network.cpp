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
