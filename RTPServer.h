#pragma once
#include "winsock2.h"
#include "RtspServer.h"
#include "AVideoSource.h"
#include <list>
using namespace std;

class CRTPServer
{
public:
	CRTPServer(void);
	~CRTPServer(void);
public:
	bool ServerStart();
	void ServerStop();
private:
	void rtp_check_error(int rtperr);
private:
	BOOL m_bSvrStop;
	CRtspServer				m_RtspServer;
	list<CAVideoSource*>	m_listAVSource;	//音视频源列表
	CRITICAL_SECTION		m_cslistAVSour;
};
