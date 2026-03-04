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
 * last updated:2020.11.25 11:45
 */
//
//////////////////////////////////////////////////////////////////////

#include "CEMCapi.h"

CEMC::CEMCAPI::CEMCAPI(string platformIDInput, string processName)
{
  platformID = platformIDInput;     #xys：平台输入ID默认2201；process name为A，对应OODA的A
// TODO Auto-generated constructor stub
# xys：根据config_sim和sim_grpc的不同来区分
#if (CONFIG_SIM == 0)
  if (RJB.Open(processName))
    std::cout << "RJB open success" << std::endl;
  else
    std::cout << "RJB open failed" << std::endl;
#else
#if (SIM_GRPC == 1)  # xys：如果仿真为1，则执行仿真
#else
  DDSController::Get().init(platformID);
  // 中间调用DDS接口
  std::cout  << "[GetDataFromSim][getPDW]:bbbbbbbbbbbbbbbbbbbbbbbb" << std::endl;
  # xys：调用dds接口？
  ModelFunc::Get().engineModelFunc.Start();
  std::cout  << "[GetDataFromSim][getPDW]:cccccccccccccccccccccccc" << std::endl;
#endif
#endif
}

CEMC::CEMCAPI::~CEMCAPI()
{
// TODO
#if (CONFIG_SIM == 0)
  RJB.Close();
#endif
}

#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
# xys：仿真和实物并存，就加载参数？
std::string CEMC::Config::readIP()
{
  std::string EnvIP;
  tinyxml2::XMLDocument config;
  tinyxml2::XMLError ret;
  // 读取配置文件,包含行为树ip,电机控制ip,ps ip以及无人机状态ip及各端口
  ret = config.LoadFile("/opt/micros/share/empt/rjboard_config/config.xml");
  if (ret != 0)
  {
    config.Clear();
    std::cout << "Load para config failed!" << std::endl;
  }

  tinyxml2::XMLElement *root = config.RootElement();
  if (root == NULL)
  {
    std::cout << "config file is invalid" << std::endl;
  }
  tinyxml2::XMLElement *elem = root->FirstChildElement();
  tinyxml2::XMLElement *IPConfig = elem->FirstChildElement("IPConfig");
  EnvIP = IPConfig->FirstChildElement("sim_ip")->GetText();
  EnvIP += ":50051";
  return EnvIP;
}

grpc::ChannelArguments CEMC::Config::getch()
{
  grpc::ChannelArguments ch_args;
  ch_args.SetMaxReceiveMessageSize(10000000);
  return ch_args;
}
#endif
#endif
bool CEMC::CEMCAPI::heartBeat(Tree_msg tree_msg)
{
#if (CONFIG_SIM == 1)
  std::cout << "Using simulation--302" << std::endl;
  return true;
#else
  RJB.HeartBeat(tree_msg);
  return true;
#endif
  return false;
}

bool CEMC::CEMCAPI::setRecon(SimReconParas &reconPara)
# xys：输入侦查配置参数，知操控引擎层根据宏定义任务向仿真或实物下发侦察/PDW参数
{
  switch (reconPara.reconMode)
  {
  case RADARSEARCH:
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
    env.setRecon(reconPara);
#else
  #xys： 调用外部函数
    ModelFunc::Get().lq301ModelFuncLK.SetReconControlParameters(platformID, reconPara);
    std::cout << "\n\n\n====================================\n下发干扰参数成功\n===========================" << std::endl;
#endif
    break;
#else    # xys： 实物不等于1时；这里是直接又定义getSearchResult？
        # xys：认知操控引擎层根据宏定义任务获取仿真或实物返回的侦察结果消息，并对消息进行解析，
        # xys：发现目标searchFlag为true，未发现目标searchFlag为false。
    bool setSearchResult;
    RJB.setSearchMode(reconPara);
    break;
#endif
  case PDWSAMPLE:
  {
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
    std::cout << "Using simulation-133" << std::endl;
    env.setRecon(reconPara);
#else
    ModelFunc::Get().lq301ModelFuncLK.SetReconControlParameters(platformID, reconPara);
#endif
    break;
#else
    bool setSampleResult;
    # xys:RJBoarder 发送PDW采样消息函数，reconPara采样配置参数
    # xys：PDWSampleMode：物理域向硬件系统层发送PDW采样消息任务，
    # 并解析采样配置参数reconPara到结构体RJBoardMsg的消息包中
    RJB.setPDWSampleMode(reconPara);
    break;
#endif
  }
  case PDWSETMODE:
  {
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
    std::cout << "Using simulation-133" << std::endl;
    env.setRecon(reconPara);
#else
    ModelFunc::Get().lq301ModelFuncLK.SetReconControlParameters(platformID, reconPara);
#endif
    break;
#else
    bool setSampleResult;
    #  这个是？
    RJB.setPDWSETMode(reconPara);
    break;
#endif
  }
  default:
    std::cout << "Invalid Recon Type" << std::endl;
    return false;
  }
  return true;
}

bool CEMC::CEMCAPI::setJamm(SimJammParas &jammingPara)
# xys：setJamm：为认知操控引擎层根据宏定义任务向仿真或实物下发干扰参数，jammingPara为干扰配置参数
{
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
  bool jamm_set_flag;
  # 初始化一个jamm_set_flag，为true
  jamm_set_flag = env.setJamm(jammingPara);
  if (jamm_set_flag)
  {
    std::cout << "jamm_set_flag success" << std::endl;
  }
  else
  {
    std::cout << "jamm_set_flag failed" << std::endl;
    return false;
  }
  for (int i = 0; i < 10; i++)
  {
    # xys：输出延迟的时间
    std::cout << "delay " << i << " is " << jammingPara.deceiveJammingParas[i].delayTime << std::endl;
  }
  std::cout << "env.setJamm complete" << std::endl;
#else
  ModelFunc::Get().lq301ModelFuncLK.setJam(platformID, jammingPara);
#endif
  return true;
#else
  // sim_JammParameters jammingPara;
  bool jammingFinishedFlag;

  RJB.setJammMode(jammingPara);
  return true;
#endif
  return false;
}

bool CEMC::CEMCAPI::interruptJamm()
# xys：探测是否干扰被打断？
{
#if (CONFIG_SIM == 1)
  std::cout << "Using simulation--302" << std::endl;
  return true;
#else
  bool jammingFinishedFlag;
  RJB.interruptJamm();
  return true;
#endif
  return false;
}

bool CEMC::CEMCAPI::setAntenna(SimAntennaParas &antennaParas)
# xys：setAntenna：该函数主要功能为认知操控引擎层根据宏定义任务向仿真或实物下发天线参数；
# antennaPara天线配置参数
{
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
  bool antenna_set_flag;
  antenna_set_flag = env.setAntenna(antennaParas);
  if (antenna_set_flag)
  {
    std::cout << "antenna_set_flag success" << std::endl;
    return true;
  }
  else
  {
    std::cout << "antenna_set_flag failed" << std::endl;
    return false;
  }
#else
  ModelFunc::Get().lq301ModelFuncLK.SetAntennaPara(platformID, antennaParas);
#endif
  // return true;
#else
  // RJB.setAntenna(antennaParas);
#endif
  return false;
}

# xys：检测天线是否得到参数？
bool CEMC::CEMCAPI::getAntennaRst(SimAntennaParas &antennaParas)
{
#if (CONFIG_SIM == 0)
  if (RJB.GetAntennaRst(antennaParas))
    return true;
  else
    return false;
#endif
}

# xys：CEMCAPI层：侦查/干扰控制、PDW/IQ获取等功能的封装接口函数

bool CEMC::CEMCAPI::getPDWdata(SimPDWArray &vecPDWData, std::string platformID, int &total_times)
# xys：PDWs为PDW数组，platformID为无人机ID，total_times为实物上传PDW时的总包数？
# xys：在认知操控引擎层，根据宏定义任务获取仿真或者实物获得的PDW采样数据
{  # xys:输出字符串
  std::cout  << "[GetDataFromSim][getPDW]:66666666666666666666" << std::endl;
#if (CONFIG_SIM == 1)       #xys:如果不是实物，输出777？
  std::cout  << "[GetDataFromSim][getPDW]:7777777777777777777777" << std::endl;
  #if (SIM_GRPC == 1)       #xys：如果是仿真，输出888，开始循环
    std::cout  << "[GetDataFromSim][getPDW]:8888888888888888888" << std::endl;
    for (int i = 0; i < 1; i++)
    {
      std::cout << "CEMC:getPDW from Sim running" << std::endl;
      // 获取侦查模式1数据
      // std::vector<SimPulseDescriptionWord> vecPDWData;
      if (env.getPDWdata(platformID, vecPDWData.aPDWArray))  
      #xys：前后getPDWdata的区别？
      #xys：输入：无人机ID、getpdw的数据来源；env和getPDWdata为基类和从基类的关系？
      {
        std::cout << "lenth: " << vecPDWData.aPDWArray.size() << std::endl;
        std::cout << "get pdw lenth: " << vecPDWData.aPDWArray.size() << std::endl;
        return true;
      }
      else
      {
        std::cout << "Get PDW Sample data failed" << std::endl;
        return false;
      }
    }
  #else     # xys：如果不是仿真，获取vecPDWData，获取PDW数组
            # xys：如何获取---ModelFunc
    std::cout  << "[GetDataFromSim][getPDW]:09999999999999999" << std::endl;
    # xys:函数位置在哪
    vecPDWData = ModelFunc::Get().lq301ModelFuncLK.getPDWData(platformID);
    std::cout  << "[GetDataFromSim][getPDW]:aaaaaaaaaaaaaaaa" << std::endl;
    std::cout << "wpwpw 获取PDW 数据 get pdw lenth: " << vecPDWData.aPDWArray.size() << "platformID is " << platformID << std::endl;
    if (vecPDWData.aPDWArray.size()>0)
        return true;
      
  #endif
#else
  unsigned char *RvBuf;
  int DataLen;
  unsigned char DataType;
  // RJB.GetData(RvBuf,DataLen,DataType);
  bool pdwrst;

  if (RJB.GetPDWRst(vecPDWData, total_times))
  {
    return true;
  }
  else
  {
    std::cout << "Get PDW Sample data failed" << std::endl;
    return false;
  }
#endif
}

# xys：getSearchResult：认知操控引擎层根据宏定义任务获取仿真或实物返回的侦察结果消息，
# 并对消息进行解析，发现目标searchFlag为true，未发现目标searchFlag为false
bool CEMC::CEMCAPI::getSearchResult(bool &searchFlag, std::string platformID)
{
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
  for (int i = 0; i < 1000000; i++)
  {
    // 获取侦查模式2数据
    bool ret = false;
    # xys：调用RJBoarder的函数getSearchResult
    # xys：RJBoarder RJB;
    env.getSearchResult(platformID, ret);
    std::cout << "SearchResult: " << ret << std::endl;
    usleep(50000);
  }
#endif
#else
  if (RJB.GetSearchRst(searchFlag))
    return true;
  else
  {
    std::cout << "Get Search rst failed" << std::endl;
    return false;
  }
#endif
  return false;
}

bool CEMC::CEMCAPI::getJammResult(bool &jammingFinishedFlag, std::string platformID)
# xys：认知操控引擎层获取实物中返回的干扰结果消息，并对消息进行解析，
# xys:干扰成功jammingFinishedFlag为true，干扰失败jammingFinishedFlag为false
{
#if (CONFIG_SIM == 0)
  if (RJB.GetJammRst(JammResult))
  {
    jammingFinishedFlag = JammResult;
    return true;
  }
  else
  {
    std::cout << "Get jamm rst failed" << std::endl;
    return false;
  }
#endif
  return false;
}

bool CEMC::CEMCAPI::getUAVdata(UAV_Data &uav_data)
# xys:getUAVdata:从飞控接收进程获取无人机的位姿数据结果返回消息
# xys:无人机位姿参数uav_data
{
#if (CONFIG_SIM == 0)
  if (RJB.getUAVdata(uav_data))
    return true;
  else
    return false;
#endif
  return true;
}

bool CEMC::CEMCAPI::getAgentState(bState &state)

# xys:?
{
#if (CONFIG_SIM == 1)
#if (SIM_GRPC == 1)
  std::cout << "CEMC platformID is " << platformID << std::endl;
  LocationReply currentObs = env.getAgentLocation(platformID);
  PTZInfo currentAngle = env.getAgentAngle(platformID);
  PTZInfo currentSpeed = env.getActorVelocity(platformID);

  state.position[0] = currentObs.x;
  state.position[1] = currentObs.y;
  state.position[2] = currentObs.z;
  state.course = -currentAngle.z * M_PI / 180;
  state.airspeed = sqrt(currentSpeed.x * currentSpeed.x + currentSpeed.y * currentSpeed.y);
#else
  std::cout << "dds getAgentState" << std::endl;
  InsDataSpace::Location locationreply = ModelFunc::Get().lq301ModelFuncLK.getLocation(platformID);
  state.position[0] = locationreply.longitude;
  state.position[1] = locationreply.latitude;
  state.position[2] = locationreply.altitude;

#endif
  return true;
#else
  return false;
#endif
}
