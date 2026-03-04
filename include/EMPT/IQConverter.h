#ifndef IQCONVERTER_H
#define IQCONVERTER_H

#include <vector>
#include <complex>
#include <cstdint>
#include <algorithm>

// 交错IQ数据格式
template<typename T>
struct InterleavedIQ {
    T* data;           // 数据指针: I0, Q0, I1, Q1, ...
    size_t length;      // 样本数量（IQ对数）
    
    InterleavedIQ(T* ptr, size_t len) : data(ptr), length(len) {}
};

class IQConverter {
private:
    double sample_rate_;
    double center_freq_;
    double bandwidth_;
    int decimation_factor_;
    
    // 相位累加器
    double phase_;
    double phase_increment_;
    
    // 滤波器系数和历史数据
    std::vector<double> filter_coeffs_;
    std::vector<double> i_history_;
    std::vector<double> q_history_;
    
    // 计数器
    int sample_counter_;

public:
    IQConverter(double sample_rate, double center_freq, 
                double bandwidth, int decimation = 4);
    
    // 主要转换函数 - 输出为交错格式
    void convertToInterleaved(const std::vector<double>& if_signal, 
                             std::vector<double>& iq_interleaved);
    
    // 批量处理版本（更高效率）
    size_t convertBlock(const double* input, size_t input_len, 
                       double* output, size_t output_max_len);
    
    // 实时处理（逐个样本）
    bool processSample(double input_sample, double& i_out, double& q_out);
    
    // 格式转换工具函数
    static void interleavedToComplex(const double* interleaved, size_t num_samples,
                                    std::vector<std::complex<double>>& complex_iq);
    
    static void complexToInterleaved(const std::vector<std::complex<double>>& complex_iq,
                                    std::vector<double>& interleaved);
    
    // 从交错格式中提取I路或Q路
    static void extractIChannel(const double* interleaved, size_t num_samples,
                               std::vector<double>& i_channel);
    
    static void extractQChannel(const double* interleaved, size_t num_samples,
                               std::vector<double>& q_channel);
    
    // 重置状态
    void reset();
    
    // 获取信息
    size_t getOutputLength(size_t input_length) const;
    double getOutputSampleRate() const;
};

#endif // IQCONVERTER_H