#include "ClientManager.h"
#include "ProtocolHandler.h"
#include "config.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <ctime>

void ClientManager::add_client(int socket_fd, const std::string& ip, uint16_t port, 
                               int server_id, const std::string& server_ip, uint8_t ad_channel) {
    std::lock_guard<std::mutex> lock(client_mutex);
    client_connections[socket_fd] = ClientInfo(socket_fd, ip, port, server_id, server_ip, ad_channel);
}

void ClientManager::remove_client(int socket_fd) {
    std::lock_guard<std::mutex> lock(client_mutex);
    client_connections.erase(socket_fd);
}

void ClientManager::update_client_status(int socket_fd, const std::string& status, bool is_active) {
    std::lock_guard<std::mutex> lock(client_mutex);
    auto it = client_connections.find(socket_fd);
    if (it != client_connections.end()) {
        it->second.status = status;
        it->second.is_active = is_active;
    }
}

void ClientManager::update_client_ad_channel(int socket_fd, uint8_t ad_channel) {
    std::lock_guard<std::mutex> lock(client_mutex);
    auto it = client_connections.find(socket_fd);
    if (it != client_connections.end()) {
        it->second.update_ad_channel(ad_channel);
    }
}

void ClientManager::update_client_sequence_num(int socket_fd, uint32_t seq_num) {
    std::lock_guard<std::mutex> lock(client_mutex);
    auto it = client_connections.find(socket_fd);
    if (it != client_connections.end()) {
        it->second.set_sequence_num(seq_num);
    }
}

ClientInfo* ClientManager::get_client(int socket_fd) {
    std::lock_guard<std::mutex> lock(client_mutex);
    auto it = client_connections.find(socket_fd);
    if (it != client_connections.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ClientInfo> ClientManager::get_all_clients() {
    std::lock_guard<std::mutex> lock(client_mutex);
    std::vector<ClientInfo> clients;
    for (const auto& pair : client_connections) {
        clients.push_back(pair.second);
    }
    return clients;
}

std::vector<ClientInfo> ClientManager::get_server_clients(int server_id) {
    std::lock_guard<std::mutex> lock(client_mutex);
    std::vector<ClientInfo> clients;
    for (const auto& pair : client_connections) {
        if (pair.second.server_id == server_id) {
            clients.push_back(pair.second);
        }
    }
    return clients;
}

std::vector<ClientInfo> ClientManager::get_ad_channel_clients(uint8_t ad_channel) {
    std::lock_guard<std::mutex> lock(client_mutex);
    std::vector<ClientInfo> clients;
    for (const auto& pair : client_connections) {
        if (pair.second.ad_channel == ad_channel) {
            clients.push_back(pair.second);
        }
    }
    return clients;
}

int ClientManager::get_client_count() const {
    std::lock_guard<std::mutex> lock(client_mutex);
    return client_connections.size();
}

int ClientManager::get_server_client_count(int server_id) const {
    std::lock_guard<std::mutex> lock(client_mutex);
    int count = 0;
    for (const auto& pair : client_connections) {
        if (pair.second.server_id == server_id && pair.second.is_active) {
            count++;
        }
    }
    return count;
}

int ClientManager::get_ad_channel_client_count(uint8_t ad_channel) const {
    std::lock_guard<std::mutex> lock(client_mutex);
    int count = 0;
    for (const auto& pair : client_connections) {
        if (pair.second.ad_channel == ad_channel && pair.second.is_active) {
            count++;
        }
    }
    return count;
}

int ClientManager::find_master_client_socket(int server_id) {
    std::lock_guard<std::mutex> lock(client_mutex);
    for (const auto& pair : client_connections) {
        const ClientInfo& client = pair.second;
        if (client.is_active && client.ad_channel == MASTER_CLIENT_CHANNEL) {
            if (server_id == 0 || client.server_id == server_id) {
                return client.socket_fd;
            }
        }
    }
    return -1;
}

int ClientManager::find_client_socket_by_display_id(int server_id, int display_id) {
    std::lock_guard<std::mutex> lock(client_mutex);
    
    int current_id = 1;
    for (const auto& pair : client_connections) {
        const ClientInfo& client = pair.second;
        if (client.is_active && client.server_id == server_id) {
            if (current_id == display_id) {
                return client.socket_fd;
            }
            current_id++;
        }
    }
    
    return -1; // 未找到
}

int ClientManager::find_client_socket_by_ad_channel(uint8_t ad_channel, int server_id) {
    std::lock_guard<std::mutex> lock(client_mutex);
    for (const auto& pair : client_connections) {
        if (pair.second.is_active && pair.second.ad_channel == ad_channel) {
            if (server_id == 0 || pair.second.server_id == server_id) {
                return pair.first;
            }
        }
    }
    return -1;
}

void ClientManager::print_clients() {
    std::lock_guard<std::mutex> lock(client_mutex);
    
    if (client_connections.empty()) {
        std::cout << "当前没有客户端连接\n";
        return;
    }
    
    // 统计活跃数
    int active_count = 0;
    for (const auto& pair : client_connections) {
        if (pair.second.is_active) {
            active_count++;
        }
    }

    std::cout << "\n===== 已连接客户端列表 =====\n";
    std::cout << "ID | Socket | AD通道 | 服务器(IP) | 客户端IP:端口 | 状态 | 连接时间 | 序列号\n";
    std::cout << "---------------------------------------------------------------------------\n";
    
    int display_id = 1;
    for (const auto& pair : client_connections) {
        const ClientInfo& client = pair.second;
        char time_buf[32];
        struct tm* tm_info = localtime(&client.connect_time);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        std::string server_name = "服务器" + std::to_string(client.server_id);
        std::string server_ip_display = server_name + "(" + client.server_local_ip + ")";
        
        std::cout << display_id << " | " << client.socket_fd << " | AD" 
                  << static_cast<int>(client.ad_channel) << " | "
                  << server_ip_display << " | "
                  << client.ip_address << ":" << client.port 
                  << " | " << client.status << " | " << time_buf
                  << " | " << client.sequence_num << std::endl;
        display_id++;
    }
    
    std::cout << "\n总连接数: " << client_connections.size() 
              << " | 活跃连接: " << active_count << std::endl;
}

void ClientManager::print_server_status() {
    std::lock_guard<std::mutex> lock(client_mutex);
    
    int active_clients = 0;
    int server1_clients = 0;
    int server2_clients = 0;
    std::map<uint8_t, int> ad_counts;
    
    for (const auto& pair : client_connections) {
        const ClientInfo& client = pair.second;
        if (client.is_active) {
            active_clients++;
            if (client.server_id == 1) server1_clients++;
            else if (client.server_id == 2) server2_clients++;
            
            // 统计AD通道
            ad_counts[client.ad_channel]++;
        }
    }

    std::cout << "\n===== 服务器状态 =====\n";
    std::cout << "活跃客户端总数: " << active_clients << std::endl;
    std::cout << "服务器1(" << SERVER_IP_1 << ":" << SERVER_PORT_1 << ")客户端: " 
              << server1_clients << std::endl;
    std::cout << "服务器2(" << SERVER_IP_2 << ":" << SERVER_PORT_2 << ")客户端: " 
              << server2_clients << std::endl;
    
    // 显示AD通道分布
    std::cout << "\nAD通道分布:\n";
    for (int ad = 0; ad < NUM_AD_CHANNELS; ad++) {
        if (ad_counts.find(ad) != ad_counts.end()) {
            std::cout << "  AD" << ad << ": " << ad_counts[ad] << " 个客户端";
            if (ad == MASTER_CLIENT_CHANNEL) {
                std::cout << " (主客户端)";
            }
            std::cout << std::endl;
        }
    }
    
    char time_buf[32];
    time_t current_time = time(nullptr);
    struct tm* tm_info = localtime(&current_time);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    std::cout << "\n服务器时间: " << time_buf << " (" << time(nullptr) << ")" << std::endl;
    std::cout << "========================\n";
}