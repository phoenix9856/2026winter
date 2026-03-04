
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
//////////////////////////////////////////////////////////////////////

#ifndef UDPInterface_H
#define UDPInterface_H

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "CEMCGlobalDef.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/types.h>
#endif

using namespace std;

class CUDPInterface
{
public:
	CUDPInterface();
	virtual ~CUDPInterface();
	int Init(unsigned char ucAgentID, string BoardIP,int m_usSrcPort);
	int Create(unsigned short usPort, short &usSocket);
	int Wait(void);
	int Wait(int nTimeout, int &nTimeElapse);
	int RecvFrom(unsigned char *Buf, int nBufLen, string &strSrcIP, unsigned short &usSrcPort);
	int SendTo(unsigned char *Buf, int nBufLen, string &strDstIP, unsigned short usDstPort);
	void Close(void);

	inline short GetSockFD(void)
	{
		return m_usSrcSocket;
	};

	short m_usSrcSocket;
	string m_strSrcIP;
	string m_strBoardIP;
	unsigned short m_usSrcPort;
};

#endif
//
