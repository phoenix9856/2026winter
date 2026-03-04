#ifndef __IQ_HPP__
#define __IQ_HPP__

#include <vector>
#include <string>
#ifndef NO_BOOST
#include <boost/serialization/base_object.hpp>
#endif
//#include "EMPT/PDW.h"
#include "EMPT/Radar.h"
#include "CEMA/cema_api_struct_v0.3.h"

#define CASE_STR(x)                             \
	case x:                                     \
    return #x;                                  \
    break;

    
struct IQData
{
    double IData; // 16bits
    double QData; // 16bits

private:
#ifndef NO_BOOST        
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar &IData;
            ar &QData;
        }
#endif    
};

struct IQDataArray
{
    std::vector<struct IQData> aIQDataArray;
    bool newflag;

private:
#ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar &aIQDataArray;
            ar &*(int*)(bool*)&newflag;
        }
#endif    
};

struct IQDataArray2D
{
    std::vector<struct IQDataArray> aIQDataArray2D;

private:
#ifndef NO_BOOST        
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar &aIQDataArray2D;
        }
#endif    
};

enum SignalType
{
	Communication=0,
	Radar=1,
	Interference=2
};

enum NodeKind
{
    normal = 0,
    sink,
};

// TODA:Reconstruct
enum ModuleTypes
{
    FSK_2 = 0,
    BPSK,
    MSK,
    QPSK,
    PSK_8,
    QAM_16,
    QAM_64,
    GMSK
};

enum InterfereTypes
{
    Linear_frequency_sweep = 0, // 线性扫频
    Multi_tone,                 // 多音
    Narrow_band,                // 窄带干扰
    Pulse,                      // 脉冲干扰
    Single_tone,                // 单音
    Aiming_interference,        // 瞄准干扰
    Tracking_interference,      // 跟踪干扰
    Deceptive_interference,     // 欺骗干扰
    AMNoise,                    // 噪声调幅干扰
    FMNoise                     // 噪声调频干扰
};

enum LocMethod
{
    Tdoa = 0,      // TDOA
    Aoa,       // AOA
    TdoaAndAoa // AOA联合TDOA
};


struct Pos
{
    double lon;    // 经度
    double lat;    // 纬度
	double height; // 高度

private:
    #ifndef NO_BOOST
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & lon;
            ar & lat;
            ar & height;
        }
    #endif
};

struct IQDataSampleInfo
{
    double sampleFreq; // 采样频率（Hz）
    double sampleBand; // 采样带宽（Hz）
    double radioFreq;  // 载频（Hz）
    double sampleTime; // 采样时间戳
    double centerFreq; // 用于检测和分离的中心频率， 已经废弃
    SignalType signalType;//信号类型
    Pos originLoc;     //辐射源位置
    Pos deviceLoc;     //站点位置
    IQDataArray data;  //

private:
   #ifndef NO_BOOST
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & sampleFreq;
            ar & sampleBand;
            ar & radioFreq;
            ar & sampleTime;
            ar & centerFreq;
            ar & signalType;
            ar & originLoc;
            ar & deviceLoc;
            ar & data;
        }
    #endif
};

struct SignalInfo
{
    double centerFreq; // 信号中心频率
    double bandWidth;  // 信号带宽
    float energy;      // 信号强度dBm
    float snr;         // 信噪比
    double sampleFreq; // 采样频率（Hz）
    double sampleBand; // 采样带宽（Hz）
    double detectTime; // 检测时间
    SignalType type;   // 信号类型

private:
#ifndef NO_BOOST
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & centerFreq;
            ar & bandWidth;
            ar & energy;
            ar & snr;
            ar & sampleFreq;
            ar & sampleBand;
            ar & detectTime;
            ar & type;
        }
#endif
};


struct SignalFftInfo
{
    int fftPoints; // fft点数
    float freqResolution;  // 频率分辨率
    float timeResolution;      // 时间分辨率
    double startFreq;         // 起始频率

private:
#ifndef NO_BOOST
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & fftPoints;
            ar & freqResolution;
            ar & timeResolution;
            ar & startFreq;
        }
#endif
};


struct IQDetectResult
{
    std::vector<SignalInfo> signalInfos; // 信号检测结果
    SignalFftInfo fftInfo;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & signalInfos;
            ar & fftInfo;
        }
    #endif
};

struct FreqLib
{
    double centerFreq; // 频段中心
    double bandWidth;  // 频段带宽
	SignalType sigTypes;

private:
#ifndef NO_BOOST        
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & centerFreq;
            ar & bandWidth;
            ar & sigTypes;
        }
    #endif    
};

struct FreqLibArray
{
    std::vector<FreqLib> aFreqLibArray;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & aFreqLibArray;
        }
    #endif    
};

struct TimeFreqFigure
{
    std::vector<std::vector<float>> data; // 频率的二维序列
    IQDataSampleInfo sampleInfo;          // 信号采样信息
    IQDetectResult detectResult;          // 信号检测结果
    double startTime;                     // 起始时间
    double endTime;                       // 终止时间
    float timeRes;                        // 时间分辨率

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & data;
            ar & sampleInfo;
            ar & detectResult;
            ar & startTime;
            ar & endTime;
            ar & timeRes;
        }
    #endif    
};


struct IQAfterDepart
{
	IQDataArray iqs;   // 单路 IQ数据
    int id;            // 序号
	double sampleFreq; // 采样频率(Hz)
	double bandWidth;  // 滤波带宽(Hz)
    float quality;     // 信号质量 0 - 1.0
	SignalType signalType;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & iqs;
            ar & id;
            ar & sampleFreq;
            ar & bandWidth;
            ar & quality;
            ar & signalType;
        }
    #endif    
};


struct IQAfterDepartArray
{
	std::vector<IQAfterDepart> separatedSignals;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & separatedSignals;
        }
    #endif    
};


struct FreqSpectrum
{
	float rData;
	float iData;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & rData;
            ar & iData;
        }
    #endif    
};


struct FreqSpectrumArray
{
	std::vector<FreqSpectrum> freqSpectrumArry;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & freqSpectrumArry;
        }
    #endif    
};


struct FreqSpectrumDepart
{
    FreqSpectrumArray freqSpectrumDepart; // 信号频率谱
    double freqBegin;                     // 频率谱起始
    double freqEnd;                       // 频率谱结束
    double resolutionFreq;                // 频率分辨率

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & freqSpectrumDepart;
            ar & freqBegin;
            ar & freqEnd;
            ar & resolutionFreq;
        }
    #endif    
};

struct FreqSpectrumDepartArray
{
    std::vector<FreqSpectrumDepart> freqSpectrumDepartArray;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & freqSpectrumDepartArray;
        }
    #endif
};


struct CommResult
{
    std::string moduleType;
private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
        {
            ar & moduleType;
            // 序列化其他成员...
        }
    #endif    
};

struct ParaExtraction
{
    std::vector<float> frequency;// 单音多音的中心频率
    int tone_num; 
    double fcStartFreq;    // 扫频信号起始频率
    double srEndFreq;      //扫频信号终止频率
    double snrSweepSpeed;  // 扫频信号扫频速度
    std::vector<int> predicts;// 预测结果列表
    double sr;      //符号速率, 未启用
    double snr;  // 信噪比, 未启用
private:
#ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & frequency;
        ar & tone_num;
        ar & fcStartFreq;
        ar & srEndFreq;
        ar & snrSweepSpeed;
        ar & predicts;
        ar & sr;
        ar & snr;
    }
    #endif    
};

struct SeparateSignal
{
    int signalsID;         // 信号ID
    IQDataArray IQs;       // IQ数据
    double sampleFreq;     // 采样频率
    double bandWidth;      // 解调带宽
	SignalType signalType; // 信号类型

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & signalsID;
        ar & IQs;
        ar & sampleFreq;
        ar & bandWidth;
        ar & signalType;
    }
    #endif    
};


struct SeparateSignals
{
    std::vector<SeparateSignal> data;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & data;
    }
    #endif    
};


struct IdentifyResultComm
{
    int signalId;           // 信号ID
    int signalType;         // 信号类型
    std::string moduleType; // 调制方式
	ParaExtraction para;
    ModuleTypes mt;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & signalId;
        ar & signalType;
        ar & moduleType;
        ar & para;
        ar & mt;
    }
    #endif
};


struct IdentifyResultCommArray
{
    std::vector<IdentifyResultComm> commresult;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & commresult;
    }
    #endif
};


struct IdentifyResultInterfere
{
    int signalId;              // 信号ID
    int signalType;            // 信号类型
    std::string interfereType; // 干扰类型
	ParaExtraction para;
    InterfereTypes it;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & signalId;
        ar & signalType;
        ar & interfereType;
        ar & para;
        ar & it;
    }
    #endif
};


struct IdentifyResultInterfereArray
{
    std::vector<IdentifyResultInterfere> interfereresult;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & interfereresult;
    }
    #endif    
};

struct IdentifyResult
{
	IdentifyResultCommArray identifyResultComm; 
	IdentifyResultInterfereArray identifyResultInterfere;
private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
		ar & identifyResultComm;
		ar & identifyResultInterfere;
	}
    #endif    
};

struct AngleInfo
{
    double pitch;   // 俯仰角
    double azimuth; // 方位角

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & pitch;
        ar & azimuth;
    }
    #endif    
};

struct CommAndInterSignalLocInfo
{
    IQDataArray2D iqs;                     // 各站点针对某目标的IQ数据
    FreqSpectrumDepartArray FreqSpectrums; // 不同站点对同一目标的频率谱
    std::vector<Pos> siteLocs;             // 站点位置(经纬/经纬高)
    std::vector<AngleInfo> aoa;            // 站点测向俯仰角结果(度)
    double carrierFre;                     // 载频(Hz)
    double bandwidth;                      // 带宽(HZ)
    double sampleFreq;                     // 采样频率
    LocMethod method;                      // 体制选择 (TDOA、AOA、TDOA & AOA)

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & iqs;
        ar & FreqSpectrums;
        ar & siteLocs;
        ar & aoa;
        ar & carrierFre;
        ar & bandwidth;
        ar & sampleFreq;
        ar & method;
    }
    #endif    
};

struct RadarSignalLocInfo
{
    CEMA::PDWArray2D pdwLoc;         // 各站点针对某目标的雷达TOA参数(ns)脉冲描述字
    std::vector<Pos> siteLocs; // 站点位置(经纬/经纬高)
    LocMethod method;          // 体制选择 (TDOA、AOA、TDOA & AOA)

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & pdwLoc;
        ar & siteLocs;
        ar & method;
    }
    #endif    
};

struct SignalLocInfo
{
    CommAndInterSignalLocInfo commAndInterSignal;
    RadarSignalLocInfo radarSignal;
    SignalType type; // 制式的选择 (雷达信号定位还是通信信号定位)

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & commAndInterSignal;
        ar & radarSignal;
        ar & type;
    }
    #endif    
};

struct TargetLoc
{
    Pos targetLoc; // 目标位置(经纬/经纬高)
    float confidence;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & targetLoc;
        ar & confidence;
    }
    #endif    
};

struct SignalSource {
    Pos pos;
    double freq;
    double power;
    std::string antennaPattern;

private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & pos;
        ar & freq;
        ar & power;
        ar & antennaPattern;
    }
    #endif    
};

struct ScanFrequency
{
    int currentNum;
    int totalNum;
	double centerFreq; 
	double bandWidth;
    double startFreq;
    double endFreq;
private:
    #ifndef NO_BOOST    
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & currentNum;
        ar & totalNum;
        ar & centerFreq;
        ar & bandWidth;
        ar & startFreq;
        ar & endFreq;
    }
    #endif
};

#endif //__IQ_HPP__
