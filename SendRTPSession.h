#pragma once

#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif // WIN32
#include "rtpsourcedata.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace jrtplib;

class CSendRTPSession : public RTPSession
{
public:
	CSendRTPSession(void);
	~CSendRTPSession(void);
public:
	void SetRtpUser(void* pUser);
public:
private:
	void*	m_pUser;
private:	
	void OnRTCPCompoundPacket(RTCPCompoundPacket *pack,const RTPTime &receivetime, const RTPAddress *senderaddress);
	void OnTimeout(RTPSourceData *srcdat);
	void OnBYEPacket(RTPSourceData *srcdat);
	void OnBYETimeout(RTPSourceData *srcdat);
};
