#include "StdAfx.h"
#include "RecvRTPSession.h"
#include "AVideoSource.h"

CRecvRTPSession::CRecvRTPSession(void)
{
	m_pUser = NULL;
	m_bVideo= false;
}

CRecvRTPSession::~CRecvRTPSession(void)
{
}
void CRecvRTPSession::SetRtpUser(void* pUser)
{
	m_pUser = pUser;
}
void CRecvRTPSession::SetIsVideo(BOOL bVideo)
{
	m_bVideo = bVideo;
}
BOOL CRecvRTPSession::GetIsVideo()
{
	return m_bVideo;
}
void CRecvRTPSession::OnPollThreadStep()
{
	BeginDataAccess();

	// check incoming packets
	if (GotoFirstSourceWithData())
	{
		do
		{
			RTPPacket *pack;
			RTPSourceData *srcdat;
			srcdat = GetCurrentSourceInfo();
			while ((pack = GetNextPacket()) != NULL)
			{
				((CAVideoSource*)m_pUser)->ProcessRTPPacket(*srcdat,*pack,m_bVideo);
				DeletePacket(pack);
			}
		} while (GotoNextSourceWithData());
	}

	EndDataAccess();
}
void CRecvRTPSession::OnRTCPCompoundPacket(RTCPCompoundPacket *pack,const RTPTime &receivetime, const RTPAddress *senderaddress)
{
	//
}
void CRecvRTPSession::OnTimeout(RTPSourceData *srcdat)
{
	//等待超时
	printf("===Recv RTP Session OnTimeout\n");
}
void CRecvRTPSession::OnBYEPacket(RTPSourceData *srcdat)
{
	//收到BYE包
	printf("===Recv RTP Session OnBYEPacket\n");
}
void CRecvRTPSession::OnBYETimeout(RTPSourceData *srcdat)
{
	//发送BYE包后超过
	printf("===Recv RTP Session OnBYETimeout\n");
}