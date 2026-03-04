#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <string>
#include "ProtocolHandler.h"

class NetworkUtils {
public:
    // 获取指定IP的网络接口信息
    static IPInfo get_interface_info(const std::string& target_ip);
    
    // 获取本地socket的IP地址
    static std::string get_local_socket_ip(int socket_fd);
    
    // 获取服务器IP信息（用于发送给客户端）
    static IPInfo get_server_ip_info(int server_id, const std::string& server_ip = "");
    
private:
    NetworkUtils() = default;
    ~NetworkUtils() = default;
};

#endif // NETWORK_UTILS_H