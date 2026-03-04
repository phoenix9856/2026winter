
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
* last updated:2020.11.24 11:45
*/

#ifndef _CEMCAPI_H_
#define _CEMCAPI_H_

#include "CEMCGlobalDef.h"



#include "PDW.h"
#include "JointPlanner_defined_data.h"

#include <iostream>

#include "shm_comm/shm_publisher.hpp"
#include "shm_comm/shm_subscriber.hpp"
#include <micros_episodefs/episode.h>

#include "EMStruct.h"

#include "unistd.h"
#include <fstream>
#if (SIM_GRPC==1)
#include "Environment.h"
#else
#include "DDSController.h"
#include "DDSUtils.h"
#include "LQ301ModelFunc.h"
#include "ModelFunc.h"
#endif
#include "InsDataStructWithBoost.h"

#include "tinyxml2.h"
#include "blackboard_data.h"

namespace CEMC
{
    //定义传输结构体
    struct SearchParameters
    {
        U16 Frequency;
        U16 Bandwidth;
        U16 Gain;
        U32 StartTime;
        U16 DwellTime; //ms
        string SearchMode;
        string TargetProperties;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &Frequency;
            ar &Bandwidth;
            ar &Gain;
            ar &StartTime;
            ar &DwellTime;
            ar &SearchMode;
            ar &TargetProperties;
        }
    };

    struct SampleParameters
    {
        U16 RFFrequency;
        U16 IFFrequency;
        U16 SampleRate;
        U16 Gain;
        U32 StartTime;
        U16 DwellTime;  //ms
        U16 SampleMode; //1 for pdw; 2 for IQData
        U16 Bandwidth;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &RFFrequency;
            ar &IFFrequency;
            ar &SampleRate;
            ar &Gain;
            ar &StartTime;
            ar &DwellTime;
            ar &SampleMode;
            ar &Bandwidth;
        }
    };

    struct JammParameters
    {
        U16 Frequency;
        U16 Gain;
        U32 StartTime;
        U16 DwellTime;
        string JammModeParameters;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &Frequency;
            ar &Gain;
            ar &StartTime;
            ar &DwellTime;
            ar &JammModeParameters;
        }
    };

    struct SpectrumRes
    {
        U16 Frequency;
        U16 Bandwidth;
        U32 Time;
        U16 NumPoints;
        U16 SpecData[8192];

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &Frequency;
            ar &Bandwidth;
            ar &Time;
            ar &NumPoints;
            ar &SpecData;
        }
    };

    struct _radarParameters
    {
        U16 radar_RF_Freq;
        U32 radarPulsewidth;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &radar_RF_Freq;
            ar &radarPulsewidth;
        }
    };

    struct ConfigRFParameters
    {
        double Rx_RF_Freq;
        double Rx1_RF_Gain;
        double Rx2_RF_Gain;

        double Tx_RF_Freq;
        double Tx1_Attenuation;
        double Tx2_Attenuation;

    private:
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
    };

    struct Common_msg
    {
        int num;
        string str_message;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &num;
            ar &str_message;
        }
    };

    template <class T>
    string serializeDataToString(T data)
    {
        std::ostringstream os;
        boost::archive::text_oarchive oarchive(os);
        oarchive << data;               //序列化到一个ostringstream里面
        std::string results = os.str(); //content保存了序列化后的数据。
        return results;
    };

    template <class T>
    void deSerializeDataFromString(const std::string src, T &des)
    {
        std::istringstream is(src);
        boost::archive::text_iarchive iarchive(is);
        iarchive >> des;
    };
#if (CONFIG_SIM == 1)
    #if (SIM_GRPC==1)
        class Config{
            public:
                static std::string readIP();
                static grpc::ChannelArguments getch();
            };

        class ENVinit : public Environment{
            public:
                ENVinit():Environment(Config::readIP(),Config::getch()){}
            };
    #endif
#endif
    class CEMCAPI
    {
    public:
        CEMCAPI(string platformIDInput = "2201", string processName = "A");
	virtual ~CEMCAPI();

        bool heartBeat(Tree_msg tree_msg);
        bool setRecon(SimReconParas &reconPara);
        bool setJamm(SimJammParas &jammingPara);
        bool interruptJamm();
        bool setAntenna(SimAntennaParas &antennaParas);
        // bool setRFConfig(ConfigRFParameters &RFPara); //TO DO

        bool getSearchResult(bool &searchFlag, std::string platformID);
        bool getPDWdata(SimPDWArray &vecPDWData, std::string platformID, int &total_times);
        bool getAntennaRst(SimAntennaParas &antennaParas);
        bool getJammResult(bool &jammingFinishedFlag, std::string platformID);
        // bool getSpectrum(SpectrumRes &Spectrum, std::string platformID); // TO DO

        bool getUAVdata(UAV_Data &uav_data);

        void Common_msgHandler(const Common_msg &msg);

        bool getAgentState(bState &state);
        

        string platformID;
    private:
#if (CONFIG_SIM == 1)
#if (SIM_GRPC==1)
        // 连接UE环境
        // grpc::ChannelArguments ch_args;
        // Environment env = Environment("0.0.0.0:50051", ch_args);
        
        
        ENVinit env;
        Common_msg ResponseMsg;

        shm_comm::ShmPublisher<Common_msg> *ptrPub;
        shm_comm::ShmSubscriber<Common_msg> *ptrSub;

        micros_episode::Episode episode;

        bool EmitterExisted = false;
        bool JammResult = false;
#else
        
#endif
#else
        RJBoarder RJB;

        Common_msg ResponseMsg;

        bool EmitterExisted = false;
        bool JammResult = false;
#endif
    };
}

#endif /* _CEMCAPI_H_ */
