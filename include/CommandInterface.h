#ifndef COMMAND_INTERFACE_H
#define COMMAND_INTERFACE_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include "TCPServer.h"
#include "ProtocolHandler.h"
#include "ClientManager.h"
#include "GlobalCollectState.h"

class CommandInterface {
private:
    TCPServer& m_server1;
    TCPServer& m_server2;
    ProtocolHandler& m_handler1;
    ProtocolHandler& m_handler2;
    GlobalCollectState& m_collect_state;

    std::thread m_cli_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_global_running;

    // 命令行交互线程函数
    void command_line_interface();
    
    // 根据客户端显示ID查找socket
    int find_client_socket_by_id(int server_id, int client_display_id) const;
    
    // 根据AD通道查找客户端socket
    int find_client_socket_by_ad_channel(int server_id, uint8_t ad_channel) const;
    
    // 获取指定服务器的活跃客户端数量
    int get_active_clients_count(int server_id) const;
    
    // 处理发送命令
    void handle_send_command(const std::vector<std::string>& tokens);
    
    // 处理采集IQ命令
    void handle_collect_command(const std::vector<std::string>& tokens);


public:
    CommandInterface(TCPServer& server1, TCPServer& server2, 
                    ProtocolHandler& handler1, ProtocolHandler& handler2,
                    GlobalCollectState& collect_state);
    ~CommandInterface();
    
    // 启动命令行交互线程
    void start();
    
    // 停止命令行交互线程
    void stop();
    
    // 检查是否正在运行
    bool is_running() const { return m_running.load(); }
    
    // 获取全局运行状态引用
    std::atomic<bool>& get_global_running() { return m_global_running; }
    
    // 静态方法：显示帮助信息
    static void print_help();
    
    // 静态方法：显示服务器状态
    static void show_server_status(std::atomic<bool>& global_running);
    
    // 静态方法：分割命令行参数
    static std::vector<std::string> split_command(const std::string& input);
    
    // 静态方法：解析服务器标识符
    static int parse_server_id(const std::string& server_str);
    
    // 静态方法：向客户端发送数据包
    static bool send_to_client(int client_socket, TCPServer& server, ProtocolHandler& handler, 
                              const std::string& packet_type, const std::vector<std::string>& params,
                              int server_id, uint8_t ad_channel = 0);
    
    // 静态方法：发送采集IQ命令
    static bool send_collect_iq_command(int client_socket, TCPServer& server, ProtocolHandler& handler,
                                       uint32_t collect_time_ms, int server_id, uint8_t ad_channel = 0);

};

#endif // COMMAND_INTERFACE_H