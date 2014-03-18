/** 
 * testsrvr.cpp : This is a small sample C++ server app
 * using liblogging. It just demonstrates how things can be 
 * done. It accepts incoming messages and just dumps them
 * to stdout. It is single-threaded.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-13
 *          file created.
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
#include <signal.h>
#ifdef WIN32
#include <process.h>
#endif
#include "liblogging.h"
#include "srAPI.h"
#include "syslogmessage.h"
#ifdef WIN32
#include <crtdbg.h>
#endif

/* quick hack, so all can access it. Do NOT do this in your server ;-) */
srAPIObj* pAPI;

/* This method is called when a message has been fully received.
 * In a real sample, you would do all your enqueuing and/or
 * processing here.
 * 
 * It is highly recommended that no lengthy processing is done in
 * this callback. Please see \ref architecture.c for a suggested
 * threading model.
 */
void OnReceive(srAPIObj* pAPI, srSLMGObj* pSLMG)
{
	char *pszRemoteHost;
	char *pszHost;
	unsigned char *pszMSG;
	unsigned char *pszRawMsg;
	int	iFacil;
	int iPrio;
	char szNotWellformed[] = "(non-wellformed msg)";

	srSLMGGetRemoteHost(pSLMG, &pszRemoteHost);
	srSLMGGetPriority(pSLMG, &iPrio);
	srSLMGGetFacility(pSLMG, &iFacil);
	srSLMGGetRawMSG(pSLMG, &pszRawMsg);
	if(srSLMGGetHostname(pSLMG, &pszHost) == SR_RET_PROPERTY_NOT_AVAILABLE)
		pszHost = szNotWellformed;
	srSLMGGetMSG(pSLMG, &pszMSG);

	printf("Msg from %s (via %d), host %s, facility %d, priority %d:\n%s\nRAW:%s\n\n", pszRemoteHost, pSLMG->iSource,
	       pszHost, iFacil, iPrio, pszMSG, pszRawMsg);
}


/* As we are single-threaded in this example, we need
 * one way to shut down the listener running on this
 * single thread. We use SIG_INT to do so - it effectively
 * provides a short-lived second thread ;-)
 */
void doSIGINT(int i)
{
	printf("SIG_INT - shutting down listener. Be patient, can take up to 30 seconds...\n");
	srAPIShutdownListener(pAPI);
}

int main(int argc, char* argv[])
{
	srRetVal iRet;

#	ifdef	WIN32
		_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#	endif

	printf("testsrvr test server - just a quick debuging aid and sample....\n");
	printf("Compiled with liblogging version %s\n", VERSION);
	printf("See http://www.monitorware.com/liblogging/ for updates.\n");
	printf("Listening for incoming requests....\n");

	signal(SIGINT, doSIGINT);

	if((pAPI = srAPIInitLib()) == NULL)
	{
		printf("Error initializing lib!\n");
		exit(1);
	}

	/* We now set the options so that the server receives all we desire... */
	if((iRet = srAPISetOption(pAPI, srOPTION_LISTEN_UDP, TRUE)) != SR_RET_OK)
	{
		printf("Error %d: can't set UDP listener option to true!!\n", iRet);
		exit(2);
	}
	if((iRet = srAPISetOption(pAPI, srOPTION_LISTEN_UXDOMSOCK, TRUE)) != SR_RET_OK)
	{
		printf("Error %d: can't set UDP listener option to true!!\n", iRet);
		exit(2);
	}

	if((iRet = srAPISetupListener(pAPI, OnReceive)) != SR_RET_OK)
	{
		printf("Error %d setting up listener!\n", iRet);
		exit(100);
	}

	/* now move the listener to running state. Control will only
	 * return after SIG_INT.
	 */
	if((iRet = srAPIRunListener(pAPI)) != SR_RET_OK)
	{
		printf("Error %d running the listener!\n", iRet);
		exit(100);
	}

	/** control will reach this point after SIG_INT */

	srAPIExitLib(pAPI);
	return 0;
}

