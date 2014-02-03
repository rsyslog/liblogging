/*! \file clntprof-3195raw.c
 *  \brief Implementation of the client profile for RFC 3195 raw.
 *
 * The prefix for this "object" is psrr which stands for
 * *P*rofile *S*yslog *R*eliable *Raw*. This file works in
 * conjunction with \ref lstnprof-3195raw.c and shares
 * its namespace.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-04
 *          coding begun.
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
#include "srAPI.h"
#include "beepsession.h"
#include "beepchannel.h"
#include "beepprofile.h"
#include "clntprof-3195raw.h"
#include "syslogmessage.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/**
 * Construct a sbPSSRObj.
 */
static srRetVal sbPSSRConstruct(sbPSSRObj** ppThis)
{
	assert(ppThis != NULL);

	if((*ppThis = calloc(1, sizeof(sbPSSRObj))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	(*ppThis)->OID = OIDsbPSSR;
	(*ppThis)->uAnsno = 0;
	(*ppThis)->uMsgno4raw = 0;

	return SR_RET_OK;
}


/**
 * Destroy a sbPSSRObj.
 * The handler must be called in the ChanClose handler, if instance
 * data was assigned.
 */
static void sbPSSRDestroy(sbPSSRObj* pThis)
{
	sbPSSRCHECKVALIDOBJECT(pThis);
	SRFREEOBJ(pThis);
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srRetVal sbPSSRClntSendSLMG(sbChanObj* pChan, srSLMGObj *pSLMG)
{
	return sbPSSRClntSendMsg(pChan, pSLMG->pszRawMsg);
}

srRetVal sbPSSRClntSendMsg(sbChanObj* pChan, char* szLogmsg)
{
	sbMesgObj *pMesg;
	srRetVal iRet;
	sbPSSRObj *pThis;

	sbChanCHECKVALIDOBJECT(pChan);
	assert(szLogmsg != NULL);

	pThis = pChan->pProfInstance;
	sbPSSRCHECKVALIDOBJECT(pThis);

	if((pMesg = sbMesgConstruct(NULL, szLogmsg)) == NULL)
		return SR_RET_ERR;

	iRet = sbMesgSendMesg(pMesg, pChan, "ANS", pThis->uAnsno++);
	sbMesgDestroy(pMesg);

	return iRet;
}


srRetVal sbPSSRClntOpenLogChan(sbChanObj *pChan)
{
	srRetVal iRet;
	sbPSSRObj *pThis;
	sbMesgObj *pMesgGreeting;

	sbChanCHECKVALIDOBJECT(pChan);

	if((iRet = sbPSSRConstruct(&pThis)) != SR_RET_OK)
		return iRet;

	/* channel created, let's wait until the remote peer sends
	 * the profile-level greeting (intial MSG).
	 */
	if((pMesgGreeting = sbMesgRecvMesg(pChan)) == NULL)
	{
		return SR_RET_ERR;
	}

	if(pMesgGreeting->idHdr != BEEPHDR_MSG)
	{
		sbMesgDestroy(pMesgGreeting);
		return SR_RET_ERR;
	}
	sbMesgDestroy(pMesgGreeting);

	pThis->uAnsno = 0;
	pThis->uMsgno4raw = pMesgGreeting->uMsgno;

	/* now store the instance pointer */
	pChan->pProfInstance = pThis;
    
	return SR_RET_OK;
}

srRetVal sbPSSRCOnClntCloseLogChan(sbChanObj *pChan)
{
	srRetVal iRet;
	sbPSSRObj *pThis;
	sbMesgObj *pMesg;

	sbChanCHECKVALIDOBJECT(pChan);

	pThis = pChan->pProfInstance;
	sbPSSRCHECKVALIDOBJECT(pThis);

	if((pMesg = sbMesgConstruct("", "")) == NULL)
	{
        sbMesgDestroy(pMesg);
		return SR_RET_ERR;
	}

	iRet = sbMesgSendMesg(pMesg, pChan, "NUL", pThis->uAnsno++);
    sbMesgDestroy(pMesg);

	sbPSSRDestroy(pThis);
	pChan->pProfInstance = NULL;

	return iRet;
}
