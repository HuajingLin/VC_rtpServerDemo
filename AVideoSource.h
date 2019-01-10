#pragma once
#include "Rtsp.h"
#include "RecvRTPSession.h"
#include "SendRTPSession.h"

#define IP_LEN 32

class CAVideoSource
{
public:
	CAVideoSource(void);
	~CAVideoSource(void);
public:
	int  OpenUseRtsp(const char* rtspIP,const char *url,int rtspPort);
	void Close();
	int  CreateRecvSession(CRecvRTPSession *session,const char* SvrIP,uint16_t nPort);
	int  CreateSendSession(RTPSession* session,const char* dstip,int dstport);
	bool AddSendDestination(RTPSession* session,const char* dstip,int dstport);
	void ProcessRTPPacket(const RTPSourceData &srcdat,const RTPPacket &rtppack,BOOL bVideo);
private:
	void rtp_check_error(int rtperr);
	unsigned short get_local_valid_port(int num);
	
private:
	CRtsp m_rtspClient;
	CRecvRTPSession	m_videoSession;
	CRecvRTPSession	m_audioSession;
	CSendRTPSession	m_videoSessSend;
	CSendRTPSession	m_audioSessSend;
	uint16_t m_portbase;
	BOOL	m_bClosed;	
	//
	//int m_videoPort;	//设备RTP视频端口
	//int m_audioPort;	//设备RTP音频端口
	//
	char m_szSrcIP[IP_LEN];
	uint32_t m_VidoeTimestamp;
	uint32_t m_AudioTimestamp;
};
