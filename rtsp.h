#pragma once
#define SEND_BUF_MAX 2048
#define URL_LEN 32
#define SESS_ID_LEN 16

#define RTSP_STATUS_UNKNOWN		0x00
#define RTSP_STATUS_SETUP		0x01
#define RTSP_STATUS_PLAY		0x02
#define RTSP_STATUS_PAUSE		0X03
#define RTSP_STATUS_STOP		0X04 
#define RTSP_STATUS_CONNECTING	0x05   

class CRtsp
{
public:
	CRtsp(void);
	~CRtsp(void);
public:
	int  RtspConnect(const char *pDestIp,const char *url, int nSrcPort,int nDesPort,int *video_port,int *audio_port);
	void SendPlay();
	void CloseConnect();
private:
	int send_opt(char *in_buf, int sockfd);
	int send_describe(char *in_buf, int sockfd, bool bAuthorized=false);
	int send_setup_s1(char *in_buf, int sockfd,int port);
	int send_setup_s2(char *in_buf, int sockfd,int port);
	int send_multi_setup_s1(char *in_buf, int sockfd, int port);
	int send_multi_setup_s2(char *in_buf, int sockfd, int port);
	int send_play(char *in_buf, int sockfd,char* m_szSessionID);
	int send_stop(char *in_buf, int sockfd, char *m_szSessionID);
	//
	int GetItem_Int(char *src, char *find_str);
	int GetItem_Str(char* src, char* find_str, char *dest);
private:
	int  m_CSeq;
	char m_szSvrAddr[URL_LEN];
	char m_szSessionId[SESS_ID_LEN];
	char m_szSendBuf[SEND_BUF_MAX+1];
	int  m_nRtspStatus;
	int  m_nRtspSockFd;
	bool m_bOpenAudio;
	char m_szDebug[64];
};
