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
 * Copyright 2002-2014 
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

#include "config.h"
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
#include "beepsession.h"

#define DEFAULT_IP "172.19.1.20" 

int main(int argc, char* argv[])
{
	int i;
	srRetVal iRet;
	char szMsg[1025];
	srAPIObj* pAPI;
	srSLMGObj *pMsg;
    char *ipStr;

    if (argc > 1)
        ipStr = argv[1];
    else
        ipStr = DEFAULT_IP;

#	ifdef	WIN32
		_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#	endif

	printf("Liblogging test driver - just a quick debuging aid and sample....\n");
	printf("Compiled with liblogging version %s\n", VERSION);
	printf("See http://www.monitorware.com/liblogging/ for updates.\n");
	if((pAPI = srAPIInitLib()) == NULL)
	{
		printf("Error initializing lib!\n");
		exit(1);
	}

	/* First of all, create message object */
	if((iRet = srSLMGConstruct(&pMsg)) != SR_RET_OK)
	{
		printf("Error %d creating syslog message object!\n", iRet);
		exit(2);
	}
	if((iRet = srSLMGSetHOSTNAMEtoCurrent(pMsg)) != SR_RET_OK)
	{
		printf("Error %d setting hostname in syslog message object!\n", iRet);
		exit(2);
	}
	if((iRet = srSLMGSetTAG(pMsg, "testdrvr[0]")) != SR_RET_OK)
	{
		printf("Error %d setting TAG in syslog message object!\n", iRet);
		exit(2);
	}
	srSLMGSetFacility(pMsg, 7);
	srSLMGSetSeverity(pMsg, 0);

	/* carry on with the transport */
	/* you need to do the following only if you would
	 * like to limit the profiles used. This is NOT
	 * recommended.
	 */
	if((iRet = srAPISetOption(pAPI, srOPTION_3195_ALLOWED_CLIENT_PROFILES, USE_3195_PROFILE_ANY)) != SR_RET_OK)
	{
		printf("Error %d: can't set 3195 profile!\n", iRet);
		exit(2);
	}

	if((iRet = srAPIOpenlog(pAPI, ipStr, 601)) != SR_RET_OK)
	{
		printf("Error %d: can't open session!\n", iRet);
		exit(2);
	}

	for(i = 0 ; i < 500 ; ++i)
	{
		/*	For a long message:	sprintf(szMsg, "Message %d. --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------", i); */
		sprintf(szMsg, "Message %d", i);
		if((iRet = srSLMGSetMSG(pMsg, szMsg, FALSE)) != SR_RET_OK)
		{
			printf("Error %d setting MSG in syslog message object!\n", iRet);
			exit(2);
		}
		srSLMGSetTIMESTAMPtoCurrent(pMsg);

		/* Replace srSLMGFmt_3164WELLFORMED with srSLMGFmt_SIGN_12 if you would
		 * like to use the new syslog-sign-12 timestamp (TIMESTAMP-3339).
		 */
		if((iRet = srSLMGFormatRawMsg(pMsg, srSLMGFmt_3164WELLFORMED)) != SR_RET_OK)
		{
			printf("Error %d formatting syslog message!\n", iRet);
			exit(100);
		}

		/* Send the message */
		if((iRet = srAPISendSLMG(pAPI, pMsg)) != SR_RET_OK)
		{
			printf("Error %d while sending!\n", iRet);
			exit(3);
		}
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
