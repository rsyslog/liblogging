#include <windows.h>
/** 
 * TestDrvr.cpp : This is a small sample C++ application
 * using liblogging. It just demonstrates how things can be 
 * done. Be sure to replace the IP addresses below
 * with your values.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          0.1 version created.
 *
 * Copyright 2002-2003 
 *     Rainer Gerhards and Adiscon GmbH. All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Adiscon GmbH or Rainer Gerhards
 *       nor the names of its contributors may be used to
 *       endorse or promote products derived from this software without
 *       specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#ifdef WIN32
#include <process.h>
#endif
#include "liblogging.h"
#include "srAPI.h"
#include "syslogmessage.h"
#ifdef WIN32
#include <crtdbg.h>
#endif
#include <time.h>

int main(int argc, char* argv[])
{
	int i;
	srRetVal iRet;
	char szMsg[1025];
	srAPIObj* pAPI;
	srSLMGObj *pMsg;

#	ifdef	WIN32
		_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#	endif

	printf("Liblogging test driver - just a quick debuging aid and sample....\n");
	printf("Compiled with liblogging version %d.%d.%d.\n", LIBLOGGING_VERSION_MAJOR, LIBLOGGING_VERSION_MINOR, LIBLOGGING_VERSION_SUBMINOR);
	printf("See http://www.monitorware.com/liblogging/ for updates.\n");
	if((pAPI = srAPIInitLib()) == NULL)
	{
		printf("Error initializing lib!\n");
		exit(1);
	}

	if((iRet = srSLMGConstruct(&pMsg)) != SR_RET_OK)
	{
		printf("Error %d creating syslog message object!\n", iRet);
		exit(2);
	}

	if(srAPIOpenlog(pAPI, "172.19.1.20", 601) != SR_RET_OK)
	{
		printf("Error: can't open session!\n");
		exit(2);
	}

	for(i = 0 ; i < 15 ; ++i)
	{
		/* First of all, create message object */
		pMsg->iFacility = 7;
		pMsg->iPriority = 0;
		pMsg->pszHostname = "host";
		pMsg->pszTag = "tag";

		sprintf(szMsg, "Message %d", i);
		pMsg->pszMsg = szMsg;
		srSLMGSetTIMESTAMPtoCurrent(pMsg);
		if((iRet = srSLMGFormatRawMsg(pMsg, srSLMGFmt_SIGN_12)) != SR_RET_OK)
//		if((iRet = srSLMGFormatRawMsg(pMsg, srSLMGFmt_3164WELLFORMED)) != SR_RET_OK)
		{
			printf("Error %d formatting syslog message!\n", iRet);
			exit(100);
		}

//		sprintf(szMsg, "<128> 2003-04-12T18:20:50.52-06:00 adiscon l %d", i);
//		sprintf(szMsg, "<128>Oct 27 13:21:08 adiscon liblogger[141]: We are currently at message # %d. -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------", i);
//		sprintf(szMsg, "<128>Oct 27 13:21:08 adiscon liblogger[141]: We are currently at message # %d.", i);
		/* Send the message */
//		if(srAPISendLogmsg(pAPI, szMsg) != SR_RET_OK)
		if(srAPISendLogmsg(pAPI, pMsg->pszRawMsg) != SR_RET_OK)
		{
			printf("Error while sending!\n");
			exit(3);
		}
		Sleep(68);
	}
	
	if((iRet = srAPICloseLog(pAPI)) != SR_RET_OK)
	{
		printf("Error %d while closing session!\n", iRet);
		exit(4);
	}

	srSLMGDestroy(pMsg);
	srAPIExitLib(pAPI);
	return 0;
}

