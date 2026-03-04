#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <vector>
#include <cstdint>
#include <string>
#include "config.h"
#include <cstring>
#include <iostream>
#include <endian.h>

class ProtocolPacket {
public:
    uint8_t packet_type;                 // 包类型
    uint32_t sequence_num;               // 序列号
    uint32_t valid_length;               // 有效载荷长度
    std::vector<uint8_t> payload;        // 有效载荷
    uint16_t crc;                        // CRC校验值
    bool crc_valid;                      // CRC校验是否有效
    bool error_flag;                     // 错误标志
    uint32_t timestamp;                  // 时间戳
    uint8_t ad_channel;                  // AD通道类型

    // 采集IQ信息（新增）
    struct CollectIQInfo {
        uint32_t collect_time_ms;        // 采集时间（毫秒）
        uint32_t expected_samples;       // 期望样本数（每个AD通道）
        uint32_t data_pool_size;         // 数据池大小（字节，每个AD通道）
        bool collection_finished;        // 采集是否完成
        uint32_t finish_timestamp;       // 采集完成时间戳
    } collect_iq_info;

};

// IP信息结构体
struct IPInfo {
    uint8_t ip_bytes[4];      // IP地址字节数组
    uint8_t netmask_bytes[4]; // 子网掩码字节数组
    uint32_t timestamp;       // 时间戳（相对1970.1.1的秒数）
    
    IPInfo() {
        memset(ip_bytes, 0, sizeof(ip_bytes));
        memset(netmask_bytes, 0, sizeof(netmask_bytes));
        timestamp = 0;
    }
    
    // 从字符串设置IP地址
    void set_ip(const std::string& ip_str) {
        int a, b, c, d;
        if (sscanf(ip_str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            ip_bytes[0] = static_cast<uint8_t>(a);
            ip_bytes[1] = static_cast<uint8_t>(b);
            ip_bytes[2] = static_cast<uint8_t>(c);
            ip_bytes[3] = static_cast<uint8_t>(d);
        }
    }
    
    // 从字符串设置子网掩码
    void set_netmask(const std::string& netmask_str) {
        int a, b, c, d;
        if (sscanf(netmask_str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            netmask_bytes[0] = static_cast<uint8_t>(a);
            netmask_bytes[1] = static_cast<uint8_t>(b);
            netmask_bytes[2] = static_cast<uint8_t>(c);
            netmask_bytes[3] = static_cast<uint8_t>(d);
        }
    }
    
    // 设置当前时间戳
    void set_current_timestamp() {
        timestamp = static_cast<uint32_t>(time(nullptr));
    }
    
    // 获取IP字符串
    std::string get_ip_string() const {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d",
                ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
        return std::string(buffer);
    }
    
    // 获取子网掩码字符串
    std::string get_netmask_string() const {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d",
                netmask_bytes[0], netmask_bytes[1], 
                netmask_bytes[2], netmask_bytes[3]);
        return std::string(buffer);
    }
    
    // 获取时间戳字符串
    std::string get_timestamp_string() const {
        char buffer[32];
        time_t t = static_cast<time_t>(timestamp);
        struct tm* tm_info = localtime(&t);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        return std::string(buffer);
    }
};

// 数据池管理结构体（新增）
struct DataPool {
    uint8_t ad_channel;                   // AD通道号
    uint32_t expected_samples;            // 期望样本数
    uint32_t received_samples;            // 已接收样本数
    uint32_t data_pool_size;              // 数据池大小（字节）
    std::vector<uint8_t> iq_data;         // IQ数据存储
    bool ready_for_data;                  // 是否准备好接收数据
    bool data_complete;                   // 数据是否接收完成
    
    DataPool(uint8_t channel = 0, uint32_t expected = 0) 
        : ad_channel(channel), expected_samples(expected),
          received_samples(0), data_pool_size(0),
          ready_for_data(false), data_complete(false) {
        if (expected_samples > 0) {
            data_pool_size = expected_samples * IQ_SAMPLE_BYTES;
            iq_data.resize(data_pool_size, 0);
        }
    }
    
    // 重置数据池
    void reset() {
        received_samples = 0;
        ready_for_data = false;
        data_complete = false;
        if (!iq_data.empty()) {
            memset(iq_data.data(), 0, iq_data.size());
        }
    }
    
    // 添加IQ数据
    bool add_iq_data(const uint8_t* data, uint32_t size) {
        if (received_samples * IQ_SAMPLE_BYTES + size > data_pool_size) {
            return false; // 数据池已满
        }
        
        memcpy(iq_data.data() + received_samples * IQ_SAMPLE_BYTES, data, size);
        received_samples += size / IQ_SAMPLE_BYTES;
        
        if (received_samples >= expected_samples) {
            data_complete = true;
        }
        
        return true;
    }
    
    // 检查数据是否完整
    bool is_complete() const {
        return data_complete;
    }
    
    // 获取数据使用率
    double get_usage_percentage() const {
        if (expected_samples == 0) return 0.0;
        return (static_cast<double>(received_samples) / expected_samples) * 100.0;
    }
};

class ProtocolHandler {
public:
    ProtocolHandler();

    // 计算CRC16
    static uint16_t compute_crc16(const uint8_t* data, size_t length);

    // 编码数据包
    bool encode_packet(const ProtocolPacket& packet, std::vector<uint8_t>& output);

    // 解码数据包
    bool decode_packet(const std::vector<uint8_t>& input, ProtocolPacket& packet);

    // 创建数据包
    ProtocolPacket create_packet(uint8_t type, const std::vector<uint8_t>& payload, 
                                 uint8_t ad_channel = 0, bool error_flag = false);

    // 创建命令包
    ProtocolPacket create_command_packet(uint8_t command, const std::vector<uint8_t>& params, 
                                         uint8_t ad_channel = 0);

    // 创建IQ数据包
    ProtocolPacket create_iq_packet(uint32_t sequence_num, uint64_t timestamp, 
                                    const std::vector<uint8_t>& iq_data, uint8_t ad_channel = 0);

    // 创建确认包
    ProtocolPacket create_ack_packet(uint32_t ack_sequence_num, bool success = true, 
                                     uint8_t ad_channel = 0);
    
    // 创建IP信息包
    ProtocolPacket create_ip_info_packet(const IPInfo& ip_info, uint8_t ad_channel = 0);

    // 创建IQ_START包
    ProtocolPacket create_iq_start_packet(uint32_t seq_num, uint32_t total_packets, 
                                         uint32_t sample_rate, uint64_t center_frequency,
                                         uint32_t packet_size, uint64_t start_timestamp,
                                         uint8_t ad_channel = 0);
    
    // 创建IQ_END包
    ProtocolPacket create_iq_end_packet(uint32_t seq_num, uint64_t end_timestamp,
                                        uint8_t ad_channel = 0);
    
    // 创建采集IQ命令包（新增）
    ProtocolPacket create_collect_iq_packet(uint32_t seq_num, uint32_t collect_time_ms,
                                           uint8_t ad_channel = 0);
    
    // 创建采集完成反馈包（新增）
    ProtocolPacket create_collect_finish_packet(uint32_t seq_num, bool success,
                                               uint32_t finish_timestamp,
                                               uint8_t ad_channel = 0);

    // 验证数据包
    bool validate_packet(const ProtocolPacket& packet);

    // 打印数据包信息
    void print_packet_info(const ProtocolPacket& packet, const std::string& prefix = "");

    // 获取下一个序列号
    uint32_t get_next_sequence_num() {
        return next_sequence_num++;
    }

    // 设置序列号（用于同步客户端序列号）
    void set_sequence_num(uint32_t seq) { next_sequence_num = seq; }

private:
    uint32_t next_sequence_num;

    uint16_t calculate_crc(const uint8_t* data, size_t length) const;
};

#endif // PROTOCOL_HANDLER_H