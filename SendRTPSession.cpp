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
	//�ȴ���ʱ
	printf("===Send RTP Session OnTimeout\n");
}
void CSendRTPSession::OnBYEPacket(RTPSourceData *srcdat)
{
	//�յ��Ͽ���
	printf("===Send RTP Session OnBYEPacket\n");
}
void CSendRTPSession::OnBYETimeout(RTPSourceData *srcdat)
{
	//����BYE���󳬹�
	printf("===Send RTP Session OnBYETimeout\n");
}