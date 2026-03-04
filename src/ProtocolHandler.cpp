#include "ProtocolHandler.h"
#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>
#include <ctime>
#include <glog/logging.h>
#include <endian.h>

ProtocolHandler::ProtocolHandler() : next_sequence_num(1) {
}

uint16_t ProtocolHandler::calculate_crc(const uint8_t* data, size_t length) const {
    uint16_t crc = 0xFFFF;
    uint16_t polynomial = 0x1021;

    for (size_t i = 0; i < length; ++i) {
        crc ^= (static_cast<uint16_t>(data[i]) << 8);

        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

uint16_t ProtocolHandler::compute_crc16(const uint8_t* data, size_t length) {
    ProtocolHandler handler;
    return handler.calculate_crc(data, length);
}

bool ProtocolHandler::encode_packet(const ProtocolPacket& packet, std::vector<uint8_t>& output) {
    // 检查有效载荷大小
    if (packet.payload.size() > MAX_PAYLOAD_SIZE) {
        std::cerr << "有效载荷太大: " << packet.payload.size()
                  << " > " << MAX_PAYLOAD_SIZE << std::endl;
        return false;
    }

    output.resize(PACKET_SIZE, 0);
    uint32_t position = 0;

    // 新帧头格式：4字节时间戳+4字节序列号+1字节包类型+1字节AD通道+4字节有效长度
    
    // 填充时间戳（4字节，网络字节序）
    uint32_t net_timestamp = htonl((uint32_t)packet.timestamp);
    std::memcpy(output.data() + position, &net_timestamp, sizeof(net_timestamp));
    position += 4;
    
    // 填充序列号（4字节，网络字节序）
    uint32_t net_seq_num = htonl(packet.sequence_num);
    std::memcpy(output.data() + position, &net_seq_num, sizeof(net_seq_num));
    position += 4;
    
    // 填充包类型（1字节）
    output[position++] = packet.packet_type;
    
    // 填充AD通道（1字节）
    output[position++] = packet.ad_channel;
    
    // 填充有效载荷长度（4字节，网络字节序）
    uint32_t net_valid_length = htonl(packet.valid_length);
    std::memcpy(output.data() + position, &net_valid_length, sizeof(net_valid_length));
    position += 4;
    
    // 现在position = 14，正好是HEADER_SIZE

    // 填充有效载荷
    if (packet.valid_length > 0) {
        std::memcpy(output.data() + HEADER_SIZE, packet.payload.data(),
                   std::min(packet.valid_length, static_cast<uint32_t>(packet.payload.size())));
    }

    // 计算CRC（对整个数据包除CRC部分）
    uint16_t crc = calculate_crc(output.data(), PACKET_SIZE - TRAILER_SIZE);

    // 填充帧尾: CRC（网络字节序）
    uint16_t net_crc = htons(crc);
    std::memcpy(output.data() + PACKET_SIZE - TRAILER_SIZE, &net_crc, sizeof(net_crc));

    return true;
}

bool ProtocolHandler::decode_packet(const std::vector<uint8_t>& input, ProtocolPacket& packet) {
    // 基本长度检查
    if (input.size() < PACKET_SIZE) {
        std::cerr << "数据包大小不足: " << input.size() << " < " << PACKET_SIZE << std::endl;
        return false;
    }

    uint32_t position = 0;
    
    // 提取时间戳（网络->主机字节序）
    uint32_t net_timestamp;
    std::memcpy(&net_timestamp, input.data() + position, sizeof(net_timestamp));
    packet.timestamp = ntohl(net_timestamp);
    position += 4;
    
    // 提取序列号（网络->主机字节序）
    uint32_t net_seq_num;
    std::memcpy(&net_seq_num, input.data() + position, sizeof(net_seq_num));
    packet.sequence_num = ntohl(net_seq_num);
    position += 4;
    
    // 提取包类型（1字节）
    packet.packet_type = input[position++];
    
    // 提取AD通道（1字节）
    packet.ad_channel = input[position++];
    
    // 提取有效载荷长度（网络->主机字节序，4字节）
    uint32_t net_valid_length;
    std::memcpy(&net_valid_length, input.data() + position, sizeof(net_valid_length));
    packet.valid_length = ntohl(net_valid_length);
    position += 4;

    // 检查有效长度
    if (packet.valid_length > MAX_PAYLOAD_SIZE) {
        std::cerr << "无效的有效长度: " << packet.valid_length << std::endl;
        return false;
    }

    // 提取有效载荷
    packet.payload.resize(packet.valid_length);
    if (packet.valid_length > 0) {
        std::memcpy(packet.payload.data(), input.data() + HEADER_SIZE, packet.valid_length);
    }
  
    // 提取帧尾: CRC
    uint16_t net_crc;
    std::memcpy(&net_crc, input.data() + PACKET_SIZE - TRAILER_SIZE, sizeof(net_crc));
    packet.crc = ntohs(net_crc);

    // 验证CRC（计算除CRC部分的数据）
    uint16_t calculated_crc = calculate_crc(input.data(), PACKET_SIZE - TRAILER_SIZE);
    packet.crc_valid = (calculated_crc == packet.crc);
    packet.error_flag = false;

    if (!packet.crc_valid) {
        std::cerr << "CRC校验失败: 接收=" << packet.crc
                  << " 计算=" << calculated_crc << std::endl;
        packet.error_flag = true;
    }

    // 解码采集IQ命令包
    if (packet.packet_type == PACKET_TYPE_COLLECT_IQ && packet.valid_length >= COLLECT_IQ_PACKET_SIZE) {
        // 提取采集时间（网络->主机字节序）
        uint32_t net_collect_time;
        std::memcpy(&net_collect_time, packet.payload.data(), 4);
        packet.collect_iq_info.collect_time_ms = ntohl(net_collect_time);
        
        // 计算期望样本数和数据池大小
        double samples_per_ms = (1e6 / IQ_SAMPLE_PERIOD_NS) * IQ_SAMPLES_PER_CYCLE;
        packet.collect_iq_info.expected_samples = static_cast<uint32_t>(
            packet.collect_iq_info.collect_time_ms * samples_per_ms);
        packet.collect_iq_info.data_pool_size = packet.collect_iq_info.expected_samples * IQ_SAMPLE_BYTES;
        packet.collect_iq_info.collection_finished = false;
        
        LOG(INFO) << "解码采集IQ命令包[AD" << (int)packet.ad_channel << "] - "
                 << "采集时间: " << packet.collect_iq_info.collect_time_ms << " ms, "
                 << "期望样本数: " << packet.collect_iq_info.expected_samples << ", "
                 << "数据池大小: " << packet.collect_iq_info.data_pool_size << " 字节";
    }
    
    // 解码采集完成反馈包
    if (packet.packet_type == PACKET_TYPE_COLLECT_FINISH && packet.valid_length >= 5) {
        // 提取状态和完成时间戳
        packet.collect_iq_info.collection_finished = (packet.payload[0] == 1);
        
        uint32_t net_finish_time;
        std::memcpy(&net_finish_time, packet.payload.data() + 1, 4);
        packet.collect_iq_info.finish_timestamp = ntohl(net_finish_time);
        
        LOG(INFO) << "解码采集完成反馈包[AD" << (int)packet.ad_channel << "] - "
                 << "状态: " << (packet.collect_iq_info.collection_finished ? "成功" : "失败") << ", "
                 << "完成时间: " << packet.collect_iq_info.finish_timestamp;
    }

    // 添加调试日志
    // LOG(INFO) << "解码数据包[AD" << (int)packet.ad_channel << "]:";
    // LOG(INFO) << "  时间戳: " << packet.timestamp << " (" 
    //           << std::ctime(reinterpret_cast<const time_t*>(&packet.timestamp)) << ")";
    // LOG(INFO) << "  序列号: " << std::dec << packet.sequence_num;
    // LOG(INFO) << "  包类型: 0x" << std::hex << (int)packet.packet_type;
    // LOG(INFO) << "  AD通道: " << (int)packet.ad_channel;
    // LOG(INFO) << "  有效长度: " << packet.valid_length;
    // LOG(INFO) << "  CRC有效: " << packet.crc_valid;


    return true;
}

ProtocolPacket ProtocolHandler::create_packet(uint8_t type, const std::vector<uint8_t>& payload,
                                              uint8_t ad_channel, bool error_flag) {
    ProtocolPacket packet;
    packet.packet_type = type;
    packet.sequence_num = get_next_sequence_num();
    packet.valid_length = static_cast<uint32_t>(payload.size());
    packet.payload = payload;
    packet.error_flag = error_flag;
    packet.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.ad_channel = ad_channel;

    return packet;
}

ProtocolPacket ProtocolHandler::create_command_packet(uint8_t command, const std::vector<uint8_t>& params,
                                                      uint8_t ad_channel) {
    std::vector<uint8_t> payload;
    payload.push_back(command);  // 命令类型

    // 添加参数
    payload.insert(payload.end(), params.begin(), params.end());

    return create_packet(PACKET_TYPE_COMMAND, payload, ad_channel);
}

ProtocolPacket ProtocolHandler::create_iq_packet(uint32_t sequence_num, uint64_t timestamp,
                                                 const std::vector<uint8_t>& iq_data, uint8_t ad_channel) {
    ProtocolPacket packet;
    packet.packet_type = PACKET_TYPE_IQ_DATA;
    packet.sequence_num = sequence_num;
    packet.valid_length = static_cast<uint32_t>(iq_data.size());
    packet.payload = iq_data;
    packet.timestamp = timestamp;
    packet.ad_channel = ad_channel;

    return packet;
}

ProtocolPacket ProtocolHandler::create_ack_packet(uint32_t ack_sequence_num, bool success, uint8_t ad_channel) {
    std::vector<uint8_t> payload(5); // 4字节序列号 + 1字节状态
    uint32_t net_seq = htonl(ack_sequence_num);
    std::memcpy(payload.data(), &net_seq, sizeof(net_seq));
    payload[4] = success ? 1 : 0;

    ProtocolPacket packet;
    packet.packet_type = PACKET_TYPE_ACK;
    packet.sequence_num = get_next_sequence_num();
    packet.valid_length = static_cast<uint32_t>(payload.size());
    packet.payload = payload;
    packet.ad_channel = ad_channel;

    return packet;
}

// 创建IP信息包
ProtocolPacket ProtocolHandler::create_ip_info_packet(const IPInfo& ip_info, uint8_t ad_channel) {
    std::vector<uint8_t> payload(12); // 4字节IP + 4字节子网掩码 + 4字节时间戳 = 12字节
    
    // 填充IP地址字节
    std::memcpy(payload.data(), ip_info.ip_bytes, 4);
    
    // 填充子网掩码字节
    std::memcpy(payload.data() + 4, ip_info.netmask_bytes, 4);
    
    // 填充时间戳（网络字节序）
    uint32_t net_timestamp = htonl(ip_info.timestamp);
    std::memcpy(payload.data() + 8, &net_timestamp, 4);
    
    // 创建包
    ProtocolPacket packet;
    packet.packet_type = PACKET_TYPE_IP_INFO;
    packet.sequence_num = get_next_sequence_num();
    packet.valid_length = static_cast<uint32_t>(payload.size());
    packet.payload = payload;
    packet.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.ad_channel = ad_channel;
    
    // 调试输出
    LOG(INFO) << "创建IP信息包[AD" << (int)ad_channel << "]:";
    LOG(INFO) << "  IP: " << ip_info.get_ip_string();
    LOG(INFO) << "  子网掩码: " << ip_info.get_netmask_string();
    LOG(INFO) << "  时间戳: " << ip_info.get_timestamp_string() 
              << " (" << ip_info.timestamp << ")";
    
    return packet;
}

// 创建采集IQ命令包（新增）
ProtocolPacket ProtocolHandler::create_collect_iq_packet(uint32_t seq_num, uint32_t collect_time_ms,
                                                       uint8_t ad_channel) {
    std::vector<uint8_t> payload(COLLECT_IQ_PACKET_SIZE); // 4字节
    
    // 填充采集时间（网络字节序）
    uint32_t net_collect_time = htonl(collect_time_ms);
    std::memcpy(payload.data(), &net_collect_time, 4);
    
    // 创建包
    ProtocolPacket packet;
    packet.packet_type = PACKET_TYPE_COLLECT_IQ;
    packet.sequence_num = seq_num;
    packet.valid_length = static_cast<uint32_t>(payload.size());
    packet.payload = payload;
    packet.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.ad_channel = ad_channel;
    
    // 填充采集IQ信息
    packet.collect_iq_info.collect_time_ms = collect_time_ms;
    
    // 计算期望样本数
    // 每25ns产生8个样本，1ms = 1,000,000ns
    // 样本数 = (采集时间ms * 1,000,000ns/ms) / 25ns * 8
    double samples_per_ms = (1e6 / IQ_SAMPLE_PERIOD_NS) * IQ_SAMPLES_PER_CYCLE;
    packet.collect_iq_info.expected_samples = static_cast<uint32_t>(collect_time_ms * samples_per_ms);
    packet.collect_iq_info.data_pool_size = packet.collect_iq_info.expected_samples * IQ_SAMPLE_BYTES;
    packet.collect_iq_info.collection_finished = false;
    
    LOG(INFO) << "创建采集IQ命令包[AD" << (int)ad_channel << "]:";
    LOG(INFO) << "  采集时间: " << collect_time_ms << " ms";
    LOG(INFO) << "  期望样本数: " << packet.collect_iq_info.expected_samples;
    LOG(INFO) << "  数据池大小: " << packet.collect_iq_info.data_pool_size << " 字节 ("
              << static_cast<double>(packet.collect_iq_info.data_pool_size) / (1024*1024) << " MB)";
    
    return packet;
}

// 创建采集完成反馈包（新增）
ProtocolPacket ProtocolHandler::create_collect_finish_packet(uint32_t seq_num, bool success,
                                                           uint32_t finish_timestamp,
                                                           uint8_t ad_channel) {
    std::vector<uint8_t> payload(5); // 1字节状态 + 4字节时间戳
    
    // 填充状态（1字节）
    payload[0] = success ? 1 : 0;
    
    // 填充完成时间戳（网络字节序）
    uint32_t net_finish_time = htonl(finish_timestamp);
    std::memcpy(payload.data() + 1, &net_finish_time, 4);
    
    // 创建包
    ProtocolPacket packet;
    packet.packet_type = PACKET_TYPE_COLLECT_FINISH;
    packet.sequence_num = seq_num;
    packet.valid_length = static_cast<uint32_t>(payload.size());
    packet.payload = payload;
    packet.timestamp = static_cast<uint64_t>(time(nullptr));
    packet.ad_channel = ad_channel;
    
    // 填充采集IQ信息
    packet.collect_iq_info.collection_finished = success;
    packet.collect_iq_info.finish_timestamp = finish_timestamp;
    
    LOG(INFO) << "创建采集完成反馈包[AD" << (int)ad_channel << "]:";
    LOG(INFO) << "  状态: " << (success ? "成功" : "失败");
    LOG(INFO) << "  完成时间: " << finish_timestamp;
    
    return packet;
}
bool ProtocolHandler::validate_packet(const ProtocolPacket& packet) {
    // 检查包类型
    if (packet.packet_type < PACKET_TYPE_COMMAND) {
        return false;
    }

    // 检查序列号
    if (packet.sequence_num == 0) {
        return false;
    }

    // 检查有效长度
    if (packet.valid_length > MAX_PAYLOAD_SIZE) {
        return false;
    }

    // 检查CRC
    if (!packet.crc_valid) {
        return false;
    }

    return true;
}

void ProtocolHandler::print_packet_info(const ProtocolPacket& packet, const std::string& prefix) {
    std::stringstream ss;
    ss << prefix;

    // 包类型字符串
    std::string type_str;
    switch (packet.packet_type) {
        case PACKET_TYPE_COMMAND: type_str = "命令"; break;
        case PACKET_TYPE_IQ_DATA: type_str = "IQ数据"; break;
        case PACKET_TYPE_ACK: type_str = "确认"; break;
        case PACKET_TYPE_IP_INFO: type_str = "IP信息"; break;
        default: type_str = "未知"; break;
    }

    ss << "包类型: " << type_str
       << ", AD通道=" << (int)packet.ad_channel
       << ", 序号=" << packet.sequence_num
       << ", 长度=" << packet.valid_length
       << ", CRC=" << (packet.crc_valid ? "有效" : "无效")
       << ", 错误标志=" << (packet.error_flag ? "是" : "否");

    if (packet.timestamp > 0) {
        ss << ", 时间戳=" << packet.timestamp;
    }

    std::cout << ss.str() << std::endl;

    // 如果是IP信息包，显示详细信息
    if (packet.packet_type == PACKET_TYPE_IP_INFO && packet.valid_length >= 12) {
        // 解析IP信息
        uint8_t ip_bytes[4];
        uint8_t netmask_bytes[4];
        uint32_t timestamp;
        
        std::memcpy(ip_bytes, packet.payload.data(), 4);
        std::memcpy(netmask_bytes, packet.payload.data() + 4, 4);
        std::memcpy(&timestamp, packet.payload.data() + 8, 4);
        timestamp = ntohl(timestamp);  // 转换为主机字节序
        
        // 格式化时间
        char time_buf[32];
        time_t t = static_cast<time_t>(timestamp);
        struct tm* tm_info = localtime(&t);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-d %H:%M:%S", tm_info);
        
        std::cout << prefix << "  IP地址: " 
                  << static_cast<int>(ip_bytes[0]) << "." 
                  << static_cast<int>(ip_bytes[1]) << "." 
                  << static_cast<int>(ip_bytes[2]) << "." 
                  << static_cast<int>(ip_bytes[3]) << std::endl;
        std::cout << prefix << "  子网掩码: " 
                  << static_cast<int>(netmask_bytes[0]) << "." 
                  << static_cast<int>(netmask_bytes[1]) << "." 
                  << static_cast<int>(netmask_bytes[2]) << "." 
                  << static_cast<int>(netmask_bytes[3]) << std::endl;
        std::cout << prefix << "  时间: " << time_buf 
                  << " (" << timestamp << ")" << std::endl;
    }

    // 如果是采集IQ命令包，显示详细信息
    if (packet.packet_type == PACKET_TYPE_COLLECT_IQ) {
        std::cout << prefix << "  采集IQ命令包:" << std::endl;
        std::cout << prefix << "    采集时间: " << packet.collect_iq_info.collect_time_ms << " ms" << std::endl;
        std::cout << prefix << "    期望样本数: " << packet.collect_iq_info.expected_samples << std::endl;
        std::cout << prefix << "    数据池大小: " << packet.collect_iq_info.data_pool_size << " 字节" << std::endl;
    }
    
    // 如果是采集完成反馈包，显示详细信息
    if (packet.packet_type == PACKET_TYPE_COLLECT_FINISH) {
        std::cout << prefix << "  采集完成反馈包:" << std::endl;
        std::cout << prefix << "    状态: " << (packet.collect_iq_info.collection_finished ? "成功" : "失败") << std::endl;
        
        char time_buf[32];
        time_t t = static_cast<time_t>(packet.collect_iq_info.finish_timestamp);
        struct tm* tm_info = localtime(&t);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        std::cout << prefix << "    完成时间: " << time_buf 
                  << " (" << packet.collect_iq_info.finish_timestamp << ")" << std::endl;
    }
}
