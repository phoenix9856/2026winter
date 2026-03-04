#include "NetworkUtils.h"
#include <iostream>
#include <cstring>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <glog/logging.h>

IPInfo NetworkUtils::get_interface_info(const std::string& target_ip) {
    IPInfo info;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        // 使用默认值
        info.set_ip(target_ip);
        info.set_netmask("255.255.255.0");
        info.set_current_timestamp();
        return info;
    }
    
    // 遍历所有网络接口
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        // 只处理IPv4接口
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN);
            
            // 检查是否匹配目标IP
            if (strcmp(ip_str, target_ip.c_str()) == 0) {
                // 获取IP地址
                info.set_ip(ip_str);
                
                // 获取子网掩码
                struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
                if (netmask) {
                    char netmask_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &netmask->sin_addr, netmask_str, INET_ADDRSTRLEN);
                    info.set_netmask(netmask_str);
                }
                
                // 设置当前时间戳
                info.set_current_timestamp();
                
                LOG(INFO) << "找到网络接口: " << ifa->ifa_name 
                         << ", IP: " << ip_str 
                         << ", 子网掩码: " << info.get_netmask_string();
                break;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    
    // 如果没有找到，使用默认值
    if (info.ip_bytes[0] == 0) {
        info.set_ip(target_ip);
        info.set_netmask("255.255.255.0");
        info.set_current_timestamp();
        LOG(WARNING) << "未找到本地IP " << target_ip << " 的接口信息，使用默认值";
    }
    
    return info;
}

std::string NetworkUtils::get_local_socket_ip(int socket_fd) {
    struct sockaddr_in local_addr;
    socklen_t local_len = sizeof(local_addr);
    
    if (getsockname(socket_fd, (struct sockaddr*)&local_addr, &local_len) == 0) {
        char local_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);
        return std::string(local_ip);
    }
    
    return "";
}

IPInfo NetworkUtils::get_server_ip_info(int server_id, const std::string& server_ip) {
    std::string target_ip;
    
    if (!server_ip.empty()) {
        target_ip = server_ip;
    } else {
        // 根据服务器ID使用默认IP
        if (server_id == 1) {
            target_ip = "192.168.2.170";
        } else if (server_id == 2) {
            target_ip = "192.168.1.111";
        } else {
            target_ip = "127.0.0.1";
        }
    }
    
    return get_interface_info(target_ip);
}