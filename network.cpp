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
