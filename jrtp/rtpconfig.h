/*

  This file is a part of JRTPLIB
  Copyright (c) 1999-2011 Jori Liesenborgs

  Contact: jori.liesenborgs@gmail.com

  This library was developed at the Expertise Centre for Digital Media
  (http://www.edm.uhasselt.be), a research center of the Hasselt University
  (http://www.uhasselt.be). The library is based upon work done for 
  my thesis at the School for Knowledge Technology (Belgium/The Netherlands).

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#ifndef RTPCONFIG_UNIX_H

#define RTPCONFIG_UNIX_H

#define JRTPLIB_EXPORT  //extern "C" __declspec(dllexport)
#define JRTPLIB_IMPORT  extern "C" __declspec(dllimport)
#define JRTPLIB_COMPILING
//#define JRTPLIB_IMPORT ${JRTPLIB_IMPORT}
//#define JRTPLIB_EXPORT ${JRTPLIB_EXPORT}
#ifdef JRTPLIB_COMPILING
	#define JRTPLIB_IMPORTEXPORT JRTPLIB_EXPORT
#else
	#define JRTPLIB_IMPORTEXPORT JRTPLIB_IMPORT
#endif // JRTPLIB_COMPILING


//#define RTP_HAVE_SYS_FILIO
//
//#define RTP_HAVE_SYS_SOCKIO
//
//#define RTP_ENDIAN			//大端字节排序
//
//#define RTP_SOCKLENTYPE_UINT	//表明getsockname使用非符号整数作为它的第三个参数。
//
//#define RTP_HAVE_SOCKADDR_LEN	//表明struct sockaddr有一个sa_len字段。
//
#define RTP_SUPPORT_IPV4MULTICAST//支持IPv4多播。
//
#define RTP_SUPPORT_THREAD		//JThread支持。
//
#define RTP_SUPPORT_SDESPRIV	//支持RTCP SDES 私有项(去掉就没RTCP)
//
//#define RTP_SUPPORT_PROBATION	//如果设置,连续几个的RTP数据包需要验证一个成员。
//
//#define RTP_SUPPORT_GETLOGINR	//如果设置,库将使用getlogin_r代替getlogin。
//
//${RTP_SUPPORT_IPV6}			//支持IPv6。
//
//${RTP_SUPPORT_IPV6MULTICAST}	//支持IPv6多播。
//
//#define RTP_SUPPORT_IFADDRS
//
#define RTP_SUPPORT_SENDAPP		//sending of RTCP app packets is enabled.
//
#define RTP_SUPPORT_MEMORYMANAGEMENT//支持内存管理
//
//#define RTP_SUPPORT_RTCPUNKNOWN//支持发送未知RTCP数据包

#endif // RTPCONFIG_UNIX_H

