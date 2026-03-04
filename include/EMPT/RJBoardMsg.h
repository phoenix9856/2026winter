
/* ==================================================================
 * Copyright (c) 2020, micROS Group, TAIIC, NIIDT & HPCL.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 * 3. All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * This product includes software developed by the micROS Group. and
 * its contributors.
 * 4. Neither the name of the Group nor the names of its contributors may
 * be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICROS,GROUP AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE MICROS,GROUP OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ===================================================================
 * Author: Zhidong Xie.
 */

#ifndef RJBOARDMSG_H_
#define RJBOARDMSG_H_

#include "CEMCGlobalDef.h"
#include <iostream>
#include <cstring>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "telemetry_local.h"

#define HEARTBEAT_MSG 0x01
#define SEARCH_MSG 0x03
#define ATTACK_MSG 0x04
#define SAMPLE_MSG 0x08
#define IQSAMP_MSG 0x10

#define ANTENNA_MSG 0x12
#define UAV_MSG 0x14

#define PARAMETER_MSG 0x01
#define RESULT_MSG 0x02
#define SETMOD_MSG 0x04

#define RADARSEARCH 1
#define PDWSAMPLE 3
#define PDWSETMODE 4

#define sizeMessageHead 32
#define sizeMessageData 480
#define sizeSocketBuffer 512

#define sizeSearchResult 33
#define sizeAttackResult 33

#define sizeRadarPDWBuffer 2048
#define sizeRadarDataBuffer 2048
#define sizeRadarPDWBuffer722 32

#define RJB_MAX_MSG_LEN sizeRadarPDWBuffer * 8 + sizeMessageHead
#define RJB_WAIT_TIMEOUT 3000

#define RJB_SAMPLE_RATE 245.76
// #define FFT_NUM  1024
#pragma pack(1)

typedef struct
{
   U16 year;
   U8 month;
   U8 day;
   U8 hour;
   U8 minute;
   U8 second;
   U8 usecond;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &year;
      ar &month;
      ar &day;
      ar &hour;
      ar &minute;
      ar &second;
      ar &usecond;
   }
} TIMER_OMC_t;

typedef struct
{
   U8 msgMainType;
   U8 msgSubType;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgMainType;
      ar &msgSubType;
   }
} MSGTYPE_t;

typedef struct
{
   U8 devMainType;
   U8 devSubType;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &devMainType;
      ar &devSubType;
   }
} DEVTYPE_t;

/* 消息头类型
   msgLen：消息头+消息体的总字节数；
   dstLinkID：目的地链路ID号；
*/
typedef struct
{
   U32 msgLen;
   MSGTYPE_t msgType;
   DEVTYPE_t dstDevType;
   U16 dstLinkID;
   DEVTYPE_t srcDevType;
   U16 srcLinkID;
   TIMER_OMC_t time_OMC;
   U8 remain[10];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgLen;
      ar &msgType;
      ar &dstDevType;
      ar &dstLinkID;
      ar &srcDevType;
      ar &srcLinkID;
      ar &time_OMC;
      ar &remain;
   }
} MSGHEAD_t;

/* 通用侦扰板消息类型
RJB_MAX_MSG_LEN：侦扰板消息最大长度，目前定为2052个字节*/
typedef struct
{
   MSGHEAD_t msgHead;
   U8 msg[RJB_MAX_MSG_LEN];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &msg;
   }
} RJBMSG_t;

// pub,sub机制通用传输类型定义
typedef struct
{
   int num;
   std::string str_message;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &num;
      ar &str_message;
   }
} Common_msg;

typedef struct
{
   MSGHEAD_t msgHead;
   Tree_msg tree_msg;
   PS_msg ps_msg;
   Ant_msg ant_msg;
   Net_msg net_msg;
   UAV_msg uav_msg;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &tree_msg;
      ar &ps_msg;
      ar &ant_msg;
      ar &net_msg;
      ar &uav_msg;
   }
} HBMSG_t;

/* 雷达参数
radar_RF_Freq：雷达载频，单位为MHz;
radarPulsewidth：雷达脉宽，单位为ns；
 * */
typedef struct
{
   U16 radar_RF_Freq;
   U32 radarPulsewidth;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &radar_RF_Freq;
      ar &radarPulsewidth;
   }
} _radarParameters;

/* 检测参数
 *
   Pfa：检测的虚警概率；
   detectionNumber：检测到的脉冲个数；

 * */
typedef struct
{
   U32 pfa;
   U32 detectionNumber;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &pfa;
      ar &detectionNumber;
   }

} _detectionParameters;

typedef struct
{
   U32 reconID;
   U32 reconMode;
   U64 Frequency;
   U32 Bandwidth;
   U32 gain;
   U32 receiverStatus;
   U32 reconTime;

   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &reconID;
      ar &reconMode;
      ar &Frequency;
      ar &Bandwidth;
      ar &gain;
      ar &receiverStatus;
      ar &reconTime;
   }
} _ReconParameters;

/* 	感兴趣雷达侦察参数 */
typedef struct
{
   _radarParameters radarPara;
   _detectionParameters detectPara;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &radarPara;
      ar &detectPara;
   }
} searchParameters;

/* 	感兴趣雷达侦察参数配置消息
 */
typedef struct
{
   MSGHEAD_t msgHead;
   _radarParameters radarPara;
   _detectionParameters detectPara;
   _ReconParameters Reconparameters;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &radarPara;
      ar &detectPara;
      ar &Reconparameters;
   }
} SEAPAR_t;

/* 	感兴趣雷达侦察结果返回消息
	searchFlag：是否检测到感兴趣雷达标志*/
typedef struct
{
   MSGHEAD_t msgHead;
   U8 searchFlag;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &searchFlag;
   }
} SEARES_t;

typedef struct
{
   bool sflag;
   U8 sremain[1];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &sflag;
      ar &sremain;
   }
} Search_Result;

/* 干扰调制参数设置
 *
 * -- 干扰模式类型：单假目标0x0、多假目标0x1
 * -- 干扰脉宽范围：200-1000000ns（容忍度-12.5%~+12.5%）

 * -- 干扰脉冲个数：1-256   （对应多假目标，同时个数对应航迹数，初步演示2条）
 * -- 干扰航迹点数：1-65536 （初步商定最大值2000点，各节点之间等间隔）
 *
 * -- 航迹时延间隔： 各条航迹之间的间隔ns
 * -- 时延取值范围：脉宽 < delay_Time < 雷达PRI-2*传播时间（无人机至目标雷达的距离）
 *              规划的航迹点数等间隔下发参数（初步商定100ms，等间隔模式）
 * -- 频移取值范围：0~65536Hz
 *
 * */
#define max_track_num 2
#define max_track_point_num 0
#define pw_select_mode_num 3
#define radar_pulse_width_num 7

typedef struct
{
   U32 delay_Time;
   S16 doppler_Freq;
   U16 jamming_Power;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &delay_Time;
      ar &doppler_Freq;
      ar &jamming_Power;
   }
} DRFM_Param_t;

/*
	delay_Time：时延调制，单位为ns；
	doppler_Freq：多普勒调制，单位为Hz；
	jamming_Power：幅度调制；
*/

typedef struct
{
   U64 jammingCF;
   U32 jammingBW;
   U32 jammingPower;

private:
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &jammingCF;
      ar &jammingBW;
      ar &jammingPower;
   }
} _NoiseJammingParas;

typedef struct
{
   U32 jamm_time;
   U8 msg_num;
   U8 current_msg_num;
   U16 current_point_num;
   U8 jammerID;
   U8 jammingType;
   U64 receiverFrequency;
   U32 receiverBandwidth;
   U32 receiverGain;
   U32 receiverStatus;
   U64 transmitterFrequency;
   U32 transmitterBandwidth;
   U32 transmitterGain;
   U32 transmitterStatus;

   U8 pwSelectMode[pw_select_mode_num];
   U32 radarPulsewidth[radar_pulse_width_num];
   U32 jammingPulseInterval;

   _NoiseJammingParas NoiseJammingParas;
   DRFM_Param_t jammingTrackSequence[max_track_point_num];
   U8 deceiveJammingParasTraceNum[max_track_point_num];
   U32 deceiveJammingParasTraceInterval[max_track_point_num];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &jammerID;
      ar &jammingType;
      ar &receiverFrequency;
      ar &receiverBandwidth;
      ar &receiverGain;
      ar &receiverStatus;
      ar &transmitterFrequency;
      ar &transmitterBandwidth;
      ar &transmitterGain;
      ar &transmitterStatus;
      ar &pwSelectMode;
      ar &radarPulsewidth;
      ar &jammingPulseInterval;
      ar &NoiseJammingParas;
      ar &jammingTrackSequence;
      ar &deceiveJammingParasTraceNum;
      ar &deceiveJammingParasTraceInterval;
   }
} _jammingParameters;
/*
	jammingMode：干扰模式，其中单条航迹为0x00，多条航迹为0x01；
	jammingTrackNumber：同一个方位向产生的航迹数目；
	jammingTrackPointNumber：每条航迹上的点数；
	jammingPulseWidth：感兴趣的雷达脉宽，单位为ns；
	jammingTimeInterval：不同航迹之间的时间间隔，单位为ns；
	max_track_point_num：最大航迹点的数目；

*/

/* 	干扰参数配置消息 */
typedef struct
{
   MSGHEAD_t msgHead;
   _jammingParameters jamPara;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &jamPara;
   }
} JAMDOWN_t;

/*干扰结果返回消息
	jammingCompleteFlag：干扰完成标志*/
typedef struct
{
   MSGHEAD_t msgHead;
   U8 jammingCompleteFlag;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &jammingCompleteFlag;
   }
} JAMRES_t;

typedef struct
{
   bool jflag;
   U8 jremain[1];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &jflag;
      ar &jremain;
   }
} Jam_Result;

typedef struct
{
   searchParameters searchPara;
   U32 radarSampletime;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &searchPara;
      ar &radarSampletime;
   }
} sampleParameters;

typedef struct
{
   U32 deviceType;
   U32 antennaType;
   U32 antennaSwitch;
   U32 antennaAngle[2];

private:
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &deviceType;
      ar &antennaType;
      ar &antennaSwitch;
      ar &antennaAngle;
   }
} _SimAntennaParas;

typedef struct
{
   MSGHEAD_t msgHead;
   _SimAntennaParas SimAntennaParas;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &SimAntennaParas;
   }
} Antenna_t;

typedef struct
{
   S64 longitude;
   S64 latitude;
   S64 height;
   S64 Yaw_mag;
   S64 Roll;
   S64 Pitch;

private:
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &longitude;
      ar &latitude;
      ar &height;
      ar &Yaw_mag;
      ar &Roll;
      ar &Pitch;
   }
} _UAVParas;

typedef struct
{
   MSGHEAD_t msgHead;
   _UAVParas UAVParas;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &UAVParas;
   }
} UAV_t;

/* 	PDW采样参数配置消息*/
typedef struct
{
   MSGHEAD_t msgHead;
   _ReconParameters Reconparameters;
   _radarParameters radarPara;
   U32 radarSampletime;
   _detectionParameters detectPara;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &Reconparameters;
      ar &radarPara;
      ar &radarSampletime;
      ar &detectPara;
   }
} SAMPDW_t;

/* 	PDW采样结果返回消息 */
typedef struct
{
   MSGHEAD_t msgHead;
   U64 SampleData[sizeRadarPDWBuffer];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &SampleData;
   }
} SAMPDWRES_t;

struct SimPulseDescriptionWord722
{
   int radarID;
   double directionOfArrival;
   double pulseWidth;
   double carrierFreqency;
   double timeOfArrival;
   double pulseAmplitude;
   double bandWidth;

private:
   friend class boost::serialization::access;

   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &radarID;
      ar &directionOfArrival;
      ar &pulseWidth;
      ar &carrierFreqency;
      ar &timeOfArrival;
      ar &pulseAmplitude;
      ar &bandWidth;
   }
};

/* 	PDW采样结果返回消息 */
typedef struct
{
   MSGHEAD_t msgHead;
   SimPulseDescriptionWord722 PDWdata[sizeRadarPDWBuffer722];
   int total_times;
   int PDWNumber;

   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &PDWdata;
      ar &total_times;
      ar &PDWNumber;
   }
} SAMPDWRES722_t;

/*
	IQ采样参数配置消息
*/
typedef struct
{
   MSGHEAD_t msgHead;
   _radarParameters radarPara;
   U32 radarSampletime;
   _detectionParameters detectPara;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &radarPara;
      ar &radarSampletime;
      ar &detectPara;
   }
} SAMIQ_t;

/*
	IQ采样结果返回消息
	SizeRadarDataBuffer：IQ数据采样的长度，暂定为2048；
*/
typedef struct
{
   MSGHEAD_t msgHead;
   U64 SampleData[sizeRadarDataBuffer];
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &msgHead;
      ar &SampleData;
   }
} SAMIQRES_t;

typedef struct
{
   double Rx_RF_Freq;
   double Rx1_RF_Gain;
   double Rx2_RF_Gain;

   double Tx_RF_Freq;
   double Tx1_Attenuation;
   double Tx2_Attenuation;
   friend class boost::serialization::access;
   template <class Archive>
   void serialize(Archive &ar, const unsigned int version)
   {
      ar &Rx_RF_Freq;
      ar &Rx1_RF_Gain;
      ar &Rx2_RF_Gain;
      ar &Tx_RF_Freq;
      ar &Tx1_Attenuation;
      ar &Tx2_Attenuation;
   }
} ConfigRFParams_t;

// ConfigRFParams_t RFParamsConfig;

class RJBoardMsg
{
public:
   RJBoardMsg();
   virtual ~RJBoardMsg();

   bool ParseMessage(RJBMSG_t *pMsg);
   int BuildMessage(unsigned char *Buf);
   void SetMsgType(U8 tp);

   U8 MsgType;
   RJBMSG_t RJBMsgBuf; // 临时存放消息
   RJBMSG_t *pRJBMsgBuf = &RJBMsgBuf;

private:
   void HandleMsgType(RJBMSG_t *pMsg);
   void HandleSearchMsg(RJBMSG_t *pMsg);
   void HandleAttackMsg(RJBMSG_t *pMsg);
   void HandlePDWSampleMsg(RJBMSG_t *pMsg);
   void HandleIQSampleMsg(RJBMSG_t *pMsg);
};
#pragma pack()
#endif /* RJBOARDMSG_H_ */
