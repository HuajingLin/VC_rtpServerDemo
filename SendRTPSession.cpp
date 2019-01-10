#include "StdAfx.h"
#include "SendRTPSession.h"
#include "AVideoSource.h"

CSendRTPSession::CSendRTPSession(void)
{
	m_pUser = NULL;
}

CSendRTPSession::~CSendRTPSession(void)
{
	
}
void CSendRTPSession::SetRtpUser(void* pUser)
{
	m_pUser = pUser;
	
}

void CSendRTPSession::OnRTCPCompoundPacket(RTCPCompoundPacket *pack,const RTPTime &receivetime, const RTPAddress *senderaddress)
{
	//	
}

void CSendRTPSession::OnTimeout(RTPSourceData *srcdat)
{
	//等待超时
	printf("===Send RTP Session OnTimeout\n");
}
void CSendRTPSession::OnBYEPacket(RTPSourceData *srcdat)
{
	//收到断开包
	printf("===Send RTP Session OnBYEPacket\n");
}
void CSendRTPSession::OnBYETimeout(RTPSourceData *srcdat)
{
	//发送BYE包后超过
	printf("===Send RTP Session OnBYETimeout\n");
}