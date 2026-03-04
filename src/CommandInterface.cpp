#include "CommandInterface.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <readline/readline.h>
#include <readline/history.h>
#include <thread>
#include "NetworkUtils.h"
#include <glog/logging.h>
#include <iomanip>
#include <ctime>

CommandInterface::CommandInterface(TCPServer& server1, TCPServer& server2,
                                   ProtocolHandler& handler1, ProtocolHandler& handler2,
                                   GlobalCollectState& collect_state)
    : m_server1(server1)
    , m_server2(server2)
    , m_handler1(handler1)
    , m_handler2(handler2)
    , m_collect_state(collect_state)
    , m_running(false)
    , m_global_running(true) {
}

CommandInterface::~CommandInterface() {
    stop();
}

void CommandInterface::start() {
    if (m_running) {
        return;
    }
    
    m_running = true;
    m_cli_thread = std::thread(&CommandInterface::command_line_interface, this);
    
    LOG(INFO) << "命令行交互线程已启动";
}

void CommandInterface::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    if (m_cli_thread.joinable()) {
        m_cli_thread.join();
    }
    
    LOG(INFO) << "命令行交互线程已停止";
}

void CommandInterface::print_help() {
    std::cout << "\n===================================== 服务器命令行帮助（八路AD版本） =====================================\n";
    std::cout << "说明：所有命令大小写不敏感\n";
    std::cout << "--------------------------------------------------------------------------------------------\n";
    std::cout << "基础命令：\n";
    std::cout << "  help                \t- 打印本帮助信息\n";
    std::cout << "  status              \t- 显示服务器和客户端运行状态\n";
    std::cout << "--------------------------------------------------------------------------------------------\n";
    std::cout << "数据发送命令（统一向AD0主通道发送）：\n";
    std::cout << "  send [server] [type] [params] \t- 向指定服务器的AD0发送数据包\n";
    std::cout << "  collect [server] [time_ms]    \t- 向指定服务器的AD0发送采集IQ命令\n";
    std::cout << "--------------------------------------------------------------------------------------------\n";
    std::cout << "参数说明：\n";
    std::cout << "  [server]  \t- 服务器标识：1/server1（192.168.2.170:8888）、2/server2（192.168.1.111:7777）\n";
    std::cout << "  [time_ms] \t- 采集时间，单位毫秒（1-10000）\n";
    std::cout << "  [type]    \t- 数据包类型，支持以下5种（带专属参数）：\n";
    std::cout << "              \t  ▶ cmd [choice] [freq] [len_type] - 命令包（业务控制指令）\n";
    std::cout << "              \t  ▶ ip                          - IP信息包（客户端/服务器IP同步）\n";
    std::cout << "              \t  ▶ iqstart [count] [rate] [freq] - IQ数据流启动包（指定采集参数）\n";
    std::cout << "              \t  ▶ iqend                       - IQ数据流结束包（停止采集）\n";
    std::cout << "              \t  ▶ iq [data_size]              - IQ数据包（模拟采集数据，指定数据长度）\n";
    std::cout << "============================================================================================\n";
    std::cout << "--------------------------------------------------------------------------------------------\n";
    std::cout << "常用命令示例：\n";
    std::cout << "  1. 查看服务器和客户端状态：\tstatus\n";
    std::cout << "  2. 向服务器1AD0发送命令：\tsend 1 cmd 1 2.4G 0\n";
    std::cout << "  3. 采集20ms数据：\t\tcollect 1 20\n";
    std::cout << "--------------------------------------------------------------------------------------------\n";
    std::cout << "采集IQ命令说明：\n";
    std::cout << "  1. 命令通过AD0（主通道）发送到客户端\n";
    std::cout << "  2. 客户端收到后回复ACK确认\n";
    std::cout << "  3. 客户端开始采集数据，预留数据池存放八路AD采集的数据\n";
    std::cout << "  4. 采集完成后，客户端通过AD0发送采集完成反馈\n";
    std::cout << "  5. 服务器收到反馈后，八路客户端各自发送对应通道的IQ数据\n";
    std::cout << "  6. 服务器将数据存入预留数据池\n";
    std::cout << "============================================================================================\n";
}


void CommandInterface::show_server_status(std::atomic<bool>& global_running) {
    // 显示客户端列表
    ClientManager::get_instance().print_clients();
    
    // 显示AD通道统计
    ClientManager::get_instance().print_server_status();
}

std::vector<std::string> CommandInterface::split_command(const std::string& input) {
    
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    // 从字符串流里，智能拆分每一个命令片段（无引号按空格拆，双引号包裹的内容整体算一个），能拆出一个就存一个，直到流里没有可拆分的内容为止。
    while (iss >> std::quoted(token)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

int CommandInterface::parse_server_id(const std::string& server_str) {
    if (server_str == "1" || server_str == "server1") {
        return 1;
    } else if (server_str == "2" || server_str == "server2") {
        return 2;
    }
    return -1; // 无效的服务器标识
}

// 发送采集IQ命令包
bool CommandInterface::send_collect_iq_command(int client_socket, TCPServer& server, ProtocolHandler& handler,
                                             uint32_t collect_time_ms, int server_id, uint8_t ad_channel) {
    try {
        if (collect_time_ms < 1 || collect_time_ms > 10000) {
            std::cout << "错误: 采集时间必须在1-10000毫秒范围内\n";
            return false;
        }
        
        // 创建采集IQ命令包
        ProtocolPacket collect_packet = handler.create_collect_iq_packet(
            handler.get_next_sequence_num(), collect_time_ms, ad_channel);
        
        if (server.send_packet(client_socket, collect_packet)) {
            std::cout << "已向客户端[AD" << (int)ad_channel 
                      << "]发送采集IQ命令: 采集时间=" << collect_time_ms << " ms" << std::endl;
            
            // 计算数据量信息
            double samples_per_ms = (1e6 / IQ_SAMPLE_PERIOD_NS) * IQ_SAMPLES_PER_CYCLE;
            uint32_t expected_samples = static_cast<uint32_t>(collect_time_ms * samples_per_ms);
            uint32_t data_pool_size = expected_samples * IQ_SAMPLE_BYTES;
            double total_data_gb = (data_pool_size * NUM_AD_CHANNELS) / (1024.0 * 1024.0 * 1024.0);
            
            std::cout << "数据量预估:\n";
            std::cout << "  每AD通道期望样本数: " << expected_samples << std::endl;
            std::cout << "  每AD通道数据池大小: " << data_pool_size << " 字节 ("
                      << static_cast<double>(data_pool_size) / (1024*1024) << " MB)" << std::endl;
            std::cout << "  八路AD总数据量: " << data_pool_size * NUM_AD_CHANNELS << " 字节 ("
                      << total_data_gb << " GB)" << std::endl;
            
            return true;
        }
    } catch (const std::exception& e) {
        std::cout << "发送采集IQ命令时发生错误: " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

bool CommandInterface::send_to_client(int client_socket, TCPServer& server, ProtocolHandler& handler, 
                                     const std::string& packet_type, const std::vector<std::string>& params,
                                     int server_id, uint8_t ad_channel) {
    try {
        if (packet_type == "cmd" && params.size() >= 3) {
            // 发送命令包
            uint8_t choice = static_cast<uint8_t>(std::stoi(params[0]));
            int32_t freq = std::stoi(params[1]);
            uint8_t len_type = static_cast<uint8_t>(std::stoi(params[2]));
            
            // 创建命令参数
            std::vector<uint8_t> cmd_payload(6);
            cmd_payload[0] = choice;
            
            int32_t net_freq = htonl(freq);
            std::memcpy(cmd_payload.data() + 1, &net_freq, sizeof(net_freq));
            cmd_payload[5] = len_type;
            
            // 创建命令包（固定向AD0发送）
            ProtocolPacket cmd_packet;
            cmd_packet.packet_type = PACKET_TYPE_COMMAND;
            cmd_packet.sequence_num = handler.get_next_sequence_num();
            cmd_packet.valid_length = static_cast<uint32_t>(cmd_payload.size());
            cmd_packet.payload = cmd_payload;
            cmd_packet.timestamp = static_cast<uint64_t>(time(nullptr));
            cmd_packet.ad_channel = ad_channel; // 使用传入的AD通道（通常为AD0）
            
            // 使用 send_packet 函数发送
            if (server.send_packet(client_socket, cmd_packet)) {
                std::cout << "已向服务器" << server_id << " AD" << (int)ad_channel 
                          << "发送命令包: choice=" << (int)choice 
                          << ", freq=" << freq << " Hz, len_type=" << (int)len_type << std::endl;
                return true;
            }
        }
        else if (packet_type == "ip") {
            // 获取服务器IP信息
            std::string server_ip;
            server_ip = (server_id == 1) ? SERVER_IP_1 : SERVER_IP_2;
            
            // 使用 NetworkUtils 获取IP信息
            IPInfo ip_info = NetworkUtils::get_server_ip_info(server_id, server_ip);
            ProtocolPacket ip_packet = handler.create_ip_info_packet(ip_info, ad_channel);
            
            if (server.send_packet(client_socket, ip_packet)) {
                std::cout << "已向服务器" << server_id << " AD" << (int)ad_channel 
                          << "发送IP信息包 (服务器" << server_id 
                          << " IP: " << ip_info.get_ip_string() 
                          << " 掩码: " << ip_info.get_netmask_string() << ")" << std::endl;
                return true;
            }
        }
        else if (packet_type == "iq" && params.size() >= 1) {
            // 发送IQ数据包（模拟数据）
            size_t data_size = std::stoi(params[0]);
            if (data_size > MAX_PAYLOAD_SIZE) {
                data_size = MAX_PAYLOAD_SIZE;
            }
            
            std::vector<uint8_t> iq_data(data_size);
            // 生成模拟IQ数据
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 255);
            
            for (size_t i = 0; i < data_size; i++) {
                iq_data[i] = static_cast<uint8_t>(dis(gen));
            }
            
            ProtocolPacket iq_packet = handler.create_iq_packet(
                handler.get_next_sequence_num(), time(nullptr), iq_data, ad_channel);
            
            if (server.send_packet(client_socket, iq_packet)) {
                std::cout << "已向服务器" << server_id << " AD" << (int)ad_channel 
                          << "发送IQ数据包: 大小=" << data_size << " 字节" << std::endl;
                return true;
            }
        }
        else {
            std::cout << "错误: 不支持的数据包类型或参数不足\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "发送数据包时发生错误: " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

int CommandInterface::find_client_socket_by_id(int server_id, int client_display_id) const {
    return ClientManager::get_instance().find_client_socket_by_display_id(server_id, client_display_id);
}

// 根据AD通道查找客户端socket（新增）
int CommandInterface::find_client_socket_by_ad_channel(int server_id, uint8_t ad_channel) const {
    return ClientManager::get_instance().find_client_socket_by_ad_channel(ad_channel, server_id);
}

int CommandInterface::get_active_clients_count(int server_id) const {
    return ClientManager::get_instance().get_server_client_count(server_id);
}

void CommandInterface::handle_send_command(const std::vector<std::string>& tokens) {
    try {
        // 解析服务器标识
        std::string server_str = tokens[1];
        int server_id = parse_server_id(server_str);
        if (server_id == -1) {
            std::cout << "错误: 无效的服务器标识 '" << server_str 
                      << "'。使用 1/server1 或 2/server2\n";
            return;
        }
        
        std::string packet_type = tokens[2];
        std::vector<std::string> params(tokens.begin() + 3, tokens.end());
        
        // 查找AD0的客户端socket
        int target_socket = find_client_socket_by_ad_channel(server_id, MASTER_CLIENT_CHANNEL);
        
        if (target_socket != -1) {
            // 根据服务器ID选择对应的服务器和处理器
            TCPServer* target_server = nullptr;
            ProtocolHandler* target_handler = nullptr;
            
            if (server_id == 1) {
                target_server = &m_server1;
                target_handler = &m_handler1;
            } else if (server_id == 2) {
                target_server = &m_server2;
                target_handler = &m_handler2;
            }
            
            // 尝试发送（固定向AD0发送）
            if (send_to_client(target_socket, *target_server, *target_handler, 
                               packet_type, params, server_id, MASTER_CLIENT_CHANNEL)) {
                std::cout << "发送成功 (服务器" << server_id << " AD0)\n";
            } else {
                std::cout << "发送失败 (服务器" << server_id << " AD0)\n";
            }
        } else {
            int client_count = get_active_clients_count(server_id);
            std::cout << "错误: 未找到服务器" << server_id << "的AD0客户端\n";
            std::cout << "该服务器当前共有 " << client_count << " 个活跃客户端，请确保AD0客户端已连接\n";
        }
    } catch (const std::exception& e) {
        std::cout << "命令解析错误: " << e.what() << std::endl;
        std::cout << "正确格式: send [server_id] [packet_type] [params]\n";
        std::cout << "示例: send 1 cmd 1 2400000000 1\n";
        std::cout << "示例: send 2 ip\n";
    }
}

// 处理采集IQ命令
void CommandInterface::handle_collect_command(const std::vector<std::string>& tokens) {
    try {
        // 解析服务器标识
        std::string server_str = tokens[1];
        int server_id = parse_server_id(server_str);
        if (server_id == -1) {
            std::cout << "错误: 无效的服务器标识 '" << server_str 
                      << "'。使用 1/server1 或 2/server2\n";
            return;
        }
        
        // 解析采集时间
        uint32_t collect_time_ms = std::stoi(tokens[2]);
        if (collect_time_ms < 1 || collect_time_ms > 20) {
            std::cout << "错误: 采集时间必须在1-20毫秒范围内\n";
            return;
        }

        // 重置全局采集状态
        m_collect_state.reset();
        m_collect_state.init_collection(collect_time_ms);
        
        // 查找AD0的客户端socket
        int target_socket = find_client_socket_by_ad_channel(server_id, MASTER_CLIENT_CHANNEL);
        
        if (target_socket != -1) {
            // 根据服务器ID选择对应的服务器和处理器
            TCPServer* target_server = nullptr;
            ProtocolHandler* target_handler = nullptr;
            
            if (server_id == 1) {
                target_server = &m_server1;
                target_handler = &m_handler1;
            } else if (server_id == 2) {
                target_server = &m_server2;
                target_handler = &m_handler2;
            }
            
            std::cout << "正在向服务器" << server_id << " AD0发送采集IQ命令...\n";
            
            // 发送采集IQ命令
            if (send_collect_iq_command(target_socket, *target_server, *target_handler, 
                                       collect_time_ms, server_id, MASTER_CLIENT_CHANNEL)) {
                std::cout << "采集IQ命令发送成功 (服务器" << server_id << ")\n";
                std::cout << "等待客户端ACK确认和采集完成反馈...\n";
                
                // 设置超时等待ACK
                auto start_time = std::chrono::steady_clock::now();
                bool ack_received = false;
                
                while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    // 检查是否有ACK已收到
                    if (m_collect_state.waiting_for_finish.load()) {
                        ack_received = true;
                        break;
                    }
                }
                
                if (ack_received) {
                    std::cout << "客户端已确认收到采集命令，开始采集...\n";
                    std::cout << "预计采集时间: " << collect_time_ms << " ms\n";
                    
                    // 等待采集完成反馈
                    start_time = std::chrono::steady_clock::now();
                    bool finish_received = false;
                    
                    while (std::chrono::steady_clock::now() - start_time < 
                           std::chrono::milliseconds(collect_time_ms + 50000)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        
                        {
                            std::lock_guard<std::mutex> lock(m_collect_state.mtx);
                            if (!m_collect_state.waiting_for_finish.load()) {
                                finish_received = true;
                                break;
                            }
                        }
                    }
                    
                    if (finish_received) {
                        std::cout << "采集完成反馈已收到，等待接收IQ数据...\n";
                    } else {
                        std::cout << "等待采集完成反馈超时\n";
                    }
                } else {
                    std::cout << "等待客户端ACK确认超时\n";
                }
            } else {
                std::cout << "采集IQ命令发送失败 (服务器" << server_id << ")\n";
            }
        } else {
            std::cout << "错误: 未找到服务器" << server_id << "的AD0客户端\n";
            std::cout << "请确保AD0客户端已连接，使用 'status' 命令查看连接状态\n";
        }
    } catch (const std::exception& e) {
        std::cout << "命令解析错误: " << e.what() << std::endl;
        std::cout << "正确格式: collect [server_id] [time_ms]\n";
        std::cout << "示例: collect 1 20 (向服务器1发送20ms采集命令)\n";
    }
}

void CommandInterface::command_line_interface() {
    std::cout << "\n服务器命令行交互模式已启动。输入 'help' 查看可用命令。\n";
    std::cout << "提示: 所有send命令将统一向AD0主通道发送\n";
    
    // 设置readline自动补全
    rl_bind_key('\t', rl_complete);
    
    while (m_running) {
        // 显示提示符
        std::string prompt = "server> ";
        char* input = readline(prompt.c_str());
        
        if (!input) {
            // Ctrl+D 退出
            std::cout << "\n检测到EOF，准备退出...\n";
            m_global_running = false;
            break;
        }
        
        std::string command(input);
        free(input);
        input = nullptr;
        
        // 跳过空命令
        if (command.empty()) {
            continue;
        }
        
        // 添加到历史记录
        add_history(command.c_str());
        
        // 转换为小写并分割命令
        std::string command_lower = command;
        std::transform(command_lower.begin(), command_lower.end(), command_lower.begin(), ::tolower);
        std::vector<std::string> tokens = split_command(command_lower);
        
        if (tokens.empty()) {
            continue;
        }
        
        std::string cmd = tokens[0];
        
        if (cmd == "help") {
            print_help();
        }
        else if (cmd == "status") {
            show_server_status(m_global_running);
        }
        else if (cmd == "send" && tokens.size() >= 3) {
            handle_send_command(tokens);
        }
        else if (cmd == "collect" && tokens.size() >= 3) {
            handle_collect_command(tokens);
        }
        else {
            std::cout << "未知命令: " << cmd << "，输入 'help' 查看可用命令\n";
        }
    }
    
    m_running = false;
    LOG(INFO) << "命令行交互线程已退出";
}