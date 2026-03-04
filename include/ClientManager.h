#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <string>
#include <map>
#include <mutex>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

// 客户端连接信息结构体
struct ClientInfo {
    int socket_fd;
    std::string ip_address;
    uint16_t port;
    time_t connect_time;
    bool is_active;
    std::string status;
    int server_id; // 1: server1, 2: server2
    std::string server_local_ip; // 服务器本地IP
    uint8_t ad_channel; // AD通道号（0-7）
    uint32_t sequence_num; // 序列号（与客户端同步）
    bool in_iq_stream; // 是否在IQ数据流中
    
    ClientInfo() : socket_fd(-1), port(0), connect_time(0), is_active(false), 
                   status("disconnected"), server_id(0), server_local_ip(""),
                   ad_channel(0), sequence_num(1), in_iq_stream(false) {}
                   
    ClientInfo(int fd, const std::string& ip, uint16_t p, int sid, 
               const std::string& server_ip, uint8_t ad_ch = 0) 
        : socket_fd(fd), ip_address(ip), port(p), connect_time(time(nullptr)), 
          is_active(true), status("connected"), server_id(sid), 
          server_local_ip(server_ip), ad_channel(ad_ch), 
          sequence_num(1), in_iq_stream(false) {}
          
    // 更新AD通道信息
    void update_ad_channel(uint8_t channel) {
        ad_channel = channel;
    }
    
    // 获取下一个序列号
    uint32_t get_next_sequence_num() {
        return sequence_num++;
    }
    
    // 设置序列号
    void set_sequence_num(uint32_t seq) {
        sequence_num = seq;
    }
    
    // 格式化客户端信息字符串
    std::string to_string() const {
        char time_buf[32];
        struct tm* tm_info = localtime(&connect_time);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-d %H:%M:%S", tm_info);
        
        return std::string("客户端[AD") + std::to_string(static_cast<int>(ad_channel)) + 
               "] " + ip_address + ":" + std::to_string(port) + 
               " (服务器" + std::to_string(server_id) + ")" +
               " 状态:" + status + " 连接时间:" + time_buf;
    }
};

class ClientManager {
private:
    std::map<int, ClientInfo> client_connections;
    mutable std::mutex client_mutex;

    ClientManager() = default;
    ~ClientManager() = default;

public:
    // 单例模式
    static ClientManager& get_instance() {
        static ClientManager instance;
        return instance;
    }
    
    // 禁止拷贝和移动
    ClientManager(const ClientManager&) = delete;
    ClientManager& operator=(const ClientManager&) = delete;
    ClientManager(ClientManager&&) = delete;
    ClientManager& operator=(ClientManager&&) = delete;
    
    // 客户端管理方法
    void add_client(int socket_fd, const std::string& ip, uint16_t port, 
                    int server_id, const std::string& server_ip, uint8_t ad_channel = 0);
    
    void remove_client(int socket_fd);
    
    void update_client_status(int socket_fd, const std::string& status, bool is_active = true);
    
    void update_client_ad_channel(int socket_fd, uint8_t ad_channel);
    
    void update_client_sequence_num(int socket_fd, uint32_t seq_num);
    
    ClientInfo* get_client(int socket_fd);
    
    std::vector<ClientInfo> get_all_clients();
    
    std::vector<ClientInfo> get_server_clients(int server_id);
    
    std::vector<ClientInfo> get_ad_channel_clients(uint8_t ad_channel);
    
    int get_client_count() const;
    
    int get_server_client_count(int server_id) const;
    
    int get_ad_channel_client_count(uint8_t ad_channel) const;
    
    // 查找主客户端（AD0）
    int find_master_client_socket(int server_id = 0);
    
    int find_client_socket_by_display_id(int server_id, int display_id);
    
    int find_client_socket_by_ad_channel(uint8_t ad_channel, int server_id = 0);
    
    void print_clients();
    
    void print_server_status();
    
    // 广播到特定AD通道的所有客户端
    template<typename Func>
    void broadcast_to_ad_channel(uint8_t ad_channel, Func func) {
        std::lock_guard<std::mutex> lock(client_mutex);
        for (auto& pair : client_connections) {
            if (pair.second.is_active && pair.second.ad_channel == ad_channel) {
                func(pair.second);
            }
        }
    }
    
};

#endif // CLIENT_MANAGER_H