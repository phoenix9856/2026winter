#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>

// 包类型定义
const uint8_t PACKET_TYPE_COMMAND = 0x01;   // 命令包
const uint8_t PACKET_TYPE_IQ_DATA = 0x02;   // IQ数据包
const uint8_t PACKET_TYPE_ACK = 0x03;       // 确认包
const uint8_t PACKET_TYPE_IP_INFO = 0x04;   // IP信息包
const uint8_t PACKET_TYPE_COLLECT_IQ = 0x07; // 采集IQ命令包（新增）
const uint8_t PACKET_TYPE_COLLECT_FINISH = 0x08; // 采集完成反馈包（新增）

// 服务器配置
const std::string SERVER_IP_1 = "192.168.2.170";
const std::string SERVER_IP_2 = "192.168.1.111";
const int SERVER_PORT_1 = 8888;
const int SERVER_PORT_2 = 7777;
const int MAX_CLIENTS = 100;
const int CLIENT_TIMEOUT_MS = -1;           // -1表示无限等待

// 协议配置（与客户端保持一致）
const size_t PACKET_SIZE = 4096 * 32 ;            // 完整数据包大小（与客户端一致）
const size_t HEADER_SIZE = 14;              // 帧头大小：4字节时间戳+4字节序列号+1字节包类型+1字节AD通道+4字节有效长度
const size_t TRAILER_SIZE = 2;              // 帧尾大小（CRC）
const size_t MAX_PAYLOAD_SIZE = PACKET_SIZE - HEADER_SIZE - TRAILER_SIZE;  // 最大有效载荷

// 发送/接收超时配置（微秒）
const int SEND_TIMEOUT_US = 1000;           // 发送超时
const int RECV_TIMEOUT_US = 1000;           // 接收超时

// 命令行交互配置
const std::string PROMPT_STR = "server> ";  // 服务器命令行提示符
const int MAX_INPUT_LEN = 256;              // 命令行输入最大长度

// IQ数据流配置（与客户端保持一致）
const int COLLECT_IQ_PACKET_SIZE = 4;       // 采集IQ命令包大小（4字节采集时间）

// 多客户端配置
const int NUM_AD_CHANNELS = 8;              // AD通道数量
const int MASTER_CLIENT_CHANNEL = 0;        // 主客户端通道（AD0）

// IQ采样配置
const double IQ_SAMPLE_RATE_HZ = 40e6;      // 40MHz采样率
const double IQ_SAMPLE_PERIOD_NS = 25.0;    // 25ns采样周期
const int IQ_SAMPLE_BYTES = 4;              // 每个IQ样本占4字节（I:2字节，Q:2字节）
const int IQ_SAMPLES_PER_CYCLE = 8;         // 每25ns产生8个IQ样本

// 频段配置（2-18GHz，每路AD采集2GHz带宽）
const double START_FREQ_GHZ = 2.0;          // 起始频率2GHz
const double BANDWIDTH_PER_AD_GHZ = 2.0;    // 每路AD带宽2GHz

#endif // CONFIG_H