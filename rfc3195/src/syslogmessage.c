/*! \file syslogmessage.c
 *  \brief Implementation of the syslog message object.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-01
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

#include <ctype.h>
#include <assert.h>
#include "settings.h"
#include "liblogging.h"
#include "srAPI.h"
#include "syslogmessage.h"
#include "namevaluetree.h"
#include "stringbuf.h"
#include "oscalls.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/* ################################################################## *
 * # Now come methods that are only available if the full-blown     # *
 * # message object should be included in the build.                # *
 * ################################################################## */
#	if FEATURE_MSGAPI == 1


#	endif /* #	if FEATURE_MSGAPI == 1 */
/* Next should be public members! */


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srRetVal srSLMGConstruct(srSLMGObj **ppThis)
{
	if(ppThis == NULL)
		return SR_RET_NULL_POINTER_PROVIDED;

	if((*ppThis = calloc(1, sizeof(srSLMGObj))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	(*ppThis)->OID = OIDsrSLMG;
	(*ppThis)->pszRawMsg = NULL;
	(*ppThis)->iSource = srSLMG_Source_OWNGenerated;
#	if FEATURE_MSGAPI == 1
	(*ppThis)->bTimStampIncludesTZ = FALSE;
	(*ppThis)->cTimStampOffsetMode = srSLMG_TimStamp_INVALID;
	(*ppThis)->iFacility = 1; /* default as of RFC 3164 */
	(*ppThis)->iSeverity = 5; /* default as of RFC 3164 */
	/* The timestamp fields are not initilized because calloc() 
	 * already did this job. So DO NOT REMOVE CALLOC()! */
	(*ppThis)->pszHostname = NULL;
	(*ppThis)->pszLanguage = NULL;
	(*ppThis)->pszRemoteHost = NULL;
	(*ppThis)->pszTag = NULL;
	(*ppThis)->pszMsg = NULL;
	(*ppThis)->bOwnMsg = TRUE;	/* right now, we own it ;) */
	(*ppThis)->pszTimeStamp = NULL;
#	endif
	return SR_RET_OK;
}


void srSLMGDestroy(srSLMGObj *pThis)
{
	/* as the caller provides this information AND the caller
	 * may not be part of this lib, we can't use assert() here!
	 */
	if(pThis == NULL)
		return;

	if(pThis->OID != OIDsrSLMG)
		return;

	if((pThis->bOwnRemoteHostBuf == TRUE) && (pThis->pszRemoteHost != NULL))
		free(pThis->pszRemoteHost);
	if((pThis->bOwnRawMsgBuf == TRUE) && (pThis->pszRawMsg != NULL))
		free(pThis->pszRawMsg);
#	if FEATURE_MSGAPI == 1
	if(pThis->pszHostname != NULL)
		free(pThis->pszHostname );
	if(pThis->pszLanguage != NULL)
		free(pThis->pszLanguage );
	if(pThis->pszTag != NULL)
		free(pThis->pszTag);
	if((pThis->bOwnMsg == TRUE) && (pThis->pszMsg != NULL))
		free(pThis->pszMsg);
	if(pThis->pszTimeStamp != NULL)
		free(pThis->pszTimeStamp);
#	endif

	SRFREEOBJ(pThis);
}

/* ################################################################## *
 * # Now come methods that are only available if the full-blown     # *
 * # message object should be included in the build.                # *
 * ################################################################## */
#if FEATURE_MSGAPI == 1


/**
 * Parse a 32 bit integer number from a string.
 *
 * \param ppsz Pointer to the Pointer to the string being parsed. It
 *             must be positioned at the first digit. Will be updated 
 *             so that on return it points to the first character AFTER
 *             the integer parsed.
 * \retval The number parsed.
 */
static int _srSLMGParseInt32(unsigned char** ppsz)
{
	int i;

	i = 0;
	while(isdigit(**ppsz))
	{
		i = i * 10 + **ppsz - '0';
		++(*ppsz);
	}

	return i;
}


/**
 * Parse the PRI out of the message.
 */
static int srSLMGParsePRI(srSLMGObj* pThis, unsigned char** ppszBuf)
{
	int i;

	srSLMGCHECKVALIDOBJECT(pThis);
	assert(ppszBuf != NULL);
	assert((*ppszBuf >= pThis->pszRawMsg));

	if(**ppszBuf != '<')
		return FALSE;
	++(*ppszBuf);

	/* extract PRI */
	i = _srSLMGParseInt32(ppszBuf);

	if(**ppszBuf != '>')
		return FALSE;
	++(*ppszBuf);

	pThis->iFacility = i >> 3;
	pThis->iSeverity = i % 8;

	return TRUE;
}


/**
 * Parse a TIMESTAMP-3339.
 */
static int srSLMGParseTIMESTAMP3339(srSLMGObj* pThis, unsigned char* pszTS)
{
	srSLMGCHECKVALIDOBJECT(pThis);
	assert(pszTS != NULL);

	pThis->iTimStampYear = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampYear < 2003 || pThis->iTimStampYear > 9999)
		/* The lib was written in 2003, so any value below that year is suspicious.
		 * Of course, this prevents replay of data, but if someone really intends
		 * to do this, he is free to modify this portion of the lib ;)
		 */
		return FALSE;

	/* We take the liberty to accept slightly malformed timestamps e.g. in 
	 * the format of 2003-9-1T1:0:0. This doesn't hurt on receiving. Of course,
	 * with the current state of affairs, we would never run into this code
	 * here because at postion 11, there is no "T" in such cases ;)
	 */
	if(*pszTS++ != '-')
		return FALSE;
	pThis->iTimStampMonth = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampMonth < 1 || pThis->iTimStampMonth > 12)
		return FALSE;

	if(*pszTS++ != '-')
		return FALSE;
	pThis->iTimStampDay = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampDay < 1 || pThis->iTimStampDay > 31)
		return FALSE;

	if(*pszTS++ != 'T')
		return FALSE;
	pThis->iTimStampHour = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampHour < 0 || pThis->iTimStampHour > 23)
		return FALSE;

	if(*pszTS++ != ':')
		return FALSE;
	pThis->iTimStampMinute = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampMinute < 0 || pThis->iTimStampMinute > 59)
		return FALSE;

	if(*pszTS++ != ':')
		return FALSE;
	pThis->iTimStampSecond = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampSecond < 0 || pThis->iTimStampSecond > 60)
		return FALSE;

	/* Now let's see if we have secfrac */
	if(*pszTS == '.')
	{
		unsigned char *pszStart = ++pszTS;
		pThis->iTimStampSecFrac = _srSLMGParseInt32(&pszTS);
		pThis->iTimStampSecFracPrecision = (int) (pszTS - pszStart);
	}
	else
	{
		pThis->iTimStampSecFracPrecision= 0;
		pThis->iTimStampSecFrac = 0;
	}

	/* check the timezone */
	if(*pszTS == 'Z')
	{
		pThis->bTimStampIncludesTZ= TRUE;
		pThis->iTimStampOffsetHour = 0;
		pThis->iTimStampOffsetMinute = 0;
	}
	else if((*pszTS == '+') || (*pszTS == '-'))
	{
		pThis->cTimStampOffsetMode = *pszTS;
		pszTS++;

		pThis->iTimStampOffsetHour = _srSLMGParseInt32(&pszTS);
		if(pThis->iTimStampOffsetHour < 0 || pThis->iTimStampOffsetHour > 23)
			return FALSE;

		if(*pszTS++ != ':')
			return FALSE;
		pThis->iTimStampOffsetMinute = _srSLMGParseInt32(&pszTS);
		if(pThis->iTimStampOffsetMinute < 0 || pThis->iTimStampOffsetMinute > 59)
			return FALSE;

		pThis->bTimStampIncludesTZ = TRUE;
	}
	else
		/* there MUST be TZ information */
		return FALSE;

	/* OK, we actually have a 3339 timestamp, so let's indicated this */
	pThis->iTimStampType = srSLMG_TimStamp_3339;

	return TRUE;
}


/**
 * Parse a TIMESTAMP-3164.
 */
static int srSLMGParseTIMESTAMP3164(srSLMGObj* pThis, unsigned char* pszTS)
{
	srSLMGCHECKVALIDOBJECT(pThis);
	assert(pszTS != NULL);

	/* If we look at the month (Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec),
	 * we may see the following character sequences occur:
	 *
	 * J(an/u(n/l)), Feb, Ma(r/y), A(pr/ug), Sep, Oct, Nov, Dec
	 *
	 * We will use this for parsing, as it probably is the
	 * fastest way to parse it.
	 */
	switch(*pszTS++)
	{
	case 'J':
		if(*pszTS++ == 'a')
			if(*pszTS++ == 'n')
				pThis->iTimStampMonth = 1;
			else
				return FALSE;
		else if(*pszTS++ == 'u')
			if(*pszTS++ == 'n')
				pThis->iTimStampMonth = 6;
			else if(*pszTS++ == 'l')
				pThis->iTimStampMonth = 7;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'F':
		if(*pszTS++ == 'e')
			if(*pszTS++ == 'b')
				pThis->iTimStampMonth = 2;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'M':
		if(*pszTS++ == 'a')
			if(*pszTS++ == 'r')
				pThis->iTimStampMonth = 3;
			else if(*pszTS++ == 'y')
				pThis->iTimStampMonth = 5;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'A':
		if(*pszTS++ == 'p')
			if(*pszTS++ == 'r')
				pThis->iTimStampMonth = 4;
			else
				return FALSE;
		else if(*pszTS++ == 'u')
			if(*pszTS++ == 'g')
				pThis->iTimStampMonth = 8;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'S':
		if(*pszTS++ == 'e')
			if(*pszTS++ == 'p')
				pThis->iTimStampMonth = 9;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'O':
		if(*pszTS++ == 'c')
			if(*pszTS++ == 't')
				pThis->iTimStampMonth = 10;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'N':
		if(*pszTS++ == 'o')
			if(*pszTS++ == 'v')
				pThis->iTimStampMonth = 11;
			else
				return FALSE;
		else
			return FALSE;
		break;
	case 'D':
		if(*pszTS++ == 'e')
			if(*pszTS++ == 'c')
				pThis->iTimStampMonth = 12;
			else
				return FALSE;
		else
			return FALSE;
		break;
	default:
		return FALSE;
	}

	/* done month */

	if(*pszTS++ != ' ')
		return FALSE;

	/* we accept a slightly malformed timestamp when receiving. This is
	 * we accept one-digit days
	 */
	if(*pszTS == ' ')
		++pszTS;

	pThis->iTimStampDay = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampDay < 1 || pThis->iTimStampDay > 31)
		return FALSE;

	if(*pszTS++ != ' ')
		return FALSE;
	pThis->iTimStampHour = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampHour < 0 || pThis->iTimStampHour > 23)
		return FALSE;

	if(*pszTS++ != ':')
		return FALSE;
	pThis->iTimStampMinute = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampMinute < 0 || pThis->iTimStampMinute > 59)
		return FALSE;

	if(*pszTS++ != ':')
		return FALSE;
	pThis->iTimStampSecond = _srSLMGParseInt32(&pszTS);
	if(pThis->iTimStampSecond < 0 || pThis->iTimStampSecond > 60)
		return FALSE;

	/* OK, we actually have a 3164 timestamp, so let's indicate this */
	pThis->iTimStampType = srSLMG_TimStamp_3164;

	return TRUE;
}


/**
 * Parse the TIMESTAMP part of the message.
 * Primarily based on http://www.ietf.org/internet-drafts/draft-ietf-syslog-sign-12.txt;
 */
static int srSLMGParseTIMESTAMP(srSLMGObj* pThis, unsigned char** ppszBuf)
{
	sbStrBObj *pStrBuf;
	int iLenTS;

	srSLMGCHECKVALIDOBJECT(pThis);
	assert(ppszBuf != NULL);
	assert((*ppszBuf >= pThis->pszRawMsg));

	/* first of all, extract timestamp. The timestamp
	 * string is also used by other parts of liblogging,
	 * so it must be stored within the SLMG obj.
	 *
	 * A timestamp must be at least 10 characters wide.
	 * As such, we read at last 15 characters (if the 
	 * string is large enough. This is mainly because
	 * TIMESTAMP-3164 contains spaces in it and this is
	 * the only (relatively) elegant way to get around
	 * them...
	 */
	iLenTS = 0;
	if((pStrBuf = sbStrBConstruct()) == NULL)	return FALSE;
	sbStrBSetAllocIncrement(pStrBuf, 33);
	while((iLenTS < 32) && **ppszBuf)
	{
		if((**ppszBuf == ' ') && (iLenTS >= 10))
			break;
		if(sbStrBAppendChar(pStrBuf, **ppszBuf) != SR_RET_OK)	{ sbStrBDestruct(pStrBuf); return FALSE; }
		++(*ppszBuf);
		++iLenTS;
	}
	if(pThis->pszTimeStamp != NULL)
		free(pThis->pszTimeStamp);
	if((pThis->pszTimeStamp = sbStrBFinish(pStrBuf)) == NULL) return FALSE;

	if(**ppszBuf != ' ')
		return FALSE;
	++(*ppszBuf);
	/* ok, the message buffer is already updated. Now let's 
	 * see what timestamp we got.
	 *
     * we first see if it looks like a TIMESTAMP-3339
	 */
	if((iLenTS > 11) && (pThis->pszTimeStamp[10] == 'T'))
	{	/* looks like it is TIMESTAMP-3339, so let's try to parse it */
		return srSLMGParseTIMESTAMP3339(pThis, pThis->pszTimeStamp);
	}
	else
	{	/* looks like TIMESTAMP-3164 */
		return srSLMGParseTIMESTAMP3164(pThis, pThis->pszTimeStamp);
	}
	/*NOTREACHED*/
}


/**
 * Parse a HOSTNAME.
 */
static int srSLMGParseHOSTNAME(srSLMGObj* pThis, unsigned char** ppszBuf)
{
	sbStrBObj *pStr;
	
	srSLMGCHECKVALIDOBJECT(pThis);
	assert(ppszBuf != NULL);
	assert((*ppszBuf >= pThis->pszRawMsg));

	if(pThis->pszRemoteHost == NULL)
	{ /* the hostname is NOT present on /dev/log */
		if(sbSock_gethostname((char**) &(pThis->pszHostname)) != SR_RET_OK)
		{
			return FALSE;
		}
	}
	else
	{ /* remotely received - extract hostname from message */
		if((pStr = sbStrBConstruct()) == NULL)
			return FALSE; /* we have no way to indicated this (unlikely) error - so this is better than nothing.. */

		sbStrBSetAllocIncrement(pStr, 64); /* should match always */

		while(**ppszBuf && **ppszBuf != ' ')
		{
			sbStrBAppendChar(pStr, **ppszBuf);
			++(*ppszBuf);
		}

		if(**ppszBuf == ' ')
			++(*ppszBuf);
		else
			/* something went wrong ;) */
			return FALSE;

		pThis->pszHostname = sbStrBFinish(pStr);
	}

	return TRUE;
}


/**
 * Parse a TAG.
 *
 * We do not have a totally clear view of what the tag is.
 * As such, we do some guesswork. First, we assume it is
 * terminated by a colon. If it isn't, we retry by terminating
 * it with a non-alphanumeric char. If that again doesn't help,
 * we have a message that is not compliant to any RFCs, thus
 * we assume it is a "raw" message.
 */
static int srSLMGParseTAG(srSLMGObj* pThis, unsigned char** ppszBuf)
{
	unsigned char *pBuf;
	unsigned char *pInBuf;
	int i;
	
	srSLMGCHECKVALIDOBJECT(pThis);
	assert(ppszBuf != NULL);
	assert((*ppszBuf >= pThis->pszRawMsg));

	pInBuf = *ppszBuf;
	if((pBuf = malloc(33 * sizeof(unsigned char))) == NULL)
		return FALSE; /* we have no way to indicated this (unlikely) error - so this is better than nothing.. */

	i = 0;
	while(**ppszBuf && **ppszBuf != ':' && i < 32)
	{
		*(pBuf+i) = **ppszBuf;
		++i;
		++(*ppszBuf);
	}

	if(**ppszBuf == ':')
	{	/* ok, looks like a syslog-sign tag */
		if(i >= 32)
		{	/* oops... */
			free(pBuf);
			return FALSE;
		}
		*(pBuf+i++) = ':';
		*(pBuf+i) = '\0';
		++(*ppszBuf);	/* eat it, the colon is not part of the message under -sign */
		pThis->pszTag = pBuf;
		return TRUE;
	}

	/* control reaches this point if the tag was no good and we
	 * need to retry.
	 */
	*ppszBuf = pInBuf;
	i = 0;
	while(**ppszBuf && isalnum(**ppszBuf) && i < 32)
	{
		*(pBuf+i) = **ppszBuf;
		++i;
		++(*ppszBuf);
	}

	if(isalnum(**ppszBuf))
	{	/* oversize tag --> invalid! */
		free(pBuf);
		return FALSE;
	}

	*(pBuf+i) = '\0';
	pThis->pszTag = pBuf;

	return TRUE;
}


/**
 * Copy the MSG to its own buffer.
 *
 * In the future, we may add coockie processing e.g. for -sign, -interntional
 * here.
 */
static srRetVal srSLMGProcessMSG(srSLMGObj* pThis, unsigned char** ppszBuf)
{
	srRetVal iRet;
	sbStrBObj *pStr;
	
	srSLMGCHECKVALIDOBJECT(pThis);
	assert(ppszBuf != NULL);
	assert((*ppszBuf >= pThis->pszRawMsg));

	if((pStr = sbStrBConstruct()) == NULL)
		return SR_RET_OUT_OF_MEMORY; /* we have no way to indicated this (unlikely) error - so this is better than nothing.. */

	while(**ppszBuf)
	{
		if((iRet = sbStrBAppendChar(pStr, **ppszBuf)) != SR_RET_OK)
			return iRet;
		++(*ppszBuf);
	}

	pThis->pszMsg = sbStrBFinish(pStr);

	return SR_RET_OK;
}


srRetVal srSLMGParseMesg(srSLMGObj *pThis)
{
	srRetVal iRet;
	unsigned char* pszBuf;
	srSLMGCHECKVALIDOBJECT_API(pThis);

	/* This algo works as follows: It tries to parse each of the syslog
	 * message. A parser is called for each part (e.g. PRI and TIMESTAMP).
	 * The parser tries to find a suitable format for that part of the
	 * message and - if it does - updates the syslog message's properties
	 * and returns TRUE. If it does not find a match, it returns FALSE.
	 * A failed parser does not necessarily mean the message is invalid - 
	 * we can always fall back to plain 3164. However, the caller for whom
	 * the message is being parsed is advised to check which format we
     * found and eventually issue an error message (e.g. if we have a
	 * syslog-sign message with a RFC 3164 message format - that does
	 * not work (or does it...?).
	 */
	pszBuf = pThis->pszRawMsg;
	if(srSLMGParsePRI(pThis, &pszBuf) == FALSE)
	{
		pThis->iFormat = srSLMGFmt_3164RAW;
		return SR_RET_OK;
	}

	if(srSLMGParseTIMESTAMP(pThis, &pszBuf) == FALSE)
	{
		pThis->iFormat = srSLMGFmt_3164RAW;
		return SR_RET_OK;
	}

	if(srSLMGParseHOSTNAME(pThis, &pszBuf) == FALSE)
	{
		pThis->iFormat = srSLMGFmt_3164RAW;
		return SR_RET_OK;
	}

	if(srSLMGParseTAG(pThis, &pszBuf) == FALSE)
	{
		pThis->iFormat = srSLMGFmt_3164RAW;
		return SR_RET_OK;
	}

	/* OK, this must be wellformed format */
	if(pThis->iTimStampType == srSLMG_TimStamp_3164)
		pThis->iFormat = srSLMGFmt_3164WELLFORMED;
	else
		pThis->iFormat = srSLMGFmt_SIGN_12;

	if((iRet = srSLMGProcessMSG(pThis, &pszBuf)) != SR_RET_OK)
		return iRet;

	return SR_RET_OK;
}


srRetVal srSLMGGetPriority(srSLMGObj *pThis, int *piPrio)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(piPrio == NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	*piPrio = pThis->iSeverity ;
	return SR_RET_OK;
}


srRetVal srSLMGGetFacility(srSLMGObj *pThis, int *piFac)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(piFac == NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	*piFac = pThis->iFacility;
	return SR_RET_OK;
}


srRetVal srSLMGGetRemoteHost(srSLMGObj *pThis, char**ppsz)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(ppsz== NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	*ppsz = pThis->pszRemoteHost;
	return SR_RET_OK;
}


srRetVal srSLMGGetHostname(srSLMGObj *pThis, char**ppsz)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(ppsz== NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	if(!((pThis->iFormat == srSLMGFmt_3164WELLFORMED) || (pThis->iFormat == srSLMGFmt_SIGN_12)))
		return SR_RET_PROPERTY_NOT_AVAILABLE;
	*ppsz = pThis->pszHostname;
	return SR_RET_OK;
}


srRetVal srSLMGGetTag(srSLMGObj *pThis, unsigned char**ppsz)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(ppsz== NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	if(!((pThis->iFormat == srSLMGFmt_3164WELLFORMED) || (pThis->iFormat == srSLMGFmt_SIGN_12)))
		return SR_RET_PROPERTY_NOT_AVAILABLE;
	*ppsz = pThis->pszTag;
	return SR_RET_OK;
}


srRetVal srSLMGGetMSG(srSLMGObj *pThis, unsigned char**ppsz)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(ppsz== NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	if(!((pThis->iFormat == srSLMGFmt_3164WELLFORMED) || (pThis->iFormat == srSLMGFmt_SIGN_12)))
		/* in this case, we have nothing  but the raw message and thus we return it.
		 * Well... 3164 says this is exactly what we should do, as in the absense
		 * of a proper header the whole message is deemed to be MSG only.
		 */
		*ppsz = pThis->pszRawMsg;
	else
		*ppsz = pThis->pszMsg;
	return SR_RET_OK;
}


/**
 * Provide the caller back with the raw message.
 *
 * This is especially useful for syslog-sign, where the
 * raw message must be stored in order to keep the sig
 * intact!
 *
 * \note The returned string is read-only. It is valid
 * only until the srSLMGObj is destroyed. If the caller intends
 * to use it for a prolonged period of time, it needs to
 * copy it!
 *
 * \param ppsz Pointer to Pointer to unsigned char to 
 *             receive the return string.
 */
srRetVal srSLMGGetRawMSG(srSLMGObj *pThis, unsigned char**ppsz)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(ppsz== NULL)
		return SR_RET_NULL_POINTER_PROVIDED;
	*ppsz = pThis->pszRawMsg;
	return SR_RET_OK;
}

srRetVal srSLMGSetRemoteHostIP(srSLMGObj *pThis, char *pszRemHostIP, int bCopyRemHost)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);

	if(pThis->pszRemoteHost != NULL)
		if(pThis->bOwnRemoteHostBuf == TRUE)
			free(pThis->pszRemoteHost);

	if(bCopyRemHost == TRUE)
	{
		if((pThis->pszRemoteHost = sbNVTEUtilStrDup(pszRemHostIP)) == NULL)
			return SR_RET_OUT_OF_MEMORY;
	}
	else
		pThis->pszRemoteHost = pszRemHostIP;

	pThis->bOwnRemoteHostBuf = bCopyRemHost;

	return SR_RET_OK;
}


srRetVal srSLMGSetRawMsg(srSLMGObj *pThis, char *pszRawMsg, int bCopyRawMsg)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(pThis->pszRawMsg != NULL)
		if(pThis->bOwnRawMsgBuf == TRUE)
			free(pThis->pszRawMsg);

	if(bCopyRawMsg == TRUE)
	{
		if((pThis->pszRawMsg = sbNVTEUtilStrDup(pszRawMsg)) == NULL)
			return SR_RET_OUT_OF_MEMORY;
	}
	else
		pThis->pszRawMsg = pszRawMsg;

	pThis->bOwnRawMsgBuf = bCopyRawMsg;

	return SR_RET_OK;
}


srRetVal srSLMGSetMSG(srSLMGObj *pThis, char *pszMSG, int bCopyMSG)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(pThis->pszMsg != NULL)
		if(pThis->bOwnMsg == TRUE)
			free(pThis->pszMsg);

	if(bCopyMSG == TRUE)
	{
		if((pThis->pszMsg = sbNVTEUtilStrDup(pszMSG)) == NULL)
			return SR_RET_OUT_OF_MEMORY;
	}
	else
		pThis->pszMsg = pszMSG;

	pThis->bOwnMsg = bCopyMSG;

	return SR_RET_OK;
}



srRetVal srSLMGSetTIMESTAMPtoCurrent(srSLMGObj *pThis)
{
	srRetVal iRet;
	srSLMGCHECKVALIDOBJECT_API(pThis);

	iRet = getCurrTime(&pThis->iTimStampYear, &pThis->iTimStampMonth, &pThis->iTimStampDay,
					   &pThis->iTimStampHour, &pThis->iTimStampMinute, &pThis->iTimStampSecond,
					   &pThis->iTimStampSecFrac, &pThis->iTimStampSecFracPrecision, &pThis->cTimStampOffsetMode,
					   &pThis->iTimStampOffsetHour, &pThis->iTimStampOffsetMinute);

	return iRet;
}


/**
 * Set the HOSTNAME to the current hostname.
 */
srRetVal srSLMGSetHOSTNAMEtoCurrent(srSLMGObj* pThis)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);
	return sbSock_gethostname((char**) (&(pThis->pszHostname)));
}


/* The following table is a helper to srSLMGFormatRawMsg:
 */
static char* srSLMGMonthNames[13] = {"ERR", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

srRetVal srSLMGFormatRawMsg(srSLMGObj *pThis, srSLMGFormat iFmtToUse)
{
	srRetVal iRet;
	sbStrBObj *pStr;
	char szBuf[128];
	int i;

	srSLMGCHECKVALIDOBJECT_API(pThis);
	if((iFmtToUse != srSLMGFmt_3164WELLFORMED) && (iFmtToUse != srSLMGFmt_SIGN_12))
		return SR_RET_UNSUPPORTED_FORMAT;

	if(pThis->pszRawMsg != NULL)
		if(pThis->bOwnRawMsgBuf == FALSE)
			return SR_RET_UNALLOCATABLE_BUFFER;
		else
		{
			free(pThis->pszRawMsg);
			pThis->pszRawMsg = NULL;
		}

	if((pStr = sbStrBConstruct()) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	/* PRI */
	i = (pThis->iFacility << 3) + pThis->iSeverity;
	SNPRINTF(szBuf, sizeof(szBuf), "<%d>", i);
	if((iRet = sbStrBAppendStr(pStr, szBuf)) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }

	/* TIMESTAMP
	 * Please note: all SNPRINTFs allow 34 Bytes, because of
	 * a) the '\0' byte
	 * b) the extra space at the end of the TIMESTAMP
	 * So the TIMESTAMP itself will by 32 bytes at most.
	 */
	if(pThis->pszTimeStamp != NULL)
		free(pThis->pszTimeStamp);
	if((pThis->pszTimeStamp = calloc(34, sizeof(char))) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	if(iFmtToUse == srSLMGFmt_3164WELLFORMED)
	{
		SNPRINTF(pThis->pszTimeStamp,
			     34, "%s %2d %2.2d:%2.2d:%2.2d ",
				 srSLMGMonthNames[pThis->iTimStampMonth], pThis->iTimStampDay,
				 pThis->iTimStampHour, pThis->iTimStampMinute, pThis->iTimStampSecond);
	}
	else
	{ /* SIGN_12! */
		if(pThis->iTimStampSecFracPrecision > 0)
		{	/* we now need to include fractional seconds. While doing so, we must look at
			 * the precision specified. For example, if we have millisec precision (3 digits), a
			 * secFrac value of 12 is not equivalent to ".12" but ".012". Obviously, this
			 * is a huge difference ;). To avoid this, we first create a format string with
			 * the specific precision and *then* use that format string to do the actual
			 * formating (mmmmhhh... kind of self-modifying code... ;)).
			 */
			char szFmtStr[64];
			/* be careful: there is ONE actual %d in the format string below ;) */
			SNPRINTF(szFmtStr, sizeof(szFmtStr), "%%04d-%%02d-%%02dT%%02d:%%02d:%%02d.%%0%dd%%c%%02d:%%02d ",
								pThis->iTimStampSecFracPrecision);

			SNPRINTF(pThis->pszTimeStamp, 34, szFmtStr,
								pThis->iTimStampYear, pThis->iTimStampMonth, pThis->iTimStampDay,
								pThis->iTimStampHour, pThis->iTimStampMinute, pThis->iTimStampSecond, pThis->iTimStampSecFrac,
								pThis->cTimStampOffsetMode, pThis->iTimStampOffsetHour, pThis->iTimStampOffsetMinute);
		}
		else
			SNPRINTF(pThis->pszTimeStamp, 34, "%4.4d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d%c%2.2d:%2.2d ",
								pThis->iTimStampYear, pThis->iTimStampMonth, pThis->iTimStampDay,
								pThis->iTimStampHour, pThis->iTimStampMinute, pThis->iTimStampSecond,
								pThis->cTimStampOffsetMode, pThis->iTimStampOffsetHour, pThis->iTimStampOffsetMinute);
	}
	/* timestamp done, add it... */
	if((iRet = sbStrBAppendStr(pStr, pThis->pszTimeStamp)) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }

	/* HOSTNAME */
	if((iRet = sbStrBAppendStr(pStr, pThis->pszHostname)) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }
	if((iRet = sbStrBAppendChar(pStr, ' ')) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }

	/* TAG */
	if((iRet = sbStrBAppendStr(pStr, pThis->pszTag)) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }
	if(*(pThis->pszTag + strlen(pThis->pszTag) - 1) != ':')
		if((iFmtToUse == srSLMGFmt_SIGN_12) || isalnum(*(pThis->pszTag + strlen(pThis->pszTag) - 1)))
		{	/* -sign MUST be terminated by a colon, in 3164 it is not allowed to be terminated by alnum */
			if((iRet = sbStrBAppendChar(pStr, ':')) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }
		}
	


	/* MSG */
	if((iRet = sbStrBAppendStr(pStr, pThis->pszMsg)) != SR_RET_OK) { sbStrBDestruct(pStr); return iRet; }

	/* done, let's store the new string */
	pThis->pszRawMsg = sbStrBFinish(pStr);
	pThis->bOwnRawMsgBuf = TRUE;
	return SR_RET_OK;
}


srRetVal srSLMGSetFacility(srSLMGObj* pThis, int iNewVal)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);

	if(iNewVal < 0 || iNewVal > 23)
		return SR_RET_FACIL_OUT_OF_RANGE;

	pThis->iFacility = iNewVal;

	return SR_RET_OK;
}


srRetVal srSLMGSetSeverity(srSLMGObj* pThis, int iNewVal)
{
	srSLMGCHECKVALIDOBJECT_API(pThis);

	if(iNewVal < 0 || iNewVal > 7)
		return SR_RET_PRIO_OUT_OF_RANGE;

	pThis->iSeverity = iNewVal;

	return SR_RET_OK;
}


srRetVal srSLMGSetTAG(srSLMGObj* pThis, char* pszNewTag)
{
	srRetVal iRet;
	sbStrBObj *pStr;
	int i;

	srSLMGCHECKVALIDOBJECT_API(pThis);
	if(pszNewTag == NULL)
		return SR_RET_NULL_POINTER_PROVIDED;

    if((pStr = sbStrBConstruct()) == NULL) return SR_RET_OUT_OF_MEMORY;

	sbStrBSetAllocIncrement(pStr, 16);

	/* copy tag value & check validity 
	 * (we assume all chars except ':' and SP to be valid)
	 */
	for(i = 0 ; *(pszNewTag+i) && i < 32 ; ++i)
		if((*(pszNewTag+i) == ':') || (*(pszNewTag+i) == ' '))
			return SR_RET_INVALID_TAG;
		else
			if((iRet = sbStrBAppendChar(pStr, *(pszNewTag+i))) != SR_RET_OK)
				return iRet;

	if(*(pszNewTag+i) != '\0')
		/* more than 32 chars */
		return SR_RET_INVALID_TAG;

	/* done, set property */
	if(pThis->pszTag != NULL)
		free(pThis->pszTag);
	if((pThis->pszTag = sbStrBFinish(pStr)) == NULL) return SR_RET_OUT_OF_MEMORY;

	return SR_RET_OK;
}

#endif /* #if FEATURE_MSGAPI == 1 */
