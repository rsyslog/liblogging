/*! \file srAPI.c
 *  \brief Implementation of the API
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          Initial version (0.1) released.
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

#include <assert.h>
#include "config.h"
#include "liblogging.h"
#include "beepsession.h"
#include "beepprofile.h"
#include "beeplisten.h"
#include "srAPI.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/**
 * This variable is a global library setting. It tells the library
 * if it should instruct the lib to call the OS socket initialiser.
 * Under Win32, for example, this is only allowed once per
 * process. So if the lib is integrated into a process that 
 * already uses sockets, the sockets initializer (WSAStartup()
 * in this case) should not be called.
 * This variable can be modified via a global setting.
 */
static int srAPI_bCallOSSocketInitializer = TRUE;

/**
 * Destructor for the API object.
 */
static void srAPIDestroy(srAPIObj* pThis)
{
	srAPICHECKVALIDOBJECT(pThis);

	if(pThis->pChan != NULL)
		sbSessCloseChan(pThis->pSess, pThis->pChan);

	if(pThis->pProf != NULL)
		sbProfDestroy(pThis->pProf);

	if(pThis->pSess != NULL)
		sbSessCloseSession(pThis->pSess);

	if(pThis->pLstn != NULL)
		sbLstnExit(pThis->pLstn);

	SRFREEOBJ(pThis);
}



/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srAPIObj* srAPIInitLib(void)
{
	srAPIObj *pThis;

	if((pThis = calloc(1, sizeof(srAPIObj))) == NULL)
		return NULL;

	pThis->OID = OIDsrAPI;
	pThis->pSess = NULL;
	pThis->pProf = NULL;
	pThis->pChan = NULL;
#	if FEATURE_LISTENER == 1
	pThis->OnSyslogMessageRcvd = NULL;
	pThis->pLstn = NULL;
#	endif
	sbSockLayerInit(srAPI_bCallOSSocketInitializer);

	return(pThis);
} 


srRetVal srAPISetOption(srAPIObj* pThis, SRoption iOpt, int iOptVal)
{
	switch(iOpt)
	{
	/* This must be done for all options, that require a pAPI
		if((pThis == NULL) || (pThis->OID != OIDsrAPI))
			return SR_RET_INVALID_HANDLE;
    */
	case srOPTION_CALL_OS_SOCKET_INITIALIZER:
		if(pThis != NULL)
			return SR_RET_INVALID_HANDLE;
		if(iOptVal != TRUE && iOptVal != FALSE)
			return SR_RET_INVALID_OPTVAL;
		srAPI_bCallOSSocketInitializer = iOptVal;
		break;
	default:
		return SR_RET_INVALID_LIB_OPTION;
	}

	return SR_RET_OK;
}


srRetVal srAPIExitLib(srAPIObj *pThis)
{
	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	srAPIDestroy(pThis);

	return SR_RET_OK;
}

/** 
 * Open a RFC 3195 / RAW log session.
 *
 * This API is effectively the initiator side of the RAW profile.
 *
 * This method looks like much code, but most of it is error 
 * handling ;) - so we keep it as a single long method.
 *
 * \param pszRemotePeer [in] The Peer to connect to.
 * \param iPort [in] The port the remote Peer is listening to.
 */
srRetVal srAPIOpenlog(srAPIObj *pThis, char* pszRemotePeer, int iPort)
{
	srRetVal iRet;
	sbMesgObj *pProfileGreeting;

	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	if((pThis->pSess = sbSessOpenSession(pszRemotePeer, iPort)) == NULL)
	{
		srAPIDestroy(pThis);
		return SR_RET_ERR;
	}

	if((iRet = sbProfConstruct(&(pThis->pProf), "http://xml.resource.org/profiles/syslog/RAW")) != SR_RET_OK)
	{
		srAPIDestroy(pThis);
		return iRet;
	}

	if((pThis->pChan = sbSessOpenChan(pThis->pSess, pThis->pProf)) == NULL)
	{
		srAPIDestroy(pThis);
		return SR_RET_ERR;
	}

	/* channel created, let's wait until the remote peer sends
	 * the profile-level greeting (intial MSG).
	 */
	if((pProfileGreeting = sbMesgRecvMesg(pThis->pChan)) == NULL)
	{
		srAPIDestroy(pThis);
		return SR_RET_ERR;
	}

	if(pProfileGreeting->idHdr != BEEPHDR_MSG)
	{
		srAPIDestroy(pThis);
		return SR_RET_ERR;
	}

	pThis->uAnsno = 0;
	pThis->uMsgno4raw = pProfileGreeting->uMsgno;
	sbMesgDestroy(pProfileGreeting);

	return(SR_RET_OK);
}


srRetVal srAPISendLogmsg(srAPIObj* pThis, char* szLogmsg)
{
	sbMesgObj *pMesg;
	srRetVal iRet;

	/* Attention: order of conditions is vitally important! */
	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	if((pMesg = sbMesgConstruct(NULL, szLogmsg)) == NULL)
		return SR_RET_ERR;

	iRet = sbMesgSendMesg(pMesg, pThis->pChan, "ANS", pThis->uAnsno++);
	sbMesgDestroy(pMesg);

	return iRet;
}


srRetVal srAPICloseLog(srAPIObj *pThis)
{
	sbMesgObj *pMesg;
	int iRet = SR_RET_OK;

	/* Attention: order of conditions is vitally important! */
	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	if((pMesg = sbMesgConstruct("", "")) == NULL)
		iRet = SR_RET_ERR;
	else
	{
		iRet = sbMesgSendMesg(pMesg, pThis->pChan, "NUL", pThis->uAnsno++);
        sbMesgDestroy(pMesg);
	}

	/* now destroy all but the API object itself */
	if(pThis->pChan != NULL)
	{
		sbSessCloseChan(pThis->pSess, pThis->pChan);
		pThis->pChan = NULL;
	}

	if(pThis->pProf != NULL)
	{
		sbProfDestroy(pThis->pProf);
		pThis->pProf = NULL;
	}

	if(pThis->pSess != NULL)
	{
		sbSessCloseSession(pThis->pSess);
		pThis->pSess = NULL;
	}

	return(iRet);
}


srRetVal srAPISetUsrPointer(srAPIObj *pAPI, void* pUsr)
{
	if((pAPI == NULL) || (pAPI->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	pAPI->pUsr = pUsr;

	return SR_RET_OK;
}


srRetVal srAPIGetUsrPointer(srAPIObj *pAPI, void **ppToStore)
{
	if((pAPI == NULL) || (pAPI->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	if(ppToStore == NULL)
		return SR_RET_INVALID_HANDLE;

	*ppToStore = pAPI->pUsr;

	return SR_RET_OK;
}
