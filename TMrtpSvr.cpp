// TMrtpSvr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RTPServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CRTPServer rtpsvr;
	if(rtpsvr.ServerStart())
		Sleep(100*1000);
	Sleep(300);
	rtpsvr.ServerStop();
	return 0;
}