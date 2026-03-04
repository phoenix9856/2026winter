#include "GlobalCollectState.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <glog/logging.h>

GlobalCollectState::GlobalCollectState() {
    reset();
}

void GlobalCollectState::reset() {
    std::lock_guard<std::mutex> lock(mtx);
    in_collection = false;
    collect_time_ms = 0;
    expected_samples_per_ad = 0;
    data_pool_size_per_ad = 0;
    ad_data_pools.clear();
    last_print_percent.clear();
    receive_start_time_us = 0;
    receive_finish_time_us = 0;
    waiting_for_finish = false;
    all_data_received = false;
}

void GlobalCollectState::init_collection(uint32_t time_ms) {
    std::lock_guard<std::mutex> lock(mtx);
    
    in_collection = true;
    collect_time_ms = time_ms;
    // receive_start_time_us = get_current_time_us();
    waiting_for_finish = true;
    
    // 计算期望样本数和数据池大小
    double samples_per_ms = (1e6 / IQ_SAMPLE_PERIOD_NS) * IQ_SAMPLES_PER_CYCLE;
    expected_samples_per_ad = static_cast<uint32_t>(time_ms * samples_per_ms);
    data_pool_size_per_ad = expected_samples_per_ad * IQ_SAMPLE_BYTES;
    
    // 为每个AD通道初始化数据池和进度打印阈值
    for (int ad = 0; ad < NUM_AD_CHANNELS; ad++) {
        ad_data_pools[ad] = DataPool(ad, expected_samples_per_ad);
        last_print_percent[ad] = 0; // 初始化：上一次打印进度为0%
    }
    
    std::cout << "初始化采集状态: 采集时间=" << time_ms << "ms, "
              << "每AD期望样本数=" << expected_samples_per_ad << ", "
              << "每AD数据池大小=" << data_pool_size_per_ad << " 字节 ("
              << static_cast<double>(data_pool_size_per_ad) / (1024*1024) << " MB)" << std::endl;
}
bool GlobalCollectState::add_iq_data(uint8_t ad_channel, const uint8_t* data, uint32_t size) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (!in_collection || ad_data_pools.find(ad_channel) == ad_data_pools.end()) {
        std::cout << "AD" << (int)ad_channel << " 数据池未找到" << std::endl;
        return false;
    }
    
    if (!ad_data_pools[ad_channel].ready_for_data) {
        std::cout << "AD" << (int)ad_channel << " 数据池未准备好接收数据" << std::endl;
        return false;
    }

    if (ad_channel == 0 && ad_data_pools[ad_channel].get_usage_percentage() == 0) {
        std::cout << "AD0 开始接收数据，记录接收开始时间" << std::endl;
        receive_start_time_us = get_current_time_us();
    }

    bool success = ad_data_pools[ad_channel].add_iq_data(data, size);
    if (success) {
        // 计算当前进度（取整，比如21%算20%，40%算40%）
        double current_percent = ad_data_pools[ad_channel].get_usage_percentage();
        int current_percent_int = static_cast<int>(current_percent);

        // 获取上一次打印的进度阈值（如果没初始化，默认0）
        int last_percent = last_print_percent[ad_channel];
        
        // 判定条件：进度达到20%的整数倍，且超过上一次打印的阈值（包括100%）
        if (current_percent_int >= 100) {
            // 100% 强制打印
            std::cout << "AD" << (int)ad_channel << " 接收数据: " << size << " 字节, "
                      << "进度: " << std::fixed << std::setprecision(1)
                      << current_percent << "%" << std::endl;
            last_print_percent[ad_channel] = 100; // 更新阈值为100%，避免重复打印
        } else if (current_percent_int % 20 == 0 && current_percent_int > last_percent) {
            // 进度达到20%/40%/60%/80%，且超过上一次打印的阈值
            std::cout << "AD" << (int)ad_channel << " 接收数据: " << size << " 字节, "
                      << "进度: " << std::fixed << std::setprecision(1)
                      << current_percent << "%" << std::endl;
            last_print_percent[ad_channel] = current_percent_int; // 更新上一次打印的阈值
        }
        
        // 检查所有AD通道是否都完成
        check_all_data_received();
    }
    
    return success;
}

void GlobalCollectState::check_all_data_received() {
    all_data_received = true;
    for (const auto& pair : ad_data_pools) {
        if (!pair.second.is_complete()) {
            all_data_received = false;
            break;
        }
    }
    
    if (all_data_received) {
        if (!in_collection) {   
            LOG(WARNING) << "print_collection_statistics: in_collection 为 false，跳过打印";
            return;
        }
        receive_finish_time_us = get_current_time_us();
        waiting_for_finish = false;

        uint64_t total_duration_us = receive_finish_time_us - receive_start_time_us;
        double total_duration_sec = static_cast<double>(total_duration_us) / 1000000.0;
        
        std::cout << "==================== 采集统计信息 ====================" << std::endl;
        std::cout << "采集时间: " << collect_time_ms << " ms" << std::endl;
        std::cout << "总持续时间: " << total_duration_us << " 微秒 ("
                << total_duration_sec << " 秒)" << std::endl;
        std::cout << "每AD期望样本数: " << expected_samples_per_ad << std::endl;
        std::cout << "每AD数据池大小: " << data_pool_size_per_ad << " 字节" << std::endl;
        
        uint64_t total_data_received = 0;
        for (const auto& pair : ad_data_pools) {
            const DataPool& pool = pair.second;
            total_data_received += pool.received_samples * IQ_SAMPLE_BYTES;
            
            std::cout << "  AD" << (int)pool.ad_channel << ": "
                    << pool.received_samples << "/" << pool.expected_samples << " 样本 ("
                    << std::fixed << std::setprecision(1) << pool.get_usage_percentage() << "%)" << std::endl;
        }
        
        double avg_data_rate_mbps = (total_data_received * 8.0) / (total_duration_sec * 1024 * 1024);
        double avg_data_rate_mb_per_s = total_data_received / (total_duration_sec * 1024 * 1024);
        
        // 2. 总数据量显示优化（保留4位小数，避免显示0.0 GB）
        std::cout << "总接收数据: " << total_data_received << " 字节 ("
                << std::setprecision(4) << static_cast<double>(total_data_received) / (1024*1024*1024) << " GB)" << std::endl;

        // 3. 补充MB显示（更直观）
        std::cout << "            " << std::setprecision(2) << static_cast<double>(total_data_received) / (1024*1024) << " MB)" << std::endl;
        std::cout << "平均数据率: " << avg_data_rate_mb_per_s << " MB/s ("
                << avg_data_rate_mbps << " Mbps)" << std::endl;
        std::cout << "======================================================" << std::endl;
    }
}

uint64_t GlobalCollectState::get_current_time_us() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}