#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <string>
#include <atomic>
#include <functional>
#include <vector>
#include <cstdint>
#include <netinet/in.h>
#include "ProtocolHandler.h"  // 包含ProtocolHandler头文件

class TCPServer {
private:
    int server_socket;
    int port;
    std::string ip_address;
    std::atomic<bool> running;

    // 协议处理器实例
    ProtocolHandler protocol_handler;
    
    // 设置socket选项
    bool set_socket_options(int socket_fd);

    // 设置为非阻塞模式
    bool set_nonblocking(int socket);

    // 设置超时
    bool set_timeout(int socket_fd, int recv_timeout_ms, int send_timeout_ms);

public:
    TCPServer();
    ~TCPServer();

    // 启动服务器
    bool start(const std::string& ip, int port);

    // 停止服务器
    void stop();

    // 接受客户端连接
    int accept_connection(int timeout_ms = 1000);

    // 发送数据
    bool send_data(int client_socket, const std::string& data);
    bool send_data(int client_socket, const char* data, size_t length);
    
    // 发送数据包（封装编码和发送）
    bool send_packet(int client_socket, const ProtocolPacket& packet);
    
    // 接收数据包（封装接收和解码）
    bool receive_packet(int client_socket, ProtocolPacket& packet, int timeout_ms = 1000);

    // 接收数据（原始接口）
    std::string receive_data(int client_socket, int timeout_ms = 1000);
    bool receive_data(int client_socket, char* buffer, size_t buffer_size,
                     int timeout_ms = 1000, ssize_t* bytes_received = nullptr);

    // 接收完整数据包（原始接口）
    bool receive_packet_raw(int client_socket, std::vector<uint8_t>& packet_buffer,
                       size_t packet_size, int timeout_ms = 1000);

    // 获取客户端信息
    std::string get_client_info(int client_socket);

    // 获取客户端IP和端口
    bool get_client_address(int client_socket, std::string& ip, uint16_t& port);

    // 检查服务器是否运行
    bool is_running() const { return running.load(); }
    
    // 接收完整协议包（兼容性接口）
    bool receive_complete_packet(int client_socket, 
                                std::vector<uint8_t>& packet_buffer,
                                int timeout_ms);
    
    // 获取协议处理器引用
    ProtocolHandler& get_protocol_handler() { return protocol_handler; }


};

#endif // TCP_SERVER_H