#include "TCPServer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <errno.h>
#include <vector>
#include <chrono>
#include "config.h"
#include <glog/logging.h>


TCPServer::TCPServer()
    : server_socket(-1), port(0), running(false), protocol_handler() {  // 初始化protocol_handler
}

TCPServer::~TCPServer() {
    stop();
}

// 【新增】发送数据包函数
bool TCPServer::send_packet(int client_socket, const ProtocolPacket& packet) {
    if (client_socket < 0) {
        return false;
    }

    // 编码数据包
    std::vector<uint8_t> encoded_data;
    if (!protocol_handler.encode_packet(packet, encoded_data)) {
        std::cerr << "编码数据包失败" << std::endl;
        return false;
    }

    // 发送数据
    return send_data(client_socket, 
                    reinterpret_cast<const char*>(encoded_data.data()), 
                    encoded_data.size());
}

// 【简化改造：直接加LOG(INFO)，通过日志时间戳看耗时，取消手动时间计算】
bool TCPServer::receive_packet(int client_socket, ProtocolPacket& packet, int timeout_ms) {
    if (client_socket < 0) {
        return false;
    }

    // 接收完整数据包
    std::vector<uint8_t> packet_buffer;
    if (!receive_complete_packet(client_socket, packet_buffer, timeout_ms)) {
        return false;
    }
    // 关键节点1：网络接收完整包完成（直接打日志，带时间戳）
    // LOG(INFO) << "【阶段1：网络接收完成】client_socket=" << client_socket 
    //           << " | 包原始大小=" << packet_buffer.size() << "字节";

    // 解码数据包
    if (!protocol_handler.decode_packet(packet_buffer, packet)) {
        LOG(ERROR) << "decode_packet 失败：客户端socket=" << client_socket 
                   << " | 原始包大小=" << packet_buffer.size() << "字节";
        return false;
    }
    // 关键节点2：数据包解码完成（直接打日志，带时间戳）
    // LOG(INFO) << "【阶段2：解码完成】client_socket=" << client_socket 
    //           << " | 包类型=" << (int)packet.packet_type
    //           << " | sequence_num=" << packet.sequence_num;

    return true;
}

bool TCPServer::start(const std::string& ip, int port) {
    this->ip_address = ip;
    this->port = port;

    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        return false;
    }

    // 设置服务器套接字选项
    if (!set_socket_options(server_socket)) {
        ::close(server_socket);
        return false;
    }

    // 设置为非阻塞
    if (!set_nonblocking(server_socket)) {
        ::close(server_socket);
        return false;
    }

    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        ::close(server_socket);
        return false;
    }

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        ::close(server_socket);
        return false;
    }

    // 开始监听
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen failed");
        ::close(server_socket);
        return false;
    }

    running = true;
    return true;
}

void TCPServer::stop() {
    if (server_socket >= 0) {
        ::close(server_socket);
        server_socket = -1;
    }
    running = false;
}

int TCPServer::accept_connection(int timeout_ms) {
    if (!running || server_socket < 0) {
        return -1;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int ready = select(server_socket + 1, &readfds, NULL, NULL, &timeout);

    if (ready > 0) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket,
                                 (struct sockaddr*)&client_addr,
                                 &client_len);

        if (client_socket >= 0) {
            // 设置客户端套接字选项
            set_socket_options(client_socket);

            // 设置为非阻塞
            set_nonblocking(client_socket);

            // 设置超时
            set_timeout(client_socket, 100, 100); // 100ms超时

            // LOG(INFO) << "接受新客户端连接: " 
            //           << inet_ntoa(client_addr.sin_addr) << ":"
            //           << ntohs(client_addr.sin_port);
        }

        return client_socket;
    }

    return ready; // 0表示超时，<0表示错误
}

bool TCPServer::send_data(int client_socket, const std::string& data) {
    return send_data(client_socket, data.c_str(), data.length());
}

bool TCPServer::send_data(int client_socket, const char* data, size_t length) {
    if (client_socket < 0) {
        return false;
    }

    size_t total_sent = 0;
    int retry_count = 0;
    const int MAX_RETRIES = 100;  // 最大重试次数
    const int RETRY_DELAY_US = 1000;  // 每次重试等待时间（微秒）

    while (total_sent < length) {
        ssize_t sent = send(client_socket, data + total_sent,
                           length - total_sent, 0);

        if (sent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 缓冲区满，等待后重试
                usleep(RETRY_DELAY_US);
                retry_count++;

                if (retry_count > MAX_RETRIES) {
                    std::cerr << "发送失败: 超过最大重试次数" << std::endl;
                    return false;
                }
                continue;
            }

            // 连接错误
            if (errno == ECONNRESET || errno == EPIPE || errno == ECONNABORTED) {
                return false;
            }

            perror("send failed");
            return false;
        }

        total_sent += sent;
        retry_count = 0;  // 成功发送后重置重试计数
    }

    return true;
}
bool TCPServer::receive_data(int client_socket, char* buffer, size_t buffer_size,
                           int timeout_ms, ssize_t* bytes_received) {
    if (client_socket < 0 || !buffer || buffer_size == 0) {
        return false;
    }

    // 使用select检查是否有数据
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(client_socket, &readfds);

    struct timeval timeout;

    // 无限等待
    if (timeout_ms < 0) {
        // 无限等待
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        int ready = select(client_socket + 1, &readfds, NULL, NULL, NULL); // 无限等待
        if (ready > 0) {
            ssize_t bytes_read = recv(client_socket, buffer, buffer_size, 0);
            if (bytes_received) {
                *bytes_received = bytes_read;
            }
            return bytes_read >= 0;
        }
    } else {
        // 有限超时
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        int ready = select(client_socket + 1, &readfds, NULL, NULL, &timeout);
        if (ready > 0) {
            ssize_t bytes_read = recv(client_socket, buffer, buffer_size, 0);
            if (bytes_received) {
                *bytes_received = bytes_read;
            }
            return bytes_read >= 0;
        }
    }

    if (bytes_received) {
        *bytes_received = 0;
    }

    return false; // 超时返回false
}

bool TCPServer::receive_packet_raw(int client_socket, std::vector<uint8_t>& packet_buffer,
                              size_t packet_size, int timeout_ms) {
    if (client_socket < 0 || packet_size == 0) {
        return false;
    }

    packet_buffer.resize(packet_size);

    size_t total_received = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (total_received < packet_size) {
        // 检查超时
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - start_time).count();

        if (elapsed > timeout_ms) {
            return false; // 超时
        }

        // 检查是否有数据可读
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        int ready = select(client_socket + 1, &readfds, NULL, NULL, &timeout);

        if (ready > 0) {
            ssize_t bytes_read = recv(client_socket,
                                     packet_buffer.data() + total_received,
                                     packet_size - total_received, 0);

            if (bytes_read > 0) {
                total_received += bytes_read;
            } else if (bytes_read == 0) {
                // 连接关闭
                return false;
            } else {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("recv failed");
                    return false;
                }
            }
        } else if (ready < 0) {
            if (errno != EINTR) {
                perror("select failed");
                return false;
            }
        }

        // 短暂休眠以避免CPU占用过高
        usleep(RECV_TIMEOUT_US);
    }

    return total_received == packet_size;
}

std::string TCPServer::get_client_info(int client_socket) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len) == 0) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        return std::string(client_ip) + ":" + std::to_string(client_port);
    }

    return "未知客户端";
}

bool TCPServer::get_client_address(int client_socket, std::string& ip, uint16_t& port) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len) == 0) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        ip = client_ip;
        port = ntohs(client_addr.sin_port);
        return true;
    }

    return false;
}

bool TCPServer::set_socket_options(int socket_fd) {
    int opt = 1;

    // 允许地址重用
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                  &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        return false;
    }

    // 设置TCP_NODELAY，禁用Nagle算法
    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY,
                  &opt, sizeof(opt)) < 0) {
        perror("setsockopt TCP_NODELAY failed");
        // 这不是致命错误，继续
    }

    // 设置发送缓冲区大小
    int send_buf_size = 1024 * 1024 * 512;  // 512MB
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF,
                  (const char*)&send_buf_size, sizeof(send_buf_size)) < 0) {
        perror("setsockopt SO_SNDBUF failed");
        // 这不是致命错误，继续
    }

    // 设置接收缓冲区大小
	int recv_buf_size = 1024 * 1024 * 512;  // 512MB
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF,
                  (const char*)&recv_buf_size, sizeof(recv_buf_size)) < 0) {
        perror("setsockopt SO_RCVBUF failed");
        // 这不是致命错误，继续
    }

    // 设置保活选项
    opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE,
                  &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_KEEPALIVE failed");
        // 这不是致命错误，继续
    }

    return true;
}

bool TCPServer::set_nonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return false;
    }

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        return false;
    }

    return true;
}

bool TCPServer::set_timeout(int socket_fd, int recv_timeout_ms, int send_timeout_ms) {
    struct timeval timeout;

    // 设置接收超时
    timeout.tv_sec = recv_timeout_ms / 1000;
    timeout.tv_usec = (recv_timeout_ms % 1000) * 1000;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                  (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO failed");
        // 这不是致命错误，继续
    }

    // 设置发送超时
    timeout.tv_sec = send_timeout_ms / 1000;
    timeout.tv_usec = (send_timeout_ms % 1000) * 1000;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO,
                  (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_SNDTIMEO failed");
        // 这不是致命错误，继续
    }
    
    return true;
}
bool TCPServer::receive_complete_packet(int client_socket,
                                       std::vector<uint8_t>& packet_buffer,
                                       int timeout_ms) {
    packet_buffer.resize(PACKET_SIZE);
    size_t total_received = 0;
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (total_received < PACKET_SIZE && running) {
        // 检查超时
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - start_time).count();
        
        if (timeout_ms > 0 && elapsed > timeout_ms) {
            std::cout << "接收数据包超时" << std::endl;
            return false;
        }
        
        // 接收剩余数据
        ssize_t bytes_received = 0;
        char* buffer_ptr = reinterpret_cast<char*>(packet_buffer.data() + total_received);
        size_t remaining = PACKET_SIZE - total_received;
        
        if (!receive_data(client_socket, buffer_ptr, remaining, 
                        100, &bytes_received)) {
            if (bytes_received == 0) {
                return false;
            }
            continue;
        }
        
        if (bytes_received > 0) {
            total_received += bytes_received;
            // LOG(INFO)  << "接收到 " << bytes_received << " 字节，总计 " 
            //          << total_received << "/" << PACKET_SIZE << " 字节" ;
        }
    }
    
    return total_received == PACKET_SIZE;
}