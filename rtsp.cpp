#include "StdAfx.h"
#include "Rtsp.h"
#include "winsock2.h"
#pragma comment(lib,"ws2_32")

char strRtsp200OK[] = "RTSP/1.0 200 OK\r\n";
char strRtspUnauthorized[] = "RTSP/1.0 401 Unauthorized\r\n";

CRtsp::CRtsp(void)
{
	m_CSeq = 0;
	memset(m_szSvrAddr,0,URL_LEN);
	memset(m_szSessionId,0,SESS_ID_LEN);
	memset(m_szSendBuf,0,SEND_BUF_MAX+1);
	m_nRtspStatus = RTSP_STATUS_UNKNOWN;
	m_nRtspSockFd = 0;
	m_bOpenAudio = false;
}

CRtsp::~CRtsp(void)
{
	closesocket(m_nRtspSockFd);
}
int CRtsp::RtspConnect(const char *pDestIp,const char *url, int nSrcPort,int nDesPort,int *video_port,int *audio_port)
{
	int  len,ret;
	struct sockaddr_in svr_addr;

	struct timeval timeout;
	fd_set fd;

	int rtp_port = 0;

	m_CSeq=0;

	m_nRtspStatus = RTSP_STATUS_CONNECTING;
	m_nRtspSockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( m_nRtspSockFd < 0 ) 
		return -1;
	
	unsigned long rtnval = 1;
	ret = ioctlsocket(m_nRtspSockFd, FIONBIO, (unsigned long*)&rtnval);
	if(ret == SOCKET_ERROR)	
	{
		closesocket(m_nRtspSockFd);
		return -2;
	}

	memset(&svr_addr, 0, sizeof(SOCKADDR_IN));
	svr_addr.sin_addr.s_addr = inet_addr(pDestIp);
	svr_addr.sin_family = AF_INET;     
	svr_addr.sin_port = htons(nDesPort);

	FD_ZERO(&fd);
	FD_SET(m_nRtspSockFd, &fd);
	timeout.tv_sec = 3;
	timeout.tv_usec= 0;
	connect(m_nRtspSockFd, (struct sockaddr *) &svr_addr, sizeof(svr_addr));
	ret = select(0, 0, &fd, 0, &timeout);
	if ( ret <= 0 )
	{
		closesocket(m_nRtspSockFd);
		return -3;
	}
	//
	/*rtnval = 0;
	ret = ioctlsocket(m_nRtspSockFd, FIONBIO, (unsigned long*)&rtnval);
	if(ret==SOCKET_ERROR)	
	{
		closesocket(m_nRtspSockFd);
		return -2;
	}*/
	//
	char strPort[8];
	memset(strPort,0,8);
	sprintf(strPort, "%d", nDesPort);
	memset(m_szSvrAddr, 0, sizeof(m_szSvrAddr));
	memcpy(m_szSvrAddr, " rtsp://", sizeof(" rtsp://"));
	strcat(m_szSvrAddr, pDestIp);
	strcat(m_szSvrAddr, ":");
	strcat(m_szSvrAddr, strPort);
	strcat(m_szSvrAddr, url);
	
	int nStep = 0;
	while(TRUE)
	{
		FD_ZERO(&fd);
		FD_SET(m_nRtspSockFd, &fd);
		if(nStep>0)
		{
			ret = select(0, &fd, 0, 0, &timeout);
			if ( ret == 0 )
				break;
			else if( ret < 0)
			{
				closesocket(m_nRtspSockFd);
				break;
			}
			if(!FD_ISSET(m_nRtspSockFd, &fd))
				continue;
		}
		else
			send_opt(m_szSendBuf, m_nRtspSockFd);
		if(nStep == 0)
			nStep += 1;
		else if(nStep == 1)
		{
			memset(m_szSendBuf, 0, SEND_BUF_MAX+1);
			len = recv(m_nRtspSockFd, m_szSendBuf, SEND_BUF_MAX, 0);
			if (len < 0)
			{
				closesocket(m_nRtspSockFd);
				return -5;
			}
			if (memcmp(m_szSendBuf, strRtsp200OK, sizeof(strRtsp200OK) -1) != 0)
			{
				closesocket(m_nRtspSockFd);
				return -6;
			}
			//==============2 step==========
			send_describe(m_szSendBuf, m_nRtspSockFd);
			nStep += 1;
		}
		else if(nStep == 2)
		{
			memset(m_szSendBuf, 0, SEND_BUF_MAX + 1);   
			len = recv(m_nRtspSockFd, m_szSendBuf, SEND_BUF_MAX, 0);
			if (len < 0)
			{ 
				closesocket(m_nRtspSockFd);
				return -7;
			}
			
			//Log("e:\\Log.txt",m_szSendBuf,0);
			if (memcmp(m_szSendBuf, strRtsp200OK, sizeof(strRtsp200OK)-1) == 0)
			{
				//不用授权
			}
			else if (memcmp(m_szSendBuf, strRtspUnauthorized, sizeof(strRtspUnauthorized)-1) == 0)
			{
				//没有授权
				send_describe(m_szSendBuf, m_nRtspSockFd, true);
				memset(m_szSendBuf, 0, SEND_BUF_MAX + 1);   
				len = recv(m_nRtspSockFd, m_szSendBuf, SEND_BUF_MAX, 0);
				if (len < 0)
				{ 
					closesocket(m_nRtspSockFd);
					return -8;
				}
				if (memcmp(m_szSendBuf, strRtsp200OK, sizeof(strRtsp200OK)-1) != 0)
				{
					closesocket(m_nRtspSockFd);
					return -9;
				}
			}
			else
			{
				closesocket(m_nRtspSockFd);
				return -10;
			}
			
			int Audio1 = GetItem_Int(m_szSendBuf,"PCMU/");
			int Audio2 = GetItem_Int(m_szSendBuf,"AMR/");
			int Audio3 = GetItem_Int(m_szSendBuf,"AMR-WB/");			
			if(Audio1>0 || Audio2>0 || Audio3>0)
				m_bOpenAudio = true;
			//==============3 step==========
			send_setup_s1(m_szSendBuf, m_nRtspSockFd, nSrcPort);
			nStep += 1;
		}
		else if(nStep == 3)
		{
			memset(m_szSendBuf, 0, SEND_BUF_MAX + 1);
			len = recv(m_nRtspSockFd, m_szSendBuf, SEND_BUF_MAX, 0);
			if (len < 0)
			{
				closesocket(m_nRtspSockFd);
				return -11;
			}

			GetItem_Str(m_szSendBuf, "Session: ", m_szSessionId);
			if (memcmp(m_szSendBuf, strRtsp200OK, sizeof(strRtsp200OK)-1) != 0)	
			{
				closesocket(m_nRtspSockFd);
				return -12;
			}
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////			
			rtp_port=GetItem_Int(m_szSendBuf,"client_port=");
			if(rtp_port == -1)
			{
				closesocket(m_nRtspSockFd);
				return -13;
			}
			sprintf(m_szDebug,"rtp local port:%d\n",rtp_port);
			OutputDebugString(m_szDebug);
			//
			*video_port = GetItem_Int(m_szSendBuf,"server_port=");
			if(*video_port == -1)
			{  
				closesocket(m_nRtspSockFd);
				return -14;
			}
			sprintf(m_szDebug,"rtp server video port:%d\n",*video_port);
			OutputDebugString(m_szDebug);
			//==============4 step==========音频====
			if( m_bOpenAudio )
				send_setup_s2(m_szSendBuf, m_nRtspSockFd, nSrcPort + 2);
			else
			{
				m_nRtspStatus = RTSP_STATUS_SETUP;
				break;
			}
			nStep += 1;
		}
		else if(nStep == 4)
		{
			//=====音频====
			if( m_bOpenAudio )
			{
				memset(m_szSendBuf, 0, SEND_BUF_MAX + 1);
				len = recv(m_nRtspSockFd, m_szSendBuf, SEND_BUF_MAX, 0);
				if (len < 0)
				{
					closesocket(m_nRtspSockFd);
					return -15;
				}
				*audio_port = GetItem_Int(m_szSendBuf, "server_port=" );
				if (memcmp(m_szSendBuf, strRtsp200OK, sizeof(strRtsp200OK)-1) != 0)
				{
					closesocket(m_nRtspSockFd);
					return -16;
				}
				sprintf(m_szDebug,"rtp server audio port:%d\n",*audio_port);
				OutputDebugString(m_szDebug);
			}
			m_nRtspStatus = RTSP_STATUS_SETUP;
			break;
			//==============5 step==========
			/*send_play(m_szSendBuf, m_nRtspSockFd, m_szSessionId);
			nStep += 1;
			if(RtpData_Receive() < 0)
			{
				closesocket(m_nRtspSockFd);
				return -18;
			}*/			
		}
		/*else if(nStep == 5)
		{
			memset(m_szSendBuf, 0, SEND_BUF_MAX + 1);
			len = recv(m_nRtspSockFd, m_szSendBuf, SEND_BUF_MAX, 0);
			if (len < 0)
			{
				closesocket(m_nRtspSockFd);
				return -19;
			}
			if (memcmp(m_szSendBuf, strRtsp200OK, sizeof(strRtsp200OK)-1) != 0)
			{
				closesocket(m_nRtspSockFd);
				return -20;
			}
			m_nRtspStatus = RTSP_STATUS_PLAY;
			break;
		}*/
	}
	
	if(m_nRtspStatus == RTSP_STATUS_SETUP)
		return 0;
	closesocket(m_nRtspSockFd);
	return -21;
}
void CRtsp::SendPlay()
{
	send_play(m_szSendBuf, m_nRtspSockFd,m_szSessionId);
	m_nRtspStatus = RTSP_STATUS_PLAY;
}
void CRtsp::CloseConnect()
{
	send_stop(m_szSendBuf, m_nRtspSockFd,m_szSessionId);
	Sleep(200);
	closesocket(m_nRtspSockFd);
}
int CRtsp::send_opt(char *in_buf, int sockfd)
{
	char strCSeq[5];

	memset(strCSeq, 0, 5);
	memset(in_buf, 0, SEND_BUF_MAX+1);			
	m_CSeq++;

	memcpy(in_buf, "OPTIONS", sizeof("OPTIONS") );
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, " RTSP/1.0\r\n" );

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");

	strcat(in_buf, "User-Agent: ");
	strcat(in_buf, "Temobi NetSDK\r\n");
	strcat(in_buf, "\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtsp::send_describe(char *in_buf, int sockfd, bool bAuthorized)
{
	char strCSeq[5];

	memset(strCSeq,0,5);
	memset(in_buf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(in_buf, "DESCRIBE", sizeof("DESCRIBE"));
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");
	if( bAuthorized )
	{
		strcat(in_buf,"Authorization: Basic YWRtaW46YWRtaW4=\r\n");
	}
	strcat(in_buf,"User-Agent: ");
	strcat(in_buf,"Temobi NetSDK\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));	
}

int CRtsp::send_setup_s1(char *in_buf, int sockfd, int port)
{
	char strCSeq[5];
	char strPort[8];

	memset(strCSeq,0,5);
	memset(strPort,0,8);
	memset(in_buf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(in_buf, "SETUP", sizeof("SETUP"));
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, "/track1");
	strcat(in_buf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");

	strcat(in_buf, "Transport: ");
	/*if(m_bRtpTCP)
		strcat(in_buf,"RTP/AVP/TCP;unicast;client_port=");
	else*/
	strcat(in_buf,"RTP/AVP;unicast;client_port=");
	sprintf(strPort, "%d", port);
	strcat(in_buf, strPort);

	strcat(in_buf, "-");
	memset(strPort,0,8);
	sprintf(strPort, "%d", port+1);
	strcat(in_buf, strPort);


	strcat(in_buf, "\r\n");
	strcat(in_buf,"User-Agent: ");
	strcat(in_buf,"Temobi NetSDK\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtsp::send_setup_s2(char *in_buf, int sockfd, int port)
{
	char strCSeq[5];
	char strPort[8];

	memset(strCSeq,0,5);
	memset(strPort,0,8);
	memset(in_buf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(in_buf, "SETUP", sizeof("SETUP"));
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, "/track2");
	strcat(in_buf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");

	strcat(in_buf, "Transport: ");
	/*if(m_bRtpTCP)
		strcat(in_buf,"RTP/AVP/TCP;unicast;client_port=");
	else*/
	strcat(in_buf,"RTP/AVP;unicast;client_port=");
	sprintf(strPort, "%d", port);
	strcat(in_buf, strPort);

	strcat(in_buf, "-");
	memset(strPort,0,8);
	sprintf(strPort, "%d", port+1);
	strcat(in_buf, strPort);


	strcat(in_buf, "\r\n");
	strcat(in_buf,"User-Agent: ");
	strcat(in_buf,"Temobi NetSDK\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtsp::send_multi_setup_s1(char *in_buf, int sockfd, int port)
{
	char strCSeq[5];
	char strPort[8];

	memset(strCSeq,0,5);
	memset(strPort,0,8);
	memset(in_buf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(in_buf, "SETUP", sizeof("SETUP"));
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, "/track1");
	strcat(in_buf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");

	strcat(in_buf, "Transport: ");
	strcat(in_buf,"RTP/AVP;multicast;client_port=");	
	sprintf(strPort, "%d", port);
	strcat(in_buf, strPort);

	strcat(in_buf, "-");
	memset(strPort,0,8);
	sprintf(strPort, "%d", port+1);
	strcat(in_buf, strPort);


	strcat(in_buf, "\r\n");
	strcat(in_buf,"User-Agent: ");
	strcat(in_buf,"Temobi NetSDK\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtsp::send_multi_setup_s2(char *in_buf, int sockfd, int port)
{
	char strCSeq[5];
	char strPort[8];

	memset(strCSeq,0,5);
	memset(strPort,0,8);
	memset(in_buf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(in_buf, "SETUP", sizeof("SETUP"));
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, "/track2");
	strcat(in_buf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");

	strcat(in_buf, "Transport: ");
	strcat(in_buf,"RTP/AVP;multicast;client_port=");	
	sprintf(strPort, "%d", port);
	strcat(in_buf, strPort);

	strcat(in_buf, "-");
	memset(strPort,0,8);
	sprintf(strPort, "%d", port+1);
	strcat(in_buf, strPort);


	strcat(in_buf, "\r\n");
	strcat(in_buf,"User-Agent: ");
	strcat(in_buf,"Temobi NetSDK\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtsp::send_play(char *pBuf, int sockfd, char *pSessID)
{
	char strCSeq[5];

	memset(strCSeq,0,5);
	memset(pBuf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(pBuf, "PLAY", sizeof("PLAY"));
	strcat(pBuf, m_szSvrAddr);
	strcat(pBuf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(pBuf, "CSeq: ");
	strcat(pBuf, strCSeq);
	strcat(pBuf, "\r\n");

	strcat(pBuf, "Session: ");
	strcat(pBuf, pSessID);
	strcat(pBuf, "\r\n");

	strcat(pBuf, "Range: ");
	strcat(pBuf,"npt=0.000-");
	strcat(pBuf, "\r\n");

	strcat(pBuf,"User-Agent: ");
	strcat(pBuf,"Temobi NetSDK\r\n");
	strcat(pBuf,"\r\n");

	return (send(sockfd,pBuf,strlen(pBuf),0));
}
int CRtsp::send_stop(char *in_buf, int sockfd, char *SessionId)
{
	char strCSeq[5];

	memset(strCSeq,0,5);
	memset(in_buf,0,SEND_BUF_MAX+1);
	m_CSeq++;

	memcpy(in_buf, "TEARDOWN", sizeof("TEARDOWN"));
	strcat(in_buf, m_szSvrAddr);
	strcat(in_buf, " RTSP/1.0\r\n");

	sprintf(strCSeq, "%d", m_CSeq);
	strcat(in_buf, "CSeq: ");
	strcat(in_buf, strCSeq);
	strcat(in_buf, "\r\n");

	strcat(in_buf, "Session: ");
	strcat(in_buf, m_szSessionId);
	strcat(in_buf, "\r\n");

	strcat(in_buf,"User-Agent: ");
	strcat(in_buf,"Temobi NetSDK\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));	
}
int CRtsp::GetItem_Int(char *src, char *find_str)
{
	char *tmp;
	int rtnval;
	int len,len1;	

	if(src==NULL||find_str==NULL)
	{
		return -1;
	}

	len=strlen(src);
	len1=strlen(find_str);
	if((!len)||(!len1)||(len<len1))
	{
		return -1;
	}

	tmp=strstr(src,find_str);
	if(!tmp)
	{
		return -1;
	}	

	rtnval=0; 

	tmp+=len1;
	while((*tmp>=0x30)&&(*tmp<=0x39))
	{
		rtnval=(int)((*tmp-0x30)+rtnval*10);;
		tmp++;	
	}
	return rtnval;
}
int CRtsp::GetItem_Str(char *src, char *find_str, char *dest)
{
	char *tmp;
	char *tmp_dest;
	int len,len1;	

	if(src==NULL||find_str==NULL)
	{
		return -1;
	}

	len=strlen(src);
	len1=strlen(find_str);
	if((!len)||(!len1)||(len<len1))
	{
		return -1;
	}

	tmp=strstr(src,find_str);
	if(!tmp)
	{
		return -1;
	}	

	tmp_dest=dest; 

	tmp+=len1;
	while((*tmp!=0x0D)&&(*tmp!=0x20))
	{
		*tmp_dest=*tmp;
		tmp++;
		tmp_dest++;
	}

	*tmp_dest=0;

	return 0;
}
