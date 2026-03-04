#ifndef CEMCGlobalDef_H
#define CEMCGlobalDef_H

#include <stdio.h>
#include <string.h>

#define MTU 512

typedef int STATUS;
typedef char D8;
typedef short int D16;
typedef int D32;
typedef long DL32;
typedef unsigned char U8;
typedef unsigned short int U16;
typedef unsigned int U32;
typedef unsigned long UL32;
typedef unsigned long long U64;

typedef signed char S8;
typedef signed short S16;
typedef signed long S32;
typedef signed long long S64;

// #define LOCAL_IP "192.168.1.86"
#define LOCAL_IP "127.0.0.1"
#define DEBUG_SHOW 1
#define DEBUG_SHOW2 1
#define DEBUG_SHOW3 1
#define DEBUG_SHOW4 1

#if (DEBUG_SHOW)
#define DEBUG_LINE(s, args...) printf(s, ##args);
#else
#define DEBUG_LINE(s, args...) //printf("");
#endif

#if (DEBUG_SHOW2)
#define DEBUG_LINE2(s, args...) printf(s, ##args);
#else
#define DEBUG_LINE2(s, args...) // printf("");
#endif

#if (DEBUG_SHOW3)
#define DEBUG_LINE3(s, args...) printf(s, ##args);
#else
#define DEBUG_LINE3(s, args...) //printf("");
#endif

#if (DEBUG_SHOW4)
#define DEBUG_LINE4(s, args...) printf(s, ##args);
#else
#define DEBUG_LINE4(s, args...) //printf("");
#endif

extern float Spec_Power_Base;
extern float Spec_AGC;
extern float IF_AGC;

extern int Earth_Station_ID;

enum CEMC_RETURN_VALUE
{
	CEMC_ECONNECT = -2,
	CEMC_ERROR = -1,
	CEMC_CONTINUE = 0,
	CEMC_SUCCESS = 1
};

enum REG_DEV_TYPE
{
	DEV_TYPE_2D = 1,
	DEV_TYPE_2DHJ = 2,
	DEV_TYPE_DSF = 3,
	DEV_TYPE_DSB = 4
};

enum CEMC_MSG_PARAM_CODE
{
	CEMC_MSG_VAR_QUERY_CODE = 0x1,
	CEMC_MSG_VAR_QUERY_RPS_CODE = 0x2,
	CEMC_MSG_VAR_SET_CODE = 0x3,
	CEMC_MSG_VAR_SET_RPS_CODE = 0x4,
	CEMC_MSG_TPD_SET_CODE = 0x5,
	CEMC_MSG_TPD_SET_RPS_CODE = 0x6

};

#endif
