/* ==========================================================================
 * Copyright (c) 2020, CEMC Group, AIRC, NIIDT.
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
 * must display the following acknowledgment:
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
 * ==========================================================================
 * Author: Yiming Hu
 * Time:   01/26/2022
 *
 */

#ifndef _telemetry_local_H_
#define _telemetry_local_H_

#include <string>
#include <vector>

#define MONITOR_TREE_MSG 0x61
#define MONITOR_PS_MSG 0x62
#define MONITOR_ANTENNA_MSG 0x63
#define MONITOR_NETWORK_MSG 0x64
#define MONITOR_UAV_STATUS_MSG 0x65

#define FIND_TASK 0x01
#define FIX_TASK 0x02
#define TARGET_TASK 0x03
#define TRACK_TASK 0x04
#define ENGAGE_TASK 0x05
#define ASSESS_TASK 0x06

/* 心跳包之行为树结构体 */
typedef struct
{
	int taskname;			   		//当前任务类型
	int radarfound_flag;		   	//当前侦察结果
	int plan_size;				   	//路径规划段数
	int plan_index;				   	//当前路径规划段序号
	float plan_start_longitude;	   	//路径规划起点经度
	float plan_start_latitude;	   	//路径规划起点纬度
	float plan_start_height;	   	//路径规划起点纬度
	float plan_end_longitude;	   	//路径规划终点经度
	float plan_end_latitude;	   	//路径规划终点纬度
	float plan_end_height;		   	//路径规划终点高度
	float phantom_start_longitude; 	//虚拟航迹起点经度
	float phantom_start_latitude;  	//虚拟航迹起点纬度
	float phantom_start_height;	   	//虚拟航迹起点纬度
	float phantom_end_longitude;   	//虚拟航迹终点经度
	float phantom_end_latitude;	   	//虚拟航迹终点纬度
	float phantom_end_height;	   	//虚拟航迹终点高度
	float uav_longitude;		   	//当前位姿经度
	float uav_latitude;			   	//当前位姿纬度
	float uav_height;			   	//当前位姿纬度
	float uav_angle;			   	//当前位姿航向角
	float uav_speed;			   	//当前无人机速度
	float pw[4];	   			   	//分选脉宽
	float pri[4];	   			   	//分选PRI1
	float radar_longitude;  	   	//雷达经度
	float radar_latitude;  	   	   	//雷达维度
	float radar_height;  	   		//雷达高度
	float path_start_longitude;  	//总路径起点经度
	float path_start_latitude;  	//总路径起点纬度
	float path_start_height;  	   	//总路径起点高度
	float path_end_longitude;  	   	//总路径终点经度
	float path_end_latitude;  	   	//总路径终点纬度
	float path_end_height;  	   	//总路径终点高度
	int pulse_num[4];				//分选脉冲个数
	int total_pulse_num;			//总脉冲个数
	int sorted_pulse_num;			//已分选脉冲总个数
} Tree_msg;
/* 心跳包之PS侧结构体 */
typedef struct
{
	int PL_status;		   //PL侧工作状态
	int SIG_status;			//脉冲锁定状态
	int jammtype;		   //当前干扰类型
	int jamming_flag;	   //干扰执行状态
	float freq;			   //中心频点
	float bandwidth;	   //带宽
	float trasmitter_gain; //发射增益
	float receiver_gain;   //接收增益
	float delay[20];		   //当前干扰点时延(相邻2个)
	float doppler[20];	   //当前干扰点多普勒（相邻2个）
	float FPGA_temp;		//FPGA温度
	float FPGA_CORE_V;		//FPGA内核电压
	float FPGA_SUP_V;		//FPGA辅助电压
	float SIG_MAX_AMP;		//信号最大幅值
	int pulse_sele_mode[3];	//脉宽选择模式
	int pulse_sele_list[7];	//脉宽选择列表
	int jamm_pulse_interval;//干扰脉冲间隔
	int jamm_trace_num;		//虚拟航迹条数
	int jamm_trace_interval;//虚拟航机间隔
} PS_msg;

/* 心跳包之天线结构体 */
typedef struct
{
	int antenna_status; //天线电机工作状态
	float antennaAngle; //当前天线角度
	float controlAngle; //电机接收到的控制角度
} Ant_msg;
/* 心跳包之飞控结构体 */
typedef struct
{
	int uav_status; //天线电机工作状态
} UAV_msg;

/* 心跳包之网络链路结构体 */
typedef struct
{
	int network_flag;
} Net_msg;

/* 指令之网络链路结构体 */
typedef struct
{
	int control_flag;
} Control_msg;

#endif
