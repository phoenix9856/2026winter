#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "nlohmann/json.hpp"
#include <memory.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "EMPT/comm_recog_itf.h"
#include "CEMA/cema_api_struct_v0.3.h"
#include <iostream>
#include "class_loader/class_loader.hpp"
#include "EMPT/IQ.h"
#include "Logging.hpp"
#include "Python.h"
#include <glog/logging.h>
#include <chrono>
#include <fstream>
#include "thread"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <map>
#include <vector>
#include <algorithm>
#include <csignal>
#include <readline/readline.h>
#include <readline/history.h>
#include "TCPServer.h"
#include "ProtocolHandler.h"
#include "ClientManager.h"
#include "NetworkUtils.h"
#include "CommandInterface.h"
#include "GlobalCollectState.h"

// 全局运行状态
std::atomic<bool> g_running(true);

// 全局采集状态实例
GlobalCollectState g_collect_state;

// 客户端处理上下文
struct ClientContext {
    int socket_fd;
    std::string client_ip;
    uint16_t client_port;
    int server_id;
    std::string server_ip;
    uint8_t ad_channel;  // AD通道号
    uint32_t client_sequence_num; // 客户端序列号

    // 添加采集状态
    bool waiting_collect_ack;      // 是否在等待采集ACK
    bool collection_in_progress;   // 采集是否进行中
    
    // 信号处理器实例（每个客户端线程独立）
    std::unique_ptr<class_loader::ClassLoader> signal_detector_loader;
    boost::shared_ptr<comm_recog::comm_recog_itf> signal_detector;
    std::unique_ptr<class_loader::ClassLoader> signal_identify_separate_loader;
    boost::shared_ptr<comm_recog::comm_recog_itf> signal_identify_separate;
    std::unique_ptr<class_loader::ClassLoader> signal_identify_id_loader;
    boost::shared_ptr<comm_recog::comm_recog_itf> signal_identify_id;
    
    // 命令参数
    int choice;
    int sweep_start_frequency;
    int read_length_type;
    int start_subchannel;
    double end_subchannel;
    
    ClientContext(int fd, const std::string& ip, uint16_t port, int sid, 
                  const std::string& srv_ip, uint8_t ad_ch = 0)
        : socket_fd(fd), client_ip(ip), client_port(port), server_id(sid),
          server_ip(srv_ip), ad_channel(ad_ch), client_sequence_num(1),
          choice(0), sweep_start_frequency(0), read_length_type(1),
          start_subchannel(0), end_subchannel(16.0),
          waiting_collect_ack(false), collection_in_progress(false) {
    }
    
    ~ClientContext() {
        // 清理插件资源
        if (signal_detector) {
            signal_detector.reset();
        }
        if (signal_detector_loader) {
            signal_detector_loader->unloadLibrary();
        }
        if (signal_identify_separate) {
            signal_identify_separate.reset();
        }
        if (signal_identify_separate_loader) {
            signal_identify_separate_loader->unloadLibrary();
        }
        if (signal_identify_id) {
            signal_identify_id.reset();
        }
        if (signal_identify_id_loader) {
            signal_identify_id_loader->unloadLibrary();
        }
    }
};

// 获取当前时间戳（微秒）
static uint64_t get_current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

// 信号处理器类（重构为线程安全）
class SignalProcessor {
private:
    std::mutex mtx;
    
public:
    SignalProcessor() = default;
    
    std::vector<SignalInfo> detect(IQDataSampleInfo const & dataSample,
                                   std::unique_ptr<class_loader::ClassLoader>& loader,
                                   boost::shared_ptr<comm_recog::comm_recog_itf>& plugin) {
        std::lock_guard<std::mutex> lock(mtx);
        
        try {
            if (!loader) {
                loader = std::make_unique<class_loader::ClassLoader>("/opt/waveform/lib/libSignalDetection.so");
            }
            if (!plugin) {
                plugin = loader->createInstance<comm_recog::comm_recog_itf>("SignalDetection");
                if (!plugin) {
                    ISN_ERROR_LOG("create detect plugin failed, quit job");
                    throw std::runtime_error("create detect plugin failed, quit job");
                }
                plugin->initialize();
            }
        } catch (const std::exception& ex) {
            ISN_ERROR_LOG("request for identify exception:" << ex.what());
            return {};
        }
        
        FreqLibArray inputLib;
        TimeFreqFigure figure;
        IQDetectResult output;
        ISN_ERROR_LOG("start to detect");
        
        if (plugin->detectSignals(dataSample, inputLib, output, figure) != 0) {
            ISN_ERROR_LOG("detect signal error");
            return {};
        }
        
        ISN_ERROR_LOG("detect signal success");
        ISN_INFO_LOG("detect signal size:" << output.signalInfos.size());
        return output.signalInfos;
    }
    
    int identify(IQDataSampleInfo const & dataSample,
                 std::unique_ptr<class_loader::ClassLoader>& separate_loader,
                 boost::shared_ptr<comm_recog::comm_recog_itf>& separate_plugin,
                 std::unique_ptr<class_loader::ClassLoader>& id_loader,
                 boost::shared_ptr<comm_recog::comm_recog_itf>& id_plugin) {
        std::lock_guard<std::mutex> lock(mtx);
        
        try {
            if (!separate_loader) {
                separate_loader = std::make_unique<class_loader::ClassLoader>("/opt/waveform/lib/libDetectAndSperate.so");
            }
            if (!separate_plugin) {
                separate_plugin = separate_loader->createInstance<comm_recog::comm_recog_itf>("DetectAndSperate");
                if (separate_plugin == nullptr) {
                    std::cout << "end to construct data sample" << std::endl;
                    return -2;
                }
            }
            
            if (!id_loader) {
                id_loader = std::make_unique<class_loader::ClassLoader>("/opt/waveform/lib/libSignalIdentification.so");
            }
            if (!id_plugin) {
                id_plugin = id_loader->createInstance<comm_recog::comm_recog_itf>("SignalIdentificationImpl");
                if (id_plugin == nullptr) {
                    std::cout << "create Identification plugin failed, quit job" << std::endl;
                    return -2;
                }
                id_plugin->initialize();
            }
        } catch (const std::exception& ex) {
            std::cout << "request for identify exception:" << ex.what() << std::endl;
            return -1;
        }

        IQDetectResult sperate_output;  
        IQAfterDepartArray departArray;
        std::cout << "start to sperate" << std::endl;
        
        if (separate_plugin->seperateForIdentify(dataSample, &sperate_output, departArray) != 0) {
            std::cout << "sperate signal error" << std::endl;
            return -2;
        }
        
        std::cout << "sperate signal success with signal size:" << departArray.separatedSignals.size() << std::endl;

        IdentifyResult identifyResult;
        if (id_plugin->identifySignals(departArray, identifyResult.identifyResultComm, 
                                      identifyResult.identifyResultInterfere) != 0) {
            std::cout << "Identify signal error" << std::endl;
            return -2;
        }
        
        std::cout << "Identify signal success" << std::endl;
        return 0;
    }
};

// 处理单个客户端线程
void handle_client(TCPServer& server, int client_socket,
                   ProtocolHandler& protocol_handler, int server_id,
                   const std::string& server_bind_ip) {
    // 获取客户端IP和端口
    std::string client_ip;
    uint16_t client_port;
    if (!server.get_client_address(client_socket, client_ip, client_port)) {
        LOG(ERROR) << "[系统] 无法获取客户端地址，关闭连接";
        ::close(client_socket);
        return;
    }
    
    // 创建客户端上下文
    std::unique_ptr<ClientContext> ctx = std::make_unique<ClientContext>(
        client_socket, client_ip, client_port, server_id, server_bind_ip, 255);
    
    // 添加到客户端管理器
    ClientManager::get_instance().add_client(client_socket, client_ip, client_port, 
                                            server_id, server_bind_ip, 255);
    
    LOG(INFO) << "[系统] 新客户端连接: " << client_ip << ":" << client_port 
              << " (服务器ID: " << server_id << ")";
    
    // 信号处理器
    SignalProcessor signal_processor;

    // 主循环：接收数据包并处理
    while (g_running && client_socket >= 0) {
        // 接收数据包
        ProtocolPacket packet;
        if (!server.receive_packet(client_socket, packet, 1000)) {
            // 检查客户端是否仍然连接
            char buf[1];
            ssize_t n = recv(client_socket, buf, 1, MSG_PEEK | MSG_DONTWAIT);
            if (n == 0) {
                // 客户端断开连接
                LOG(INFO) << "客户端断开连接: " << client_ip << ":" << client_port;
                break;
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                // 连接错误
                LOG(ERROR) << "套接字错误: " << strerror(errno);
                break;
            }
            continue;
        }
        
        // LOG(INFO) << "接收到完整数据包: sequence_num=" << packet.sequence_num 
        //          << ", type=" << (int)packet.packet_type
        //          << ", ad_channel=" << (int)packet.ad_channel;
        
        // 检查AD通道是否变化，如果是第一次连接（AD通道为255）或AD通道发生变化，则更新上下文和客户端管理器中的AD通道信息
        if (ctx->ad_channel == 255 || ctx->ad_channel != packet.ad_channel) {
            uint8_t old_channel = ctx->ad_channel;
            ctx->ad_channel = packet.ad_channel;
            
            // 更新客户端管理器中的AD通道
            ClientManager::get_instance().update_client_ad_channel(client_socket, packet.ad_channel);
            
            LOG(INFO) << "AD通道更新: " << (int)old_channel << " -> " << (int)ctx->ad_channel 
                      << " (客户端: " << client_ip << ":" << client_port << ")";
        }
        
        // 同步客户端序列号
        ClientManager::get_instance().update_client_sequence_num(client_socket, packet.sequence_num);
        ctx->client_sequence_num = packet.sequence_num;
        
        // 根据数据包类型处理
        switch (packet.packet_type) {
            // 处理命令包
            case PACKET_TYPE_COMMAND: {
                if (packet.payload.size() >= 6) {
                    // 解析命令包
                    ctx->choice = packet.payload[0];
                    int32_t net_freq;
                    memcpy(&net_freq, &packet.payload[1], sizeof(net_freq));
                    ctx->sweep_start_frequency = ntohl(net_freq);
                    ctx->read_length_type = packet.payload[5];
                    
                    LOG(INFO) << "命令包解析完成: ad_channel=" << static_cast<int>(packet.ad_channel)
                            << ", choice=" << static_cast<int>(ctx->choice) 
                            << ", freq=" << ctx->sweep_start_frequency << " Hz"
                            << ", len_type=" << static_cast<int>(ctx->read_length_type);
                    
                    // 发送确认包
                    ProtocolPacket ack_packet = protocol_handler.create_ack_packet(
                        packet.sequence_num, true, packet.ad_channel);
                    if (server.send_packet(client_socket, ack_packet)) {
                        LOG(INFO) << "已发送确认包到AD通道" << (int)packet.ad_channel;
                    }
                }
                break;
            }
            // 处理IP信息包请求
            case PACKET_TYPE_IP_INFO: {
                LOG(INFO) << "收到客户端IP信息包请求，AD通道: " << (int)packet.ad_channel;
                IPInfo server_ip_info = NetworkUtils::get_interface_info(server_bind_ip);
                // 发送IP信息包（带AD通道号）
                auto new_ip_info_packet = protocol_handler.create_ip_info_packet(server_ip_info, packet.ad_channel);
                if (server.send_packet(client_socket, new_ip_info_packet)) {
                    LOG(INFO) << "发送IP信息包成功到AD通道" << (int)packet.ad_channel;
                }
                break;
            }
            // 处理采集IQ命令的ACK包
            case PACKET_TYPE_ACK: {
                if (packet.ad_channel == MASTER_CLIENT_CHANNEL && packet.payload.size() >= 5) {
                    uint32_t ack_seq_num;
                    std::memcpy(&ack_seq_num, packet.payload.data(), 4);
                    ack_seq_num = ntohl(ack_seq_num);
                    bool success = (packet.payload[4] == 1);
                    
                    LOG(INFO) << "收到ACK包，序列号: " << ack_seq_num 
                             << ", 状态: " << (success ? "成功" : "失败")
                             << ", AD通道: " << (int)packet.ad_channel;
                    
                    // 如果是采集IQ命令的ACK
                    if (success && g_collect_state.waiting_for_finish) {
                        LOG(INFO) << "采集IQ命令ACK确认收到，客户端开始采集...";
                        ctx->collection_in_progress = true;
                    }
                }
                break;
            }
            // 处理采集完成反馈包
            case PACKET_TYPE_COLLECT_FINISH: {
                LOG(INFO) << "收到采集完成反馈包，AD通道: " << (int)packet.ad_channel
                         << ", sequence_num: " << packet.sequence_num;
                
                if (packet.ad_channel == MASTER_CLIENT_CHANNEL) {
                    // 只有主通道AD0发送采集完成反馈
                    ctx->collection_in_progress = false;
                    // 通知全局采集状态
                    {
                        std::lock_guard<std::mutex> lock(g_collect_state.mtx);
                        for (auto& pair : g_collect_state.ad_data_pools) {
                            pair.second.ready_for_data = true;
                        }
                        g_collect_state.waiting_for_finish = false;
                    }
                        
                    LOG(INFO) << "收到客户端采集完成反馈，开始接收IQ数据...\n";
                }
                
                // 发送确认包
                ProtocolPacket ack_packet = protocol_handler.create_ack_packet(
                    packet.sequence_num, true, packet.ad_channel);
                if (server.send_packet(client_socket, ack_packet)) {
                    LOG(INFO) << "已发送采集完成反馈的ACK包到AD通道" << (int)packet.ad_channel;
                }
                break;
            }
            // 处理IQ数据包
            case PACKET_TYPE_IQ_DATA: {
                // 检查是否在采集过程中
                if (g_collect_state.ad_data_pools.find(packet.ad_channel) != 
                    g_collect_state.ad_data_pools.end() && 
                    g_collect_state.ad_data_pools[packet.ad_channel].ready_for_data) {
                    // 这是采集数据，添加到数据池
                    bool success = g_collect_state.add_iq_data(
                        packet.ad_channel, packet.payload.data(), packet.payload.size());
                } else {
                    // 如果不是采集数据，按常规处理
                    LOG(WARNING) << "收到IQ数据包，但AD通道" << (int)packet.ad_channel 
                                << "未准备好接收数据";
                    
                    // 如果需要处理信号
                    if (ctx->choice > 0) {
                        // 计算读取长度
                        size_t read_len = 8 * 1024 * 1024; // 默认8M
                        switch (ctx->read_length_type) {
                            case 1: read_len = 8 * 1024 * 1024; break;
                            case 2: read_len = 64 * 1024 * 1024; break;
                            case 3: read_len = 512 * 1024 * 1024; break;
                        }
                        
                        // 为每个子信道处理
                        for (int j = ctx->start_subchannel; j < static_cast<int>(ctx->end_subchannel); j++) {
                            IQDataSampleInfo dataSample;
                            
                            // 计算有效IQ样本数：I(2字节)+Q(2字节)，单样本4字节
                            const size_t IQ_SAMPLE_BYTES = 4;
                            size_t num_samples = packet.payload.size() / IQ_SAMPLE_BYTES;
                            dataSample.data.aIQDataArray.resize(num_samples);
                            
                            // 解析纯IQ数据：交替I/Q，各2字节int16_t
                            for (size_t n = 0; n < num_samples; n++) {
                                int16_t I = *reinterpret_cast<const int16_t*>(&packet.payload[n * IQ_SAMPLE_BYTES]);
                                int16_t Q = *reinterpret_cast<const int16_t*>(&packet.payload[n * IQ_SAMPLE_BYTES + 2]);
                                dataSample.data.aIQDataArray[n].IData = I;
                                dataSample.data.aIQDataArray[n].QData = Q;
                            }
                            
                            dataSample.sampleFreq = 75 * 1e6;
                            dataSample.radioFreq = (ctx->sweep_start_frequency + 262.5 - 37.5 * j) * 1e6;
                            
                            LOG(INFO) << "Processing channel " << j 
                                    << ", AD通道: " << (int)packet.ad_channel
                                    << ", center freq: " << dataSample.radioFreq
                                    << ", sample count: " << num_samples;
                            
                            // 根据choice执行相应功能
                            switch (ctx->choice) {
                                case 1: { // 检测
                                    auto detect_result = signal_processor.detect(
                                        dataSample, ctx->signal_detector_loader, ctx->signal_detector);
                                    if (detect_result.size() > 0) {
                                        LOG(INFO) << "AD通道" << (int)packet.ad_channel 
                                                 << " 检测到 " << detect_result.size() 
                                                 << " 个信号, sequence num: " << packet.sequence_num;
                                    } else {
                                        LOG(INFO) << "AD通道" << (int)packet.ad_channel 
                                                 << " 未检测到信号, sequence num: " << packet.sequence_num;
                                    }
                                    break;
                                }
                                case 2: { // 识别通信信号
                                    dataSample.sampleBand = 37.5 * 1e6;
                                    dataSample.signalType = Communication;
                                    signal_processor.identify(dataSample, 
                                        ctx->signal_identify_separate_loader, ctx->signal_identify_separate,
                                        ctx->signal_identify_id_loader, ctx->signal_identify_id);
                                    break;
                                }
                                case 3: { // 识别干扰信号
                                    dataSample.sampleBand = 37.5 * 1e6;
                                    dataSample.signalType = Interference;
                                    signal_processor.identify(dataSample,
                                        ctx->signal_identify_separate_loader, ctx->signal_identify_separate,
                                        ctx->signal_identify_id_loader, ctx->signal_identify_id);
                                    break;
                                }
                                default:
                                    LOG(WARNING) << "AD通道" << (int)packet.ad_channel 
                                                << " 未知choice: " << ctx->choice;
                                    break;
                            }
                        }
                    }
                }
                break;
            }
            default:
                LOG(WARNING) << "未知包类型: " << (int)packet.packet_type 
                           << ", AD通道: " << (int)packet.ad_channel;
                break;
        }
    }
    
    // 从客户端连接列表中移除
    ClientManager::get_instance().update_client_status(client_socket, "disconnected", false);
    
    // 清理客户端
    ::close(client_socket);
    LOG(INFO) << "Client handler exited for server" << server_id 
              << ": " << client_ip << ":" << client_port
              << " [AD" << (int)ctx->ad_channel << "]";
    std::cout << "\n[系统] 服务器" << server_id << "的客户端断开连接: " 
              << client_ip << ":" << client_port 
              << " [AD" << (int)ctx->ad_channel << "]" << std::endl;
}

// 服务器运行函数
void run_server(TCPServer& server, ProtocolHandler& protocol_handler, 
                const std::string& ip, int port, int server_id) {
    if (!server.start(ip, port)) {
        LOG(ERROR) << "[系统] 服务器" << server_id << "启动失败在 " << ip << ":" << port;
        return;
    }
    LOG(INFO) << "[系统] 服务器" << server_id << "已启动在 " << ip << ":" << port;
    
    while (g_running) {
        // 接受新连接
        int client_socket = server.accept_connection(1000);
        
        if (client_socket > 0) {
            // 为新客户端创建处理线程
            std::thread client_thread([&, client_socket]() {
                handle_client(server, client_socket, protocol_handler, server_id, ip);
            });
            client_thread.detach();
        } else if (client_socket < 0) {
            LOG(ERROR) << "服务器" << server_id << "接受连接失败: " << strerror(errno);
        }
        
        // 短暂休眠以避免CPU占用过高
        usleep(10000);
    }
    
    server.stop();
    LOG(INFO) << "[系统] 服务器" << server_id << "已停止";
}

// 信号处理函数
void signal_handler(int signum) {
    std::cout << "\n[系统] 收到信号 " << signum << "，正在优雅退出...\n";
    g_running = false;
}

int main(int argc, char* argv[]) {
    // 初始化Google日志库
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    FLAGS_colorlogtostderr = 1;

    // 注册信号处理
    signal(SIGINT, signal_handler);     // 处理Ctrl+C
    signal(SIGTERM, signal_handler);    // 处理kill命令
    
    // 输出欢迎信息
    std::cout << "========= 香橙派AIPRO 20T智能电磁信号处理器（八路AD版本） =========\n";
    std::cout << "服务器1: " << SERVER_IP_1 << ":" << SERVER_PORT_1 << "\n";
    std::cout << "服务器2: " << SERVER_IP_2 << ":" << SERVER_PORT_2 << "\n";
    std::cout << "采集IQ命令: collect [server_id] [time_ms]\n";
    std::cout << "示例: collect 1 20 (向服务器1发送20ms采集命令)\n";
    std::cout << "输入 'help' 查看命令\n";
    
    // 创建实例
    TCPServer server1, server2;         // 两个服务器实例
    ProtocolHandler handler1, handler2;     // 两个协议处理器实例
    CommandInterface cmd_interface(server1, server2, handler1, handler2, g_collect_state);      // 命令行接口实例

    // 启动线程
    std::thread server2_thread([&]() {
        run_server(server2, handler2, SERVER_IP_2, SERVER_PORT_2, 2);       // 启动服务器2线程
    });
    // std::thread server1_thread([&]() {
    //     run_server(server1, handler1, SERVER_IP_1, SERVER_PORT_1, 1);
    // });
    cmd_interface.start();      // 启动命令行接口线程
     
    // 等待终止信号
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 定期检查采集状态
        static uint64_t last_check_time = 0;
        uint64_t current_time = get_current_time_us();
        
        if (current_time - last_check_time > 50000000) { // 每50秒检查一次
            if (g_collect_state.all_data_received) {
                // 所有数据接收完成，可以重置状态
                g_collect_state.reset();
                LOG(INFO) << "采集状态已重置";
            }
            last_check_time = current_time;
        }
    }
    
    // 收到终止信号，清理资源
    LOG(INFO) << "[系统] 正在关闭服务器...";
    auto all_clients = ClientManager::get_instance().get_all_clients();
    for (const auto& client : all_clients) {
        if (client.is_active) {
            close(client.socket_fd);
            ClientManager::get_instance().update_client_status(
                client.socket_fd, "server shutdown", false);
        }
    }
    cmd_interface.stop();
    // if (server1_thread.joinable()) {
    //     server1_thread.join();
    // }
    if (server2_thread.joinable()) {
        server2_thread.join();
    }
    LOG(INFO) << "[系统] 服务器已关闭，程序退出";
    
    return 0;
}