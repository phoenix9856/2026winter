#include "IQConverter.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

IQConverter::IQConverter(double sample_rate, double center_freq, 
                         double bandwidth, int decimation)
    : sample_rate_(sample_rate), center_freq_(center_freq),
      bandwidth_(bandwidth), decimation_factor_(decimation),
      phase_(0.0), sample_counter_(0) {
    
    // 参数验证
    if (sample_rate <= 0) throw std::invalid_argument("采样率必须大于0");
    if (center_freq >= sample_rate / 2) throw std::invalid_argument("中心频率过高");
    if (bandwidth <= 0) throw std::invalid_argument("带宽必须大于0");
    if (decimation <= 0) throw std::invalid_argument("抽取因子必须大于0");
    
    // 计算相位增量
    phase_increment_ = 2.0 * M_PI * center_freq_ / sample_rate_;
    
    // 设计低通滤波器
    int filter_order = 8 * decimation_factor_;
    if (filter_order % 2 == 0) filter_order++;
    
    filter_coeffs_.resize(filter_order);
    int center = filter_order / 2;
    double cutoff = bandwidth * 1.2 / sample_rate;
    
    for (int i = 0; i < filter_order; i++) {
        if (i == center) {
            filter_coeffs_[i] = 2.0 * cutoff;
        } else {
            double x = 2.0 * M_PI * cutoff * (i - center);
            filter_coeffs_[i] = std::sin(x) / (M_PI * (i - center));
        }
        
        // 汉明窗
        double window = 0.54 - 0.46 * std::cos(2.0 * M_PI * i / (filter_order - 1));
        filter_coeffs_[i] *= window;
    }
    
    // 初始化历史数据
    i_history_.resize(filter_order, 0.0);
    q_history_.resize(filter_order, 0.0);
}

// 转换为交错IQ格式
void IQConverter::convertToInterleaved(const std::vector<double>& if_signal, 
                                      std::vector<double>& iq_interleaved) {
    size_t output_len = getOutputLength(if_signal.size());
    iq_interleaved.resize(output_len * 2);  // 每个IQ样本需要2个元素
    
    size_t output_index = 0;
    
    for (size_t i = 0; i < if_signal.size(); i++) {
        double i_val, q_val;
        
        if (processSample(if_signal[i], i_val, q_val)) {
            // 只有当需要输出时，processSample返回true
            iq_interleaved[output_index * 2] = i_val;      // I分量
            iq_interleaved[output_index * 2 + 1] = q_val;  // Q分量
            output_index++;
        }
    }
}

// 批量处理版本（更高效率）
size_t IQConverter::convertBlock(const double* input, size_t input_len, 
                                double* output, size_t output_max_len) {
    if (!input || !output) return 0;
    
    size_t output_index = 0;
    
    for (size_t i = 0; i < input_len && output_index * 2 < output_max_len - 1; i++) {
        double i_val, q_val;
        
        if (processSample(input[i], i_val, q_val)) {
            output[output_index * 2] = i_val;      // I
            output[output_index * 2 + 1] = q_val;  // Q
            output_index++;
        }
    }
    
    return output_index;  // 返回生成的IQ对数
}

// 实时处理单个样本
bool IQConverter::processSample(double input_sample, double& i_out, double& q_out) {
    // 1. 数字混频
    double cos_val = std::cos(phase_);
    double sin_val = std::sin(phase_);
    
    double i_mixed = input_sample * cos_val;
    double q_mixed = input_sample * (-sin_val);  // 注意负号
    
    // 更新相位
    phase_ += phase_increment_;
    if (phase_ > 2.0 * M_PI) {
        phase_ -= 2.0 * M_PI;
    }
    
    // 2. 更新滤波器历史数据（高效移位）
    std::copy(i_history_.begin(), i_history_.end() - 1, i_history_.begin() + 1);
    i_history_[0] = i_mixed;
    
    std::copy(q_history_.begin(), q_history_.end() - 1, q_history_.begin() + 1);
    q_history_[0] = q_mixed;
    
    sample_counter_++;
    
    // 3. 抽取：只在抽取时刻计算输出
    if (sample_counter_ % decimation_factor_ == 0) {
        // FIR滤波
        i_out = 0.0;
        q_out = 0.0;
        
        for (size_t j = 0; j < filter_coeffs_.size(); j++) {
            i_out += filter_coeffs_[j] * i_history_[j];
            q_out += filter_coeffs_[j] * q_history_[j];
        }
        
        return true;
    }
    
    return false;
}

// 从交错格式转换为复数格式
void IQConverter::interleavedToComplex(const double* interleaved, size_t num_samples,
                                      std::vector<std::complex<double>>& complex_iq) {
    complex_iq.resize(num_samples);
    
    for (size_t i = 0; i < num_samples; i++) {
        complex_iq[i] = std::complex<double>(
            interleaved[i * 2],      // I分量
            interleaved[i * 2 + 1]   // Q分量
        );
    }
}

// 从复数格式转换为交错格式
void IQConverter::complexToInterleaved(const std::vector<std::complex<double>>& complex_iq,
                                      std::vector<double>& interleaved) {
    interleaved.resize(complex_iq.size() * 2);
    
    for (size_t i = 0; i < complex_iq.size(); i++) {
        interleaved[i * 2] = complex_iq[i].real();      // I
        interleaved[i * 2 + 1] = complex_iq[i].imag();  // Q
    }
}

// 提取I路数据
void IQConverter::extractIChannel(const double* interleaved, size_t num_samples,
                                 std::vector<double>& i_channel) {
    i_channel.resize(num_samples);
    
    for (size_t i = 0; i < num_samples; i++) {
        i_channel[i] = interleaved[i * 2];
    }
}

// 提取Q路数据
void IQConverter::extractQChannel(const double* interleaved, size_t num_samples,
                                 std::vector<double>& q_channel) {
    q_channel.resize(num_samples);
    
    for (size_t i = 0; i < num_samples; i++) {
        q_channel[i] = interleaved[i * 2 + 1];
    }
}

void IQConverter::reset() {
    phase_ = 0.0;
    sample_counter_ = 0;
    std::fill(i_history_.begin(), i_history_.end(), 0.0);
    std::fill(q_history_.begin(), q_history_.end(), 0.0);
}

size_t IQConverter::getOutputLength(size_t input_length) const {
    return (input_length + decimation_factor_ - 1) / decimation_factor_;
}

double IQConverter::getOutputSampleRate() const {
    return sample_rate_ / decimation_factor_;
}