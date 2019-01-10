#pragma once

#include <winsock2.h>
#include "rtsp.h"
#include <list>
using namespace std;

#pragma pack(1)
//完成例程用到的
typedef struct
{
	WSAOVERLAPPED overlap;
	WSABUF        Buffer;
	char          szMessage[SEND_BUF_MAX];
	DWORD         NumberOfBytesRecvd;
	DWORD         Flags;
	SOCKET        sClient;
}PER_IO_OPERATION_DATA;

typedef struct
{
	SOCKET	sSocket;
	char*	pBuf;
	int		nLen;
}DATA_NODE;

typedef struct
{
	SOCKET	nSocket;
	int		nUserID;
	int		nPermit;
}CLIENT_NODE;


#pragma pack()
class CRtspServer
{
public:
	CRtspServer(void);
	~CRtspServer(void);
	BOOL StartServer(const char* IP,int nRtspPort);
	void StopServer();
private:
	HANDLE	m_hListen;
	HANDLE	m_hHandle;
	BOOL	m_bClosed;
	SOCKET	m_sListen;
public:
	list<DATA_NODE>		m_listData;
private:
	static unsigned int __stdcall ListenThread(void *param);
	void ListenConnect();
	static unsigned int __stdcall HandleDataThread(void *param);
	void HandleDataThrd();
	//
	int tctp_getitem_str(char *src, char *find_str, char *dest,unsigned char space_flag);
	int tctp_getitem_int(char *src, char *find_str);
	int tctp_send_bad_request(char *in_buf,int cseq, int sockfd);
	int tctp_response_recv_opt(char *in_buf,int cseq, char *cdate, char *cpublic, int sockfd);
	int tctp_response_recv_describe(char *in_buf,int cseq, char *cdate,
		char * cbase, char *ctype,char * clength,char *csdp,int sockfd);
	int tctp_response_recv_setup_s1(char *in_buf,int cseq, char *cdate,
		char * destIP, char * srcIP,char * cliPort,int svrPort,char * sessionId,int sockfd);
	int tctp_multi_response_recv_setup_s1(char *in_buf,int cseq, char *cdate,
		char * destIP, char * srcIP,int multiPort,unsigned char multiTtl,char * sessionId,int sockfd);
	int tctp_response_recv_play(char *in_buf,int cseq, char *cdate,
		char * range, char * rtpinfo,char * sessionId,int sockfd);
	int tctp_response_recv_stop(char *in_buf,int cseq, char *cdate, int sockfd);
	int tctp_getitem_connect_type(char *src, char *dest);
	int ReplaceStr(char *sSrc, char *sMatchStr, char *sReplaceStr);
};
