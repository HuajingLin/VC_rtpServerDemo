#include "StdAfx.h"
#include "AVideoSource.h"

extern unsigned short g_localport;
extern char* g_serverip;
CAVideoSource::CAVideoSource(void)
{
	m_bClosed = TRUE;
	m_portbase = 0;
	
	m_VidoeTimestamp = 0;
	m_AudioTimestamp = 0;
	memset(m_szSrcIP,0,IP_LEN);
}

CAVideoSource::~CAVideoSource(void)
{
	if(m_bClosed == FALSE)
		Close();
}
int  CAVideoSource::OpenUseRtsp(const char* rtspIP,const char *url,int rtspPort)
{
	int nPortNum = 4;
	m_portbase = get_local_valid_port(nPortNum);
	if(m_portbase <= 0)
		return -1;
	m_portbase -= (nPortNum-1);
	int videoPort;
	int audioPort;
	CRtsp rtsp;
	int ret = m_rtspClient.RtspConnect(rtspIP,url,m_portbase,rtspPort,&videoPort,&audioPort);
	if(ret != 0)
	{
		char szdebug[32];
		sprintf(szdebug,"rtsp connect fail.(%d)\n",ret);
		OutputDebugString(szdebug);
		return -2;
	}
	strncpy(m_szSrcIP,rtspIP,IP_LEN);
	m_videoSession.SetIsVideo(TRUE);
	ret = CreateRecvSession(&m_videoSession,rtspIP,videoPort);
	if(ret < 0)
		return ret;
	if( audioPort > 0)
	{
		ret = CreateRecvSession(&m_audioSession,rtspIP,audioPort);
		if(ret < 0)
			return ret;
	}
	m_rtspClient.SendPlay();
	m_bClosed = FALSE;
	return 0;
}
int  CAVideoSource::CreateRecvSession(CRecvRTPSession* session,const char* SvrIP,uint16_t nPort)
{
	int status;
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	
	sessparams.SetOwnTimestampUnit(1.0 / 90000.0);
	sessparams.SetUsePollThread(true);	//是否利用poll线程
	sessparams.SetMaximumPacketSize(MAX_PACKET_SIZE);	//允许的最大的packet size
	//sessparams.SetAcceptOwnPackets(true);		//将接受自己的packets并相应地将它们存储在源表
	if(session->GetIsVideo())
		transparams.SetPortbase(m_portbase);
	else
		transparams.SetPortbase(m_portbase+2);
	transparams.SetRTPReceiveBuffer(RTP_FRAME_BUF);
	
	status = session->Create(sessparams,&transparams);	
	if(status < 0)
	{
		rtp_check_error(status);
		return -3;
	}
	RTPIPv4Address	RtpAddr;
	u_long uDestIP = ntohl(inet_addr(SvrIP));
	RtpAddr.SetIP(uDestIP);
	RtpAddr.SetPort(nPort);
	status = session->AddDestination(RtpAddr);
	if(status < 0)
	{
		rtp_check_error(status);
		return -4;
	}
	session->SetRtpUser(this);
		
	if(session->GetIsVideo())
	{
		m_videoSessSend.SetRtpUser(this);
		CreateSendSession(&m_videoSessSend,"192.168.9.39",5566);
	}
	else
	{
		m_audioSessSend.SetRtpUser(this);
		CreateSendSession(&m_audioSessSend,"192.168.9.39",5568);
	}
	return 0;
}
void CAVideoSource::Close()
{
	m_bClosed = TRUE;
	
	m_videoSession.BYEDestroy(RTPTime(3,0),0,0);
	m_audioSession.BYEDestroy(RTPTime(3,0),0,0);
	m_rtspClient.CloseConnect();
}
int  CAVideoSource::CreateSendSession(RTPSession* session,const char* dstip,int dstport)
{
	int nPortNum = 2;
	uint16_t portbase = get_local_valid_port(nPortNum);
	if(portbase <= 0)
		return -1;
	portbase -= (nPortNum-1);
	char szDebug[32];
	sprintf_s(szDebug,32,"===CreatSendSession port:%d\n",portbase);
	OutputDebugString(szDebug);
	int status;
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0 / 90000.0);
	sessparams.SetAcceptOwnPackets(true);
	sessparams.SetUsePollThread(true); 
	sessparams.SetMaximumPacketSize(MAX_PACKET_SIZE);

	transparams.SetPortbase(portbase);//视频传输源端口
	status = session->Create(sessparams, &transparams);
	if (status < 0)
	{
		rtp_check_error(status);
		return -2;
	}
	RTPIPv4Address	RtpAddr;
	u_long uDestIP = ntohl(inet_addr(dstip));
	RtpAddr.SetIP(uDestIP);
	RtpAddr.SetPort(dstport);
	status = session->AddDestination(RtpAddr);
	if(status < 0)
	{
		rtp_check_error(status);
		return -3;
	}
	OutputDebugString("===create send session ok.\n");
	return 0;
}
bool CAVideoSource::AddSendDestination(RTPSession* session,const char* dstip,int dstport)
{
	int status;
	RTPIPv4Address	RtpAddr;
	u_long uDestIP = ntohl(inet_addr(dstip));
	RtpAddr.SetIP(uDestIP);
	RtpAddr.SetPort(dstport);
	status = session->AddDestination(RtpAddr);
	if(status < 0)
	{
		rtp_check_error(status);
		return false;
	}
	return true;
}


void CAVideoSource::ProcessRTPPacket(const RTPSourceData &srcdat,const RTPPacket &rtppack,BOOL bVideo)
{
	//char szDebug[32];
	//sprintf_s(szDebug,32,"===Got packet %u - %d\n",rtppack.GetExtendedSequenceNumber(),rtppack.GetPayloadLength());
	//OutputDebugString(szDebug);
	//return;
	uint32_t Timestamp = rtppack.GetTimestamp();
	if(bVideo)
	{
		if(m_VidoeTimestamp == 0)
			m_VidoeTimestamp = Timestamp;
		m_videoSessSend.SendPacket(rtppack.GetPayloadData(), rtppack.GetPayloadLength(),rtppack.GetPayloadType(), rtppack.HasMarker(), Timestamp-m_VidoeTimestamp);//3600
		m_VidoeTimestamp = Timestamp;
	}
	else
	{
		if(m_AudioTimestamp == 0)
			m_AudioTimestamp = Timestamp;
		m_audioSessSend.SendPacket(rtppack.GetPayloadData(), rtppack.GetPayloadLength(),rtppack.GetPayloadType(), rtppack.HasMarker(), Timestamp-m_AudioTimestamp);
		m_AudioTimestamp = Timestamp;
	}
	
}

void CAVideoSource::rtp_check_error(int rtperr)
{
	if (rtperr >= 0)
		return;
	char szdebug[128];
	sprintf_s(szdebug,128,"===source:%s,rtpERROR: %s\n",m_szSrcIP,RTPGetErrorString(rtperr).c_str());
	OutputDebugString(szdebug);
}
unsigned short CAVideoSource::get_local_valid_port(int num)
{
	struct sockaddr_in addrLocal;
	SOCKET sockTest;
	int nTry = 1000;
	int i = 0;
	int ret = 0;
	int LocalPort = g_localport;//已经被使用的最大端口
	//int nTempPort = 0;
	BOOL bFail = FALSE;
	while (true)
	{
		bFail = FALSE;
		for ( i=0; i < num; i++ )
		{
			LocalPort += 1;
			if( !bFail )
			{
				sockTest = socket(PF_INET,SOCK_DGRAM,0);
				memset(&addrLocal,0,sizeof(struct sockaddr_in));
				addrLocal.sin_family = AF_INET;
				addrLocal.sin_port = htons(LocalPort);
				addrLocal.sin_addr.s_addr = inet_addr(g_serverip);
				ret = bind( sockTest, (struct sockaddr *)&addrLocal, sizeof(struct sockaddr_in) );
				if(ret != 0)//SOCKET_ERROR
				{
					closesocket(sockTest);
					bFail = TRUE;
				}
				closesocket(sockTest);
			}
		}
		nTry -= 1;
		if(nTry < 0)
		{
			g_localport = LocalPort;
			return 0;
		}
		if(bFail)
			continue;
		g_localport = LocalPort;
		return LocalPort;
	}
}