#include "StdAfx.h"
#include "RtspServer.h"
#include <process.h>
//typedef unsigned (__stdcall* PTHREAD_START)(void *);

CRITICAL_SECTION g_csNewSocket;
CRITICAL_SECTION g_csListData;
//完成例程
SOCKET	g_sNewClientConnect;
BOOL	g_bNewConnectArrived;
//完成例程工作线程
DWORD WINAPI WorkerThread(LPVOID lpParam);
void CALLBACK CompletionROUTINE(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
CRtspServer* g_pSvr = NULL;
CRtspServer::CRtspServer(void)
{
	m_hListen = NULL;
	m_hHandle = NULL;
	m_bClosed = FALSE;
	m_sListen = 0;
	g_pSvr = this;
	InitializeCriticalSection(&g_csNewSocket);
	InitializeCriticalSection(&g_csListData);
}

CRtspServer::~CRtspServer(void)
{
	if( !m_bClosed )
		StopServer();
	if(m_hListen)
	{
		DWORD theErr = ::WaitForSingleObject(m_hListen, 3000);
		if (theErr != WAIT_OBJECT_0)
		{
			TerminateThread(m_hListen, 0);
			WaitForSingleObject(m_hListen, INFINITE);
		}
		CloseHandle(m_hListen);		
	}
	if(m_hHandle)
	{
		DWORD theErr = ::WaitForSingleObject(m_hHandle, 3000);
		if (theErr != WAIT_OBJECT_0)
		{
			TerminateThread(m_hHandle, 0);
			WaitForSingleObject(m_hHandle, INFINITE);
		}
		CloseHandle(m_hHandle);		
	}
	DeleteCriticalSection(&g_csNewSocket);
	DeleteCriticalSection(&g_csListData);
}
BOOL CRtspServer::StartServer(const char* IP,int nRtspPort)
{
	m_sListen = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(m_sListen == INVALID_SOCKET)
	{
		printf("===Create socket fail.\n");
		return FALSE;
	}

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nRtspPort);
	sin.sin_addr.S_un.S_addr = inet_addr( IP );//INADDR_ANY;
	if(bind(m_sListen,(LPSOCKADDR)&sin,sizeof(sin)) == SOCKET_ERROR)
	{
		printf("===bind port fail.\n");
		return FALSE;
	}
	if(listen(m_sListen, 5) == SOCKET_ERROR)
	{
		printf("===listen err.\n");
		return FALSE;
	}
	::CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
	m_bClosed = FALSE;
	m_hListen = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, NULL);
	if(m_hListen == NULL)
		return FALSE;
	m_hHandle = (HANDLE)_beginthreadex(NULL, 0, HandleDataThread, this, 0, NULL);
	if(m_hListen == NULL)
		return FALSE;
	return TRUE;
}
void CRtspServer::StopServer()
{	
	m_bClosed = TRUE;
	closesocket( m_sListen );
	m_sListen = 0;
		
	DATA_NODE DataNode;
	list<DATA_NODE>::iterator iterData = NULL;
	EnterCriticalSection(&g_csListData);
	iterData = m_listData.begin();
	while(iterData != m_listData.end())
	{
		DataNode = (*iterData);
		delete [] DataNode.pBuf;
		iterData ++;
	}
	m_listData.clear();
	LeaveCriticalSection(&g_csListData);
	
}
unsigned int __stdcall CRtspServer::ListenThread(void *param)
{
	((CRtspServer*)param)->ListenConnect();
	return 0;
}
void CRtspServer::ListenConnect()
{
	sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	SOCKET sClient;	
	DWORD dwIPAddr = 0;
	while (m_sListen > 0)
	{
		sClient = accept(m_sListen,(SOCKADDR*)&remoteAddr,&nAddrLen);
		if(sClient == INVALID_SOCKET)
		{
			Sleep(1);
			continue;
		}
		EnterCriticalSection(&g_csNewSocket);
		g_sNewClientConnect = sClient;
		g_bNewConnectArrived = TRUE;
		LeaveCriticalSection(&g_csNewSocket);
		//printf("===Accepted client:%s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));
	}
}
DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	PER_IO_OPERATION_DATA* lpPerIOData = NULL;
	while (TRUE)
	{
		if (g_bNewConnectArrived)
		{
			// Launch an asynchronous operation for new arrived connection
			EnterCriticalSection(&g_csNewSocket);
			lpPerIOData = (PER_IO_OPERATION_DATA*)HeapAlloc(
				GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));
			lpPerIOData->Buffer.len = SEND_BUF_MAX;
			lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
			lpPerIOData->sClient = g_sNewClientConnect;
			printf("=== New Client Connect : %d\n",g_sNewClientConnect);
			WSARecv(lpPerIOData->sClient, &lpPerIOData->Buffer, 1,
				&lpPerIOData->NumberOfBytesRecvd, &lpPerIOData->Flags,
				&lpPerIOData->overlap, CompletionROUTINE);	
			g_bNewConnectArrived = FALSE;
			LeaveCriticalSection(&g_csNewSocket);
		}
		SleepEx(1000, TRUE);
	}
	return 0;
}
void CALLBACK CompletionROUTINE(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	PER_IO_OPERATION_DATA* lpPerIOData = (PER_IO_OPERATION_DATA*)lpOverlapped;
	if (dwError != 0 || cbTransferred == 0)
	{
		// Connection was closed by client
		//printf("=== Client Close : %d\n",lpPerIOData->sClient);		
		closesocket(lpPerIOData->sClient);
		HeapFree(GetProcessHeap(), 0, lpPerIOData);
	}
	else
	{
		//printf("===recv %d\n",cbTransferred);
		DATA_NODE dn;
		dn.nLen = cbTransferred;
		dn.pBuf = new char[dn.nLen];
		if( dn.pBuf )
		{
			dn.sSocket = lpPerIOData->sClient;
			memcpy(dn.pBuf, lpPerIOData->szMessage, dn.nLen);
			EnterCriticalSection(&g_csListData);
			g_pSvr->m_listData.push_back(dn);
			LeaveCriticalSection(&g_csListData);
		}		

		// Launch another asynchronous operation
		memset(&lpPerIOData->overlap, 0, sizeof(WSAOVERLAPPED));
		lpPerIOData->Buffer.len = SEND_BUF_MAX;
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;

		WSARecv(lpPerIOData->sClient, &lpPerIOData->Buffer, 1,
			&lpPerIOData->NumberOfBytesRecvd, &lpPerIOData->Flags,
			&lpPerIOData->overlap, CompletionROUTINE);
	}
}
unsigned int __stdcall CRtspServer::HandleDataThread(void *param)
{
	((CRtspServer*)param)->HandleDataThrd();
	return 0;
}
void CRtspServer::HandleDataThrd()
{
	DATA_NODE dn;
	while( !m_bClosed )
	{
		EnterCriticalSection(&g_csListData);
		if( m_listData.size() == 0 )
		{
			Sleep(10);
			LeaveCriticalSection(&g_csListData);
			continue;
		}
		dn = m_listData.front();
		m_listData.pop_front();
		LeaveCriticalSection(&g_csListData);
				
		if(memcmp(dn.pBuf, "OPTION", 6) == 0)
		{		
			int CSeq=tctp_getitem_int(dn.pBuf,"CSeq: ");		

			//get_rtpcli_content_public(pRsuint->channel, p2);
			//tctp_response_recv_opt(dn.pBuf,CSeq,p1,p2,pRsuint->rtspcli_sockfd);

			memset(dn.pBuf, 0, SEND_BUF_MAX+1);
			//len = recv(pRsuint->rtspcli_sockfd, in_buffer, SEND_BUF_MAX, 0);			
		}	
		if(memcmp(dn.pBuf, "DESCRIBE", 8) == 0)
		{
			int CSeq=tctp_getitem_int(dn.pBuf,"CSeq: ");	
			char tmp_buf[256];
			memset(tmp_buf,0,256);
			tctp_getitem_str(dn.pBuf,"DESCRIBE ",tmp_buf,1);			

			//get_rtpcli_content_sdp(pRsuint->channel, p4);

			/*{
				char * p5=strstr(p4,pRssvr->rs_unit[0].m_ClientIP);
				if(p5!=NULL)
				{
					int tt_len=strlen(pRssvr->rs_unit[0].m_ClientIP);
					char * p6;
					p6=(char *)malloc(sdp_len+32);
					if(p6==NULL)
					{
						goto THREAD_CLIENT_END;
					}

					char * p7;
					p7=(char *)malloc(sdp_len+32);
					if(p7==NULL)
					{
						free(p6);
						goto THREAD_CLIENT_END;
					}

					memset(p6,0,sdp_len+32);
					memset(p7,0,sdp_len+32);
					memcpy(p6,p4,sdp_len);
					memcpy(p7,p4,sdp_len);

					*p5=0;
					int tt_len1=strlen(p4);

					p6[tt_len1]=0;
					strcat(p6,pRsuint->cli_ip);
					strcat(p6,p7+tt_len1+tt_len);

					tt_len=strlen(p6);

					free(p4);
					free(p7);

					p4=(char *)malloc(tt_len+1);
					if(p4==NULL)
					{
						free(p6);
						goto THREAD_CLIENT_END;
					}

					strcpy(p4,p6);
					free(p6);

					itoa(tt_len,p3,10);		
				}			
			}*/
			//if(tctp_response_recv_describe(dn.pBuf,CSeq,p1,tmp_buf,p2,p3,p4,pRsuint->rtspcli_sockfd)<0)
			{
				
			}
		}
		else
		{
			int CSeq=0;
			tctp_send_bad_request(dn.pBuf,CSeq,dn.sSocket);
			
		}
		//=======================
		delete [] dn.pBuf;
	}
}
/************************************************************************/
/*   协议解析                                                           */
/************************************************************************/
int CRtspServer::tctp_getitem_str(char *src, char *find_str, char *dest,unsigned char space_flag)
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

	if(space_flag)
	{
		while((*tmp!=0x0D)&&(*tmp!=0x20))
		{
			*tmp_dest=*tmp;
			tmp++;
			tmp_dest++;
		}

	}
	else
	{
		while(*tmp!=0x0D)
		{
			*tmp_dest=*tmp;
			tmp++;
			tmp_dest++;
		}
	}

	*tmp_dest=0;

	return 0;
}

int CRtspServer::tctp_getitem_int(char *src, char *find_str)
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

int CRtspServer::tctp_send_bad_request(char *in_buf,int cseq, int sockfd)
{	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 400 Bad Request\r\n");
	strcat(in_buf, "Server: Win32/1.0.1\r\n");
	strcat(in_buf, "Cseq: \r\n");
	strcat(in_buf, "Connection: Close\r\n\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_response_recv_opt(char *in_buf,int cseq, char *cdate, char *cpublic, int sockfd)
{	
	char strCSeq[5];

	memset(strCSeq,0,5);	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 200 OK\r\n");

	sprintf(strCSeq,"%d",cseq);
	strcat(in_buf,"CSeq: ");
	strcat(in_buf,strCSeq);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Date: ");
	strcat(in_buf,cdate);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Public: ");
	strcat(in_buf,cpublic);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_response_recv_describe(char *in_buf,int cseq, char *cdate,
								char * cbase, char *ctype,char * clength,char *csdp,int sockfd)
{	
	char strCSeq[5];	

	memset(strCSeq,0,5);	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 200 OK\r\n");

	sprintf(strCSeq,"%d",cseq);
	strcat(in_buf,"CSeq: ");
	strcat(in_buf,strCSeq);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Date: ");
	strcat(in_buf,cdate);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"Content-Base: ");
	strcat(in_buf,cbase);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Content-Type: ");
	strcat(in_buf,ctype);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"Content-Length: ");
	strcat(in_buf,clength);
	strcat(in_buf,"\r\n");
	strcat(in_buf,"\r\n");

	strcat(in_buf,csdp);

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_response_recv_setup_s1(char *in_buf,int cseq, char *cdate,
								char * destIP, char * srcIP,char * cliPort,int svrPort,char * sessionId,int sockfd)
{	
	char strCSeq[5];
	char temp[8];

	memset(strCSeq,0,5);	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 200 OK\r\n");

	sprintf(strCSeq,"%d",cseq);
	strcat(in_buf,"CSeq: ");
	strcat(in_buf,strCSeq);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Date: ");
	strcat(in_buf,cdate);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"Transport: RTP/AVP;unicast;destination=");
	strcat(in_buf,destIP);
	strcat(in_buf,";source=");
	strcat(in_buf,srcIP);	
	strcat(in_buf,";client_port=");
	strcat(in_buf,cliPort);
	strcat(in_buf,";server_port=");	

	itoa(svrPort,temp,10);
	strcat(in_buf,temp);
	strcat(in_buf,"-");
	itoa(svrPort+1,temp,10);
	strcat(in_buf,temp);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Session: ");
	strcat(in_buf,sessionId);
	strcat(in_buf,"\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_multi_response_recv_setup_s1(char *in_buf,int cseq, char *cdate,
									  char * destIP, char * srcIP,int multiPort,unsigned char multiTtl,char * sessionId,int sockfd)
{	
	char strCSeq[5];
	char temp[16];

	memset(strCSeq,0,5);	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 200 OK\r\n");

	sprintf(strCSeq,"%d",cseq);
	strcat(in_buf,"CSeq: ");
	strcat(in_buf,strCSeq);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Date: ");
	strcat(in_buf,cdate);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"Transport: RTP/AVP;multicast;destination=");
	strcat(in_buf,destIP);
	strcat(in_buf,";source=");
	strcat(in_buf,srcIP);

	strcat(in_buf,";port=");
	sprintf(temp, "%d-%d\0", multiPort, multiPort+1);	
	strcat(in_buf,temp);

	sprintf(temp, ";ttl=%d\0", multiTtl);	
	strcat(in_buf,temp);	
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Session: ");
	strcat(in_buf,sessionId);
	strcat(in_buf,"\r\n");
	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_response_recv_play(char *in_buf,int cseq, char *cdate,
							char * range, char * rtpinfo,char * sessionId,int sockfd)
{	
	char strCSeq[5];

	memset(strCSeq,0,5);	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 200 OK\r\n");

	sprintf(strCSeq,"%d",cseq);
	strcat(in_buf,"CSeq: ");
	strcat(in_buf,strCSeq);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Date: ");
	strcat(in_buf,cdate);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"Range: ");
	strcat(in_buf,range);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"Session: ");
	strcat(in_buf,sessionId);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"RTP-Info: ");
	strcat(in_buf,rtpinfo);
	strcat(in_buf,"\r\n");	

	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_response_recv_stop(char *in_buf,int cseq, char *cdate, int sockfd)
{	
	char strCSeq[5];

	memset(strCSeq,0,5);	
	memset(in_buf,0,SEND_BUF_MAX+1);

	strcat(in_buf, "RTSP/1.0 200 OK\r\n");

	sprintf(strCSeq,"%d",cseq);
	strcat(in_buf,"CSeq: ");
	strcat(in_buf,strCSeq);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"Date: ");
	strcat(in_buf,cdate);
	strcat(in_buf,"\r\n");

	strcat(in_buf,"\r\n");

	return (send(sockfd,in_buf,strlen(in_buf),0));
}

int CRtspServer::tctp_getitem_connect_type(char *src, char *dest)
{
	char tmp_buf[256];	

	if(tctp_getitem_str(src,"rtsp://",tmp_buf,1)<0)
	{
		return -1;
	}

	for(int i=0;i<256;i++)
	{
		if(tmp_buf[i]==0x2F)
		{
			strcpy(dest,&tmp_buf[i]);
			return 0;
		}
	}

	return -1;
}

int CRtspServer::ReplaceStr(char *sSrc, char *sMatchStr, char *sReplaceStr)
{
	int  StringLen;
	char *caNewString;
	char *FindPos = strstr(sSrc, sMatchStr);

	if((!FindPos) || (!sMatchStr))
		return -1;

	caNewString=(char *)malloc(strlen(sSrc)+64);
	if(caNewString==NULL)
	{
		return -1;
	}
	while(FindPos)
	{
		memset(caNewString, 0, strlen(sSrc)+64);
		StringLen = FindPos-sSrc;
		memcpy(caNewString, sSrc, StringLen);
		strcat(caNewString, sReplaceStr);
		strcat(caNewString, FindPos + strlen(sMatchStr));
		strcpy(sSrc, caNewString);
		FindPos = strstr(sSrc, sMatchStr);
	}
	free(caNewString);
	return 0;
}