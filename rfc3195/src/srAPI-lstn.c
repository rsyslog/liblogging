/*! \file srAPI-lstn.c
 *  \brief Implementation of the listener part of the API
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-26
 *          begun coding on initial version.
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

#include <assert.h>
#include "settings.h"
#include "liblogging.h"
#include "beepsession.h"
#include "beepprofile.h"
#include "beeplisten.h"
#include "lstnprof-3195raw.h"
#include "lstnprof-3195cooked.h"
#include "srAPI.h"
#include "syslogmessage.h"
#include "namevaluetree.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srRetVal srAPISetMsgRcvCallback(srAPIObj* pThis, void(*NewHandler)(srAPIObj*, srSLMGObj*))
{
	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	pThis->OnSyslogMessageRcvd = NewHandler;

	return SR_RET_OK;
}


srRetVal srAPISetupListener(srAPIObj* pThis, void(*NewHandler)(srAPIObj*, srSLMGObj*))
{
	srRetVal iRet;
	sbProfObj *pProf;

	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	if(pThis->pLstn != NULL)
		return SR_RET_ALREADY_LISTENING;

	if((pThis->pLstn = sbLstnConstruct()) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	/* now tell the lib which listeners to start */

#	if FEATURE_UDP == 1
	/* UDP Listener */
	pThis->pLstn->bLstnUDP = pThis->bListenUDP;
	pThis->pLstn->uUDPLstnPort = pThis->iUDPListenPort;
#	endif

#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	/* Unix Domain Sockets listener */
	pThis->pLstn->bLstnUXDOMSOCK = pThis->bListenUXDOMSOCK;
	if(pThis->szNameUXDOMSOCK != NULL)
	{
		if((pThis->pLstn->pSockName = sbNVTEUtilStrDup(pThis->szNameUXDOMSOCK)) == NULL)
			return SR_RET_OUT_OF_MEMORY;
	}
#	endif

	/* please note: as of now, the BEEP listener will always be
	 * started.
	 */
	pThis->pLstn->uListenPort = pThis->iBEEPListenPort;

	/* now begin initializing the listeners */

	if((iRet = sbLstnInit(pThis->pLstn)) != SR_RET_OK)
		return iRet;

	pThis->pLstn->pAPI = pThis;

	if(pThis->bListenBEEP == TRUE)
	{
		/* set up the rfc 3195/raw listener */
		if((iRet = sbProfConstruct(&pProf, "http://xml.resource.org/profiles/syslog/RAW")) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			return iRet;
		}

		if((iRet = sbProfSetAPIObj(pProf, pThis)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = srAPISetMsgRcvCallback(pThis, NewHandler)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = sbProfSetEventHandler(pProf, sbPROFEVENT_ONCHANCREAT, (void*) psrrOnChanCreate)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = sbProfSetEventHandler(pProf, sbPROFEVENT_ONMESGRECV, (void*) psrrOnMesgRecv)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = sbLstnAddProfile(pThis->pLstn, pProf)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		/* set up the rfc 3195/COOKED listener */
		if((iRet = sbProfConstruct(&pProf, "http://xml.resource.org/profiles/syslog/COOKED")) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			return iRet;
		}

		if((iRet = sbProfSetAPIObj(pProf, pThis)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = srAPISetMsgRcvCallback(pThis, NewHandler)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = sbProfSetEventHandler(pProf, sbPROFEVENT_ONCHANCREAT, (void*) psrcOnChanCreate)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = sbProfSetEventHandler(pProf, sbPROFEVENT_ONMESGRECV, (void*) psrcOnMesgRecv)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}

		if((iRet = sbLstnAddProfile(pThis->pLstn, pProf)) != SR_RET_OK)
		{
			sbLstnDestroy(pThis->pLstn);
			sbProfDestroy(pProf);
			return iRet;
		}
	} /* End setup of BEEP listener (only executed if BEEP listener is enabled) */

	return SR_RET_OK;
}


srRetVal srAPIRunListener(srAPIObj *pThis)
{
	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;

	return sbLstnRun(pThis->pLstn);
}


srRetVal srAPIShutdownListener(srAPIObj *pThis)
{
	if((pThis == NULL) || (pThis->OID != OIDsrAPI))
		return SR_RET_INVALID_HANDLE;
	
	pThis->pLstn->bRun = FALSE;

	return SR_RET_OK;
}
