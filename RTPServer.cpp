#include "StdAfx.h"
#include "RTPServer.h"

#pragma comment(lib,"Ws2_32.lib")

unsigned short g_localport = 3999;
char* g_serverip=NULL;
CRTPServer::CRTPServer(void)
{
	m_bSvrStop = FALSE;
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);
	InitializeCriticalSection(&m_cslistAVSour);
}

CRTPServer::~CRTPServer(void)
{
	if(m_bSvrStop==FALSE)
		ServerStop();
	if(g_serverip)
	{
		delete [] g_serverip;
		g_serverip = NULL;
	}
	DeleteCriticalSection(&m_cslistAVSour);
	WSACleanup();
}
bool CRTPServer::ServerStart()
{
	g_serverip = new char[IP_LEN];
	memset(g_serverip,0,IP_LEN);
	sprintf_s(g_serverip,IP_LEN,"192.168.3.247");
	m_RtspServer.StartServer(g_serverip,554);
	CAVideoSource* pAVS = new CAVideoSource();
	int ret = pAVS->OpenUseRtsp("192.168.9.218","/1/master",554);
	if(ret < 0)
	{
		char szdebug[32];
		sprintf_s(szdebug,32,"===open rtsp fail(%d)\n",ret);
		OutputDebugString(szdebug);
		delete pAVS;
		return false;
	}
	EnterCriticalSection(&m_cslistAVSour);
	m_listAVSource.push_back(pAVS);
	LeaveCriticalSection(&m_cslistAVSour);
	
	return true;
}
void CRTPServer::ServerStop()
{
	m_bSvrStop = TRUE;

	list<CAVideoSource *>::iterator it;
	EnterCriticalSection(&m_cslistAVSour);	
	for (it = m_listAVSource.begin(); it != m_listAVSource.end(); ++it)
	{
		(*it)->Close();
		delete (*it);
	}
	m_listAVSource.clear();
	LeaveCriticalSection(&m_cslistAVSour);
}
//============private=========
