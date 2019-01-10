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
//#define RECV_BUFFER_LEN 64*1024
#define MAX_PACKET_SIZE 2*1024
#define RTP_FRAME_BUF 64*1024
#define NAL_SPS 0x67
#define NAL_PPS 0x68

class CRecvRTPSession : public RTPSession
{
public:
	CRecvRTPSession(void);
	~CRecvRTPSession(void);
public:
	void SetRtpUser(void* pUser);
	void SetIsVideo(BOOL bVideo);
	BOOL GetIsVideo();
public:
private:
	void* m_pUser;
	BOOL  m_bVideo;// « ”∆µsession
private:
	void OnPollThreadStep();
	void OnRTCPCompoundPacket(RTCPCompoundPacket *pack,const RTPTime &receivetime, const RTPAddress *senderaddress);
	void OnTimeout(RTPSourceData *srcdat);
	void OnBYEPacket(RTPSourceData *srcdat);
	void OnBYETimeout(RTPSourceData *srcdat);
};
