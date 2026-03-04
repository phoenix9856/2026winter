#ifndef GLOBAL_COLLECT_STATE_H
#define GLOBAL_COLLECT_STATE_H

#include <mutex>
#include <map>
#include <cstdint>
#include <vector>
#include <atomic>
#include "config.h"
#include "ProtocolHandler.h"

// 全局采集状态管理结构体
struct GlobalCollectState {
    std::mutex mtx;
    std::atomic<bool> in_collection;          // 是否在采集过程中
    uint32_t collect_time_ms;               // 采集时间（毫秒）
    uint32_t expected_samples_per_ad;       // 每个AD通道期望的样本数
    uint32_t data_pool_size_per_ad;         // 每个AD通道数据池大小（字节）
    std::map<uint8_t, DataPool> ad_data_pools; // 每个AD通道的数据池
    uint64_t receive_start_time_us;         // 采集开始时间（微秒）
    uint64_t receive_finish_time_us;        // 采集完成时间（微秒）
    std::atomic<bool> waiting_for_finish;    // 是否在等待采集完成反馈
    std::atomic<bool> all_data_received;     // 所有AD通道数据是否接收完成
    std::map<uint8_t, int> last_print_percent;  // 记录每个AD通道上一次打印的进度阈值（初始为0%）
    
    GlobalCollectState();
    
    void reset();
    void init_collection(uint32_t time_ms);
    bool add_iq_data(uint8_t ad_channel, const uint8_t* data, uint32_t size);
    void check_all_data_received();
    
private:
    static uint64_t get_current_time_us();
};

#endif // GLOBAL_COLLECT_STATE_H