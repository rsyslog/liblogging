/*! \file namevaluetree.c
 *  \brief Implemetation of the NameValueTree helper object
 *         (includes XML parser).
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          Initial version as part of liblogging 0.2 begun.
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
#include <ctype.h>
#include "settings.h"
#include "liblogging.h"
#include "namevaluetree.h"
#include "stringbuf.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */
static srRetVal sbNVTXMLProcessXMLSTREAM(char **ppXML, sbNVTRObj *pRoot);

/**
 * We can not use the usual sbNVTSearchKeySZ() method in this
 * destructor, as we would invalidate the previous pointer while
 * walking & deleting the list. So we need to walk it by using
 * the raw pointers.
 */
void sbNVTRDestroy(sbNVTRObj* pThis)
{
	sbNVTEObj* pEntry;
	sbNVTEObj* pNext;

	sbNVTRCHECKVALIDOBJECT(pThis);

	/* debug: printf("sbNVTRDestroy: %8.8x\n", pThis); */

	pEntry = pThis->pFirst;
	while(pEntry != NULL)
	{
		pNext = pEntry->pNext;
		sbNVTEDestroy(pEntry);
		pEntry = pNext;
	}
	
	/* unlink from parent! */
	sbNVTRUnlinkFromParent(pThis);

	SRFREEOBJ(pThis);
}


void sbNVTEDestroy(sbNVTEObj* pThis)
{
	sbNVTECHECKVALIDOBJECT(pThis);

	/* debug: printf("sbNVTEDestroy: %8.8x\n", pThis); */
	if(pThis->pszKey != NULL)
		free(pThis->pszKey);

	if(pThis->pszValue != NULL)
		free(pThis->pszValue);

	if(pThis->pCDATA != NULL)
		free(pThis->pCDATA);

	if(pThis->pXMLProps != NULL)
		sbNVTRDestroy(pThis->pXMLProps);

	if(pThis->pChild != NULL)
		sbNVTRDestroy(pThis->pChild);

	if(pThis->pUsr != NULL)
		(pThis->pUsrDestroy)(pThis->pUsr);

	SRFREEOBJ(pThis);
}


/**
 * Remove WHITESPACE (see ABNF \ref sbNVTRParseXML) from the XML stream.
 *
 * \param pp [in/out] pointer to XML stream. Updated to reflect new position.
 */
static void sbNVTXMLEatWHITESPACE(char **ppXML)
{
	assert(ppXML != NULL);
	while(isspace(**ppXML))
		(*ppXML)++;
}


/**
 * Retrieve an ESCSEQ (see ABNF \ref sbNVTRParseXML) from the XML stream.
 *
 * \param pp [in/out] pointer to XML stream. Updated to reflect new position.
 * \retval pointer to text of ESCSEQ or NULL, if a memory allocation error occured.
 */
static char* sbNTVXMLReadESCSEQ(char **ppXML)
{
	sbStrBObj *pStrB;

	assert(ppXML != NULL);

	if((pStrB = sbStrBConstruct()) == NULL)
		return NULL;
	sbStrBSetAllocIncrement(pStrB, 16);

	while(   (**ppXML != ';')
		  && (**ppXML != '\0'))
	{
		sbStrBAppendChar(pStrB, **ppXML);
		(*ppXML)++;
	}

	if(**ppXML != '\0')
		(*ppXML)++;	/* need to eat the ";" */

	return sbStrBFinish(pStrB);
}


/**
 * Retrieve an XMLVALUE (see ABNF \ref sbNVTRParseXML) from the XML stream.
 *
 * \param pp [in/out] pointer to XML stream. Updated to reflect new position.
 * \param cTerm [in] terminating character. This is the counterpart to the
 *                   character the initiated the call to XMLVALUE. With BEEP,
 *                   we expect it to be ', but it may also be ".
 * \retval pointer to XMLVALUE or NULL, if an error occured.
 */
static char* sbNVTXMLReadXMLVALUE(char **ppXML, char cTerm)
{
	sbStrBObj *pStrB;
	char *pszESC;

	assert(ppXML != NULL);

	if((pStrB = sbStrBConstruct()) == NULL)
		return NULL;

	while(   (**ppXML != cTerm)
		  && (**ppXML != '\0'))
	{
		if(**ppXML == '&')
		{	/* we have an escape sequence, need to handle this */
			(*ppXML)++;	/* eat the "&" */
			if((pszESC = sbNTVXMLReadESCSEQ(ppXML)) == NULL)
				return NULL;
			if(!strcmp(pszESC, "gt"))
				sbStrBAppendChar(pStrB, '>');
			else if(!strcmp(pszESC, "lt"))
				sbStrBAppendChar(pStrB, '<');
			else if(!strcmp(pszESC, "amp"))
				sbStrBAppendChar(pStrB, '&');
			else if(!strcmp(pszESC, "apos"))
				sbStrBAppendChar(pStrB, ';');
			else if(!strcmp(pszESC, "quot"))
				sbStrBAppendChar(pStrB, '"');
			else
			{
				free(pszESC);
				return NULL;
			}
			free(pszESC);
		}
		else
		{	/* just a plain character */
			sbStrBAppendChar(pStrB, **ppXML);
			(*ppXML)++;
		}
	}

	return sbStrBFinish(pStrB);

}


/**
 * Retrieve an XMLNAME (see ABNF \ref sbNVTRParseXML) from the XML stream.
 *
 * \param pp [in/out] pointer to XML stream. Updated to reflect new position.
 * \retval pointer to XMLNAME or NULL, if a memory allocation error occured.
 */
static char* sbNVTXMLReadXMLNAME(char **ppXML)
{
	sbStrBObj *pStrB;

	assert(ppXML != NULL);

	if((pStrB = sbStrBConstruct()) == NULL)
		return NULL;
	sbStrBSetAllocIncrement(pStrB, 64);

	while(   !isspace(**ppXML)
		  && (**ppXML != '\0')
		  && (**ppXML != '<')
		  && (**ppXML != '>')
		  && (**ppXML != '=')
		  && (**ppXML != ';')
		  && (**ppXML != '/'))
	{
		sbStrBAppendChar(pStrB, **ppXML);
		(*ppXML)++;
	}

	return sbStrBFinish(pStrB);
}


/**
 * Retrieve a CDATAVALUE (see ABNF \ref sbNVTRParseXML) from the XML stream.
 *
 * \param pp [in/out] pointer to XML stream. Updated to reflect new position.
 * \retval pointer to CDATA value or NULL, if a memory allocation error occured.
 */
static char* sbNVTXMLReadCDATAVALUE(char **ppXML)
{
	sbStrBObj *pStrB;

	assert(ppXML != NULL);

	if((pStrB = sbStrBConstruct()) == NULL)
		return NULL;

	while(   !isspace(**ppXML)
		  && (**ppXML != '\0')
		  && (**ppXML != ']'))
	{
		sbStrBAppendChar(pStrB, **ppXML);
		(*ppXML)++;
	}

	return sbStrBFinish(pStrB);
}


/**
 * Retrieve a TAG (see ABNF \ref sbNVTRParseXML) from the XML stream.
 *
 * \param pp [in/out] pointer to XML stream. Updated to reflect new position.
 * \retval pointer to XMLNAME or NULL, if a memory allocation error occured.
 */
static char* sbNVTXMLReadTAG(char **ppXML)
{
	return(sbNVTXMLReadXMLNAME(ppXML));
}


/**
 * Process a PARAM (see ABNF \ref sbNVTRParseXML) from the XML stream
 * and add it to the NameValueTree.
 *
 * \param ppXML [in/out] pointer to XML stream. Updated to reflect new position.
 * \param pRoot	Pointer to the list the element is to be added to.
 */
static srRetVal sbNVTXMLProcessPARAM(char **ppXML, sbNVTRObj *pRoot)
{
	char* pName;
	char *pValue;
	char cTerm;
	sbNVTEObj *pEntry;

	assert(ppXML != NULL);
	sbNVTRCHECKVALIDOBJECT(pRoot);

	if((pName = sbNVTXMLReadXMLNAME(ppXML)) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	/* check if we have a value and, if yes, retrieve it */
	if(**ppXML == '=')
	{
		(*ppXML)++; /* eat "=" */
		if(**ppXML == '\'')
			cTerm = '\'';
		else if(**ppXML == '"')
			cTerm = '"';
		else
			return SR_RET_XML_INVALID_PARAMTAG;

		(*ppXML)++; /* eat delimiter */
		if((pValue = sbNVTXMLReadXMLVALUE(ppXML, cTerm)) == NULL)
			return SR_RET_OUT_OF_MEMORY;

		/* check terminating character */
		if(**ppXML != cTerm)
			return SR_RET_XML_INVALID_TERMINATOR;

		(*ppXML)++; /* eat terminating char */
	}
	else
		pValue = NULL;

	/* OK, we have name and value. Now let's add it to the list. */
	if((pEntry = sbNVTAddEntry(pRoot)) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	/* the string buffers are handed over to the list,
	 * so they MUST NOT be freed.
	 */
	sbNVTESetKeySZ(pEntry, pName, FALSE);
	sbNVTESetValueSZ(pEntry, pValue, FALSE);

	return SR_RET_OK;
}


/**
 * Process a TAGwPARAMS (see ABNF \ref sbNVTRParseXML) from the XML stream
 * and add it to the NameValueTree.
 *
 * \param ppXML [in/out] pointer to XML stream. Updated to reflect new position.
 * \param pEntry Pointer to the current list entry.
 */
static srRetVal sbNVTXMLProcessTAGwPARAMS(char **ppXML, sbNVTEObj *pEntry)
{
	char *pTag;
	sbNVTRObj *pParamRoot = NULL;

	assert(ppXML != NULL);
	sbNVTECHECKVALIDOBJECT(pEntry);

	if((pTag = sbNVTXMLReadTAG(ppXML)) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	sbNVTESetKeySZ(pEntry, pTag, FALSE);

	sbNVTXMLEatWHITESPACE(ppXML); /* eat whitespace, if any */

	/* now check if we have params */
	while((**ppXML != '\0') && (**ppXML != '/') && (**ppXML != '>'))
	{	/* OK, we have params, so now let's retrieve
		 * and store them. We need to create a new list
		 * root for the parameter tree now if we do not
		 * have one already.
		 */
		if(pParamRoot == NULL)
			if((pParamRoot = sbNVTRConstruct()) == NULL)
				return SR_RET_OUT_OF_MEMORY;

		pEntry->pXMLProps = pParamRoot;
		sbNVTXMLProcessPARAM(ppXML, pParamRoot);
		sbNVTXMLEatWHITESPACE(ppXML); /* eat whitespace, if any */
	}

	return SR_RET_OK;
}


srRetVal sbNVTXMLEscapePCDATAintoStrB(char *pszToEscape, sbStrBObj *pStr)
{
	srRetVal iRet;
	sbSTRBCHECKVALIDOBJECT(pStr);
	
	if(pszToEscape == NULL)
		return SR_RET_OK;

	while(*pszToEscape)
	{
		if(*pszToEscape == '<')
		{
			if((iRet = sbStrBAppendStr(pStr, "&lt;")) != SR_RET_OK) return iRet;
		}
		else if(*pszToEscape == '&')
		{
			if((iRet = sbStrBAppendStr(pStr, "&quot;")) != SR_RET_OK) return iRet;
		}
		else
		{
			if((iRet = sbStrBAppendChar(pStr, *pszToEscape)) != SR_RET_OK) return iRet;
		}
		pszToEscape++;
	}

	return SR_RET_OK;
}


char* sbNVTXMLEscapePCDATA(char *pszToEscape)
{
	sbStrBObj *pStrBuf;

	if(pszToEscape == NULL)
		return NULL;

	if((pStrBuf = sbStrBConstruct()) == NULL)
		return NULL;

	if(sbNVTXMLEscapePCDATAintoStrB(pszToEscape, pStrBuf) != SR_RET_OK)
	{
		sbStrBDestruct(pStrBuf);
		return NULL;
	}

	return sbStrBFinish(pStrBuf);
}

/**
 * Process a XMLNODE and a XMLCONTAINER (see ABNF \ref sbNVTRParseXML) from the XML stream
 * and add it to the NameValueTree.
 *
 * \note Due to the syntaxes, we can not distinguish if we have a plain XMLNODE
 *       or a container at this stage. As such, this method checks if we have
 *       a container (no closing tag) and if so, it processes the container.
 *
 * \param ppXML [in/out] pointer to XML stream. Updated to reflect new position.
 * \param pEntry Pointer to the current list entry.
 */
static srRetVal sbNVTXMLProcessXMLNODE(char **ppXML, sbNVTEObj *pEntry)
{
	int iRet;
	char *pCloseTag;
	char *pValue;
	char **ppXMLSave;
	sbNVTRObj *pChildRoot = NULL;

	assert(ppXML != NULL);
	sbNVTECHECKVALIDOBJECT(pEntry);

	if((iRet = sbNVTXMLProcessTAGwPARAMS(ppXML, pEntry)) != SR_RET_OK)
		return iRet;

	if(*((*ppXML)++) == '/')
		/* we have a plain node */
		if(**ppXML == '>')
		{
			(*ppXML)++;
			return SR_RET_OK;
		}
		else
			return SR_RET_MISSING_CLOSE_BRACE;

	/* We now need to check if we have an XMLVALUE or an XMLCONTAINER.
	 * to do so, we look at the first non-whitespace chartert. If it
	 * is '<', then we have an XMLCONTAINER. If it is not, we have an
	 * XMLVALUE. In the later case, we need to go back to the front of
	 * the whitespace, as the whitespaces, too, belongs to the
	 * XMLVALUE. We use a temporary pointer to do this.
	 */

	ppXMLSave = ppXML;
	sbNVTXMLEatWHITESPACE(ppXML); /* eat whitespace, if any */
	if(**ppXML == '<')
	{	/* OK, we have a container */
		if((pChildRoot = sbNVTRConstruct()) == NULL)
			return SR_RET_OUT_OF_MEMORY;

		if((iRet = sbNVTESetChild(pEntry, pChildRoot)) != SR_RET_OK)
			return iRet;

		if((iRet = sbNVTXMLProcessXMLSTREAM(ppXML, pChildRoot)) != SR_RET_OK)
			return iRet;
	}
	else
	{	/* we have an XMLVALUE and need to retrieve that */
		ppXML = ppXMLSave; /* restore previous char position */
		if((pValue = sbNVTXMLReadXMLVALUE(ppXML, '<')) == NULL)
			return SR_RET_OUT_OF_MEMORY;

		if((iRet = sbNVTESetValueSZ(pEntry, pValue, FALSE)) != SR_RET_OK)
			return iRet;
	}

	/* now checking the close tag */
	if(**ppXML != '<')
		return SR_RET_XML_MISSING_CLOSETAG;

	(*ppXML)++;
	if(**ppXML != '/')
		return SR_RET_XML_MISSING_CLOSETAG;

	(*ppXML)++;

	/* OK, now retrieve the closing tag name */
	sbNVTXMLEatWHITESPACE(ppXML); /* eat whitespace, if any */
	if((pCloseTag = sbNVTXMLReadTAG(ppXML)) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	if(**ppXML != '>')
			return SR_RET_MISSING_CLOSE_BRACE;
	(*ppXML)++;	/* eat ">" */
	
	/* check if start and end tag match */

	if(strcmp(pEntry->pszKey, pCloseTag))
		return SR_RET_XML_TAG_MISMATCH;

	free(pCloseTag);	/* no longer needed */

	return SR_RET_OK;
}


/**
 * Process a CDATA (see ABNF \ref sbNVTRParseXML) from the XML stream
 * and add it to the NameValueTree.
 *
 * \param ppXML [in/out] pointer to XML stream. Updated to reflect new position.
 *                       Needs to point at the first [ in the stream.
 * \param pEntry Pointer to the current list entry.
 */
static srRetVal sbNVTXMLProcessCDATA(char **ppXML, sbNVTEObj *pEntry)
{
	assert(ppXML != NULL);
	sbNVTECHECKVALIDOBJECT(pEntry);

	/* check header */
	if(**ppXML != '[')
		return SR_RET_XML_INVALID_CDATA_HDR;
	if(*(++*ppXML) != 'C')
		return SR_RET_XML_INVALID_CDATA_HDR;
	if(*(++*ppXML) != 'D')
		return SR_RET_XML_INVALID_CDATA_HDR;
	if(*(++*ppXML) != 'A')
		return SR_RET_XML_INVALID_CDATA_HDR;
	if(*(++*ppXML) != 'T')
		return SR_RET_XML_INVALID_CDATA_HDR;
	if(*(++*ppXML) != 'A')
		return SR_RET_XML_INVALID_CDATA_HDR;
	if(*(++*ppXML) != '[')
		return SR_RET_XML_INVALID_CDATA_HDR;
	(*ppXML)++;	/* eat '[' */

	/* Now retrieve & set CDATA pointer. We should eventually
	 * create a specific method for this, but on the other
	 * hand it is only done in exactly this one case...
	 */
	if((pEntry->pCDATA = sbNVTXMLReadCDATAVALUE(ppXML)) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	
	/* check trailer */
	if(**ppXML != ']')
		return SR_RET_XML_INVALID_CDATA_TRAIL;
	if(*(++*ppXML) != ']')
		return SR_RET_XML_INVALID_CDATA_TRAIL;
	if(*(++*ppXML) != '>')
		return SR_RET_XML_INVALID_CDATA_TRAIL;
	(*ppXML)++;	/* eat '<' */

	return SR_RET_OK;
}

/**
 * Process a XMLELEMENT (see ABNF \ref sbNVTRParseXML) from the XML stream
 * and add it to the NameValueTree.
 *
 * \param ppXML [in/out] pointer to XML stream. Updated to reflect new position.
 * \param pEntry Pointer to the current list entry.
 */
static srRetVal sbNVTXMLProcessXMLELEMENT(char **ppXML, sbNVTEObj *pEntry)
{
	srRetVal iRet;

	assert(ppXML != NULL);
	sbNVTECHECKVALIDOBJECT(pEntry);

	if(**ppXML != '<')
		return SR_RET_XML_MISSING_OPENTAG;
	(*ppXML)++;	/* eat '<' */

	if(**ppXML == '!')
	{
		(*ppXML)++; /* eat '!' */
		if((iRet = sbNVTXMLProcessCDATA(ppXML, pEntry)) != SR_RET_OK)
			return iRet;
	}
	else
		if((iRet = sbNVTXMLProcessXMLNODE(ppXML, pEntry)) != SR_RET_OK)
			return iRet;

	sbNVTXMLEatWHITESPACE(ppXML); /* eat whitespace, if any */
	
	return SR_RET_OK;
}

/**
 * Process a XMLSTREAM (see ABNF \ref sbNVTRParseXML) from the XML stream
 * and add it to the NameValueTree.
 *
 * This creates one new entry element for each entree it finds.
 *
 * \param ppXML [in/out] pointer to XML stream. Updated to reflect new position.
 * \param pRoot Pointer to the current list root.
 */
static srRetVal sbNVTXMLProcessXMLSTREAM(char **ppXML, sbNVTRObj *pRoot)
{
	srRetVal iRet;
	sbNVTEObj *pEntry;

	assert(ppXML != NULL);
	sbNVTRCHECKVALIDOBJECT(pRoot);

	while((**ppXML != '\0') && !((**ppXML == '<') && (*(*ppXML + 1)) == '/'))
	{
		sbNVTXMLEatWHITESPACE(ppXML); /* eat whitespace, if any */

		if(**ppXML == '\0')
		{
			return SR_RET_OK;
		}
		if((pEntry = sbNVTAddEntry(pRoot)) == NULL)
			return SR_RET_OUT_OF_MEMORY;

        if((iRet = sbNVTXMLProcessXMLELEMENT(ppXML, pEntry)) != SR_RET_OK)
			return iRet;
	}

	return SR_RET_OK;
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */


 sbNVTRObj* sbNVTRConstruct(void)
 {
	 sbNVTRObj *pThis;

	 if((pThis = calloc(1, sizeof(sbNVTRObj))) == NULL)
		 return NULL;

	 pThis->OID = OIDsbNVTR;
	 pThis->pFirst = NULL;
	 pThis->pLast = NULL;
	 pThis->pParent = NULL;

	 /* debug: printf("sbNVTRConstuct: %8.8x\n", pThis); */
	 return(pThis);
 }


 sbNVTEObj* sbNVTEConstruct(void)
 {
	 sbNVTEObj *pThis;

	 if((pThis = calloc(1, sizeof(sbNVTEObj))) == NULL)
		 return NULL;

	 pThis->OID = OIDsbNVTE;
	 pThis->pCDATA = NULL;
	 pThis->pChild = NULL;
	 pThis->pNext = NULL;
	 pThis->pszKey = NULL;
	 pThis->pszValue = NULL;
	 pThis->pUsr = NULL;
	 pThis->pXMLProps = NULL;
	 pThis->uKey = 0;
	 pThis->uKeyPresent = FALSE;
	 pThis->uValue = 0;
	 pThis->bIsSetUValue = FALSE;

	 /* debug: printf("sbNVTeConstuct: %8.8x\n", pThis); */
	 return(pThis);
 }


srRetVal sbNVTESetChild(sbNVTEObj *pEntry, sbNVTRObj *pChildRoot)
 {
	sbNVTECHECKVALIDOBJECT(pEntry);
	sbNVTRCHECKVALIDOBJECT(pChildRoot);

	pEntry->pChild = pChildRoot;

	return SR_RET_OK;
 }


 sbNVTEObj* sbNVTAddEntry(sbNVTRObj* pRoot)
 {
	sbNVTEObj * pThis;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	if((pThis = sbNVTEConstruct()) == NULL)
		 return NULL;

	 if(pRoot->pLast == NULL)
	 {	/* empty list */
		 pRoot->pLast = pRoot->pFirst = pThis;
	 }
	 else
	 {	/* add to list */
		sbNVTECHECKVALIDOBJECT(pRoot->pLast);
		assert(pRoot->pLast->pNext == NULL);

		pRoot->pLast->pNext = pThis;
		pRoot->pLast = pThis;
	 }
	 return(pThis);
 }


 srRetVal sbNVTRRemoveFirst(sbNVTRObj* pRoot)
 {
	sbNVTEObj *pEntry;
	sbNVTRCHECKVALIDOBJECT(pRoot);

	if((pEntry = sbNVTUnlinkElement(pRoot)) != NULL)
		sbNVTEDestroy(pEntry);

	return SR_RET_OK;
 }


 sbNVTEObj* sbNVTUnlinkElement(sbNVTRObj* pRoot)
 {
	sbNVTEObj *pThis;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	if(pRoot->pFirst == NULL)
		return NULL;

	pThis = pRoot->pFirst;
	pRoot->pFirst = pThis->pNext;
	if(pRoot->pFirst == NULL)
		pRoot->pLast = NULL;

	return(pThis);
 }



sbNVTEObj* sbNVTSearchKeySZ(sbNVTRObj* pRoot, sbNVTEObj* pStart, char* pszSearch)
{
	sbNVTEObj *pThis;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	pThis = (pStart == NULL) ? pRoot->pFirst : pStart->pNext;

	while(pThis != NULL)
	{
		if(pszSearch == NULL)
			break;
		/* attention: order of conditions is vitally important! */
		if((pThis->pszKey != NULL) && !strcmp(pThis->pszKey, pszSearch))
				break; /* found it! */
		pThis = pThis->pNext;
	}

	return(pThis);
}



sbNVTEObj* sbNVTSearchKeyU(sbNVTRObj* pRoot, sbNVTEObj* pStart, unsigned uSearch)
{
	sbNVTEObj *pThis;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	pThis = (pStart == NULL) ? pRoot->pFirst : pStart->pNext;

	while(pThis != NULL)
	{
		if((pThis->uKeyPresent == TRUE) && (pThis->uKey == uSearch))
			break; /* found it! */
		pThis = pThis->pNext;
	}

	return(pThis);
}


sbNVTEObj* sbNVTSearchKeyUAndPrev(sbNVTRObj* pRoot, sbNVTEObj* pStart, unsigned uSearch, sbNVTEObj** ppPrevEntry)
{
	sbNVTEObj *pThis;
	sbNVTEObj *pPrev = NULL;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	pThis = (pStart == NULL) ? pRoot->pFirst : pStart->pNext;

	while(pThis != NULL)
	{
		if((pThis->uKeyPresent == TRUE) && (pThis->uKey == uSearch))
			break; /* found it! */
		pPrev = pThis;
		pThis = pThis->pNext;
	}

	*ppPrevEntry = pPrev;
	return(pThis);
}

sbNVTEObj* sbNVTSearchpUsrAndPrev(sbNVTRObj* pRoot, sbNVTEObj* pStart, void *pUsr, sbNVTEObj** ppPrevEntry)
{
	sbNVTEObj *pThis;
	sbNVTEObj *pPrev = NULL;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	pThis = (pStart == NULL) ? pRoot->pFirst : pStart->pNext;

	while(pThis != NULL)
	{
		if(pThis->pUsr == pUsr)
			break; /* found it! */
		pPrev = pThis;
		pThis = pThis->pNext;
	}

	*ppPrevEntry = pPrev;
	return(pThis);
}


char* sbNVTEUtilStrDup(char *pszStrToDup)
{
	size_t iLen;
	char* pszBuf;

	assert(pszStrToDup != NULL);

	iLen = strlen(pszStrToDup);
	/* we use malloc() below for performance
	 * reasons - after all, the buffer is immediately
	 * overwritten.
	 */
	if((pszBuf = malloc((iLen+1) * sizeof(char))) == NULL)
		return NULL;
	memcpy(pszBuf, pszStrToDup, (iLen+1) * sizeof(char));
	return(pszBuf);
}


srRetVal sbNVTESetKeySZ(sbNVTEObj* pThis, char* pszKey, int bCopy)
{
	char *pszBuf;
	sbNVTECHECKVALIDOBJECT(pThis);

	if(bCopy == TRUE)
	{
		if((pszBuf = sbNVTEUtilStrDup(pszKey)) == NULL)
			return SR_RET_ERR;
	}
	else
		pszBuf = pszKey;

	if(pThis->pszKey != NULL)
		free(pThis->pszKey);

	pThis->pszKey = pszBuf;
	return SR_RET_OK;
}


srRetVal sbNVTESetValueSZ(sbNVTEObj* pThis, char* pszVal, int bCopy)
{
	char *pszBuf;
	sbNVTECHECKVALIDOBJECT(pThis);

	if(bCopy == TRUE)
	{
		if((pszBuf = sbNVTEUtilStrDup(pszVal)) == NULL)
			return SR_RET_ERR;
	}
	else
		pszBuf = pszVal;

	if(pThis->pszValue != NULL)
		free(pThis->pszValue);

	pThis->pszValue = pszBuf;
	return SR_RET_OK;
}


srRetVal sbNVTESetKeyU(sbNVTEObj* pThis, unsigned uKey)
{
	sbNVTECHECKVALIDOBJECT(pThis);

	pThis->uKey = uKey;
	pThis->uKeyPresent = TRUE;
	return SR_RET_OK;
}


srRetVal sbNVTEUnsetKeyU(sbNVTEObj* pThis)
{
	sbNVTECHECKVALIDOBJECT(pThis);

	pThis->uKeyPresent = FALSE;
	return SR_RET_OK;
}

srRetVal sbNVTESetUsrPtr(sbNVTEObj* pThis, void *pPtr, void(pPtrDestroy)(void*))
{
	sbNVTECHECKVALIDOBJECT(pThis);

	if(pPtrDestroy == NULL)
		return SR_RET_INVALID_DESTRUCTOR;

	pThis->pUsr = pPtr;
	pThis->pUsrDestroy = pPtrDestroy;

	return SR_RET_OK;
}


srRetVal sbNVTESetValueU(sbNVTEObj* pThis, unsigned uVal)
{
	sbNVTECHECKVALIDOBJECT(pThis);

	pThis->uValue = uVal;
	pThis->bIsSetUValue = TRUE;
	return SR_RET_OK;
}


srRetVal sbNVTEGetValueU(sbNVTEObj* pThis, unsigned* puValue)
{
	char *pBuf;
	unsigned u;

	sbNVTECHECKVALIDOBJECT(pThis);
	assert(puValue != NULL);

	if(pThis->bIsSetUValue)
	{
		*puValue = pThis->uValue;
	}
	else
	{	/* try to convert the szValue */
		if(pThis->pszValue == NULL)
			return SR_RET_NO_VALUE;
		else
		{
			pBuf = pThis->pszValue;
			u = 0;
			while(*pBuf)
			{
				if(!isdigit(*pBuf))
					return SR_RET_NO_VALUE;
				u = u * 10 + (*pBuf - '0');
				pBuf++;
			}
			pThis->uValue = u;
			/* Please note: this can cause an error if the szValue
			 * is being re-set. Things then go out of sync. This is
			 * no issue for the current library, but may be worth
			 * looking into if the lib use is extended.
			 */
			pThis->bIsSetUValue = TRUE;
			*puValue = u;
		}
	}
		
	return SR_RET_OK;
}


void sbNVTEUnsetUsrPtr(sbNVTEObj *pEntry)
{
	sbNVTECHECKVALIDOBJECT(pEntry);

	pEntry->pUsr = NULL;
	pEntry->pUsrDestroy = NULL;

}


void sbNVTRUnlinkFromParent(sbNVTRObj *pRoot)
{
	sbNVTRCHECKVALIDOBJECT(pRoot);

	if(pRoot->pParent != NULL)
		pRoot->pParent->pChild = NULL;
}


void sbNVTEUnlinkFromList(sbNVTRObj *pRoot, sbNVTEObj* pEntry, sbNVTEObj* pPrev)
{
	sbNVTRCHECKVALIDOBJECT(pRoot);
	sbNVTECHECKVALIDOBJECT(pEntry);

	if(pPrev == NULL)
	{
		pRoot->pFirst = pEntry->pNext;
		if(pRoot->pLast == pEntry)
			pRoot->pLast = NULL;
	}
	else
	{
		pPrev->pNext = pEntry->pNext;
		if(pRoot->pLast == pEntry)
			pRoot->pLast = pPrev;
	}
}


srRetVal sbNVTRRemoveKeyU(sbNVTRObj *pRoot, unsigned uKey)
{
	sbNVTEObj *pEntry, *pPrev;

	sbNVTRCHECKVALIDOBJECT(pRoot);
	
	if((pEntry = sbNVTSearchKeyUAndPrev(pRoot, NULL, uKey, &pPrev)) == NULL)
		return SR_RET_NOT_FOUND;

	sbNVTEUnlinkFromList(pRoot, pEntry, pPrev);
	sbNVTEDestroy(pEntry);

	return SR_RET_OK;	
}


srRetVal sbNVTRRemovEntryWithpUsr(sbNVTRObj *pRoot, void* pUsr)
{
	sbNVTEObj *pEntry, *pPrev;

	sbNVTRCHECKVALIDOBJECT(pRoot);
	
	if((pEntry = sbNVTSearchpUsrAndPrev(pRoot, NULL, pUsr, &pPrev)) == NULL)
		return SR_RET_NOT_FOUND;

	sbNVTEUnlinkFromList(pRoot, pEntry, pPrev);
	sbNVTEDestroy(pEntry);

	return SR_RET_OK;	
}

srRetVal sbNVTRParseXML(sbNVTRObj *pRoot, char *pszXML)
{
	srRetVal iRet;

	sbNVTRCHECKVALIDOBJECT(pRoot);

	if(pszXML == NULL)
		/* this does not make much sense, but is
		 * a valid case. So let's do it ;)
		 */
		return SR_RET_OK;

	if((iRet = sbNVTXMLProcessXMLSTREAM(&pszXML, pRoot)) != SR_RET_OK)
			return iRet;

	return SR_RET_OK;
}


#if DEBUGLEVEL
/* Helper to srNVTDebugPrintTree - indents a new line
 * by iLevel*2 spaces.
 */
static void sbNVTDebugPrintTreeSpacer(int iLevel)
{
	int i;
	for(i = 0 ; i < iLevel * 2 ; ++i)
	{
		putchar(' ');
	}
}

void sbNVTDebugPrintTree(sbNVTRObj *pRoot, int iLevel)
{
	sbNVTEObj *pEntry = NULL;

	while((pEntry = sbNVTSearchKeySZ(pRoot, pEntry, NULL)) != NULL)
	{
		sbNVTDebugPrintTreeSpacer(iLevel);
		printf("KeySZ: '%s', ValueSZ '%s'\n", pEntry->pszKey, pEntry->pszValue);
		if(pEntry->uKeyPresent)
		{
			sbNVTDebugPrintTreeSpacer(iLevel);
			printf("KeyU: '%d', ValueU '%d'\n", pEntry->uKey, pEntry->uValue);
		}
		if(pEntry->pCDATA != NULL)
		{
			sbNVTDebugPrintTreeSpacer(iLevel);
			printf("CDATA: '%s'\n", pEntry->pCDATA);
		}
		if(pEntry->pXMLProps != NULL)
		{
			sbNVTDebugPrintTreeSpacer(iLevel);
			printf("HAS XML Properties:\n");
			sbNVTDebugPrintTree(pEntry->pXMLProps, iLevel + 1);
		}
		if(pEntry->pChild != NULL)
		{
			sbNVTDebugPrintTreeSpacer(iLevel);
			printf("HAS Child:\n");
			sbNVTDebugPrintTree(pEntry->pChild, iLevel + 1);
		}
	}
}
/* #endif for #if DEBUGLEVEL  */
#endif


sbNVTEObj *sbNVTRHasElement(sbNVTRObj* pRoot, char *pEltname, int bMustBeOnly)
{
	sbNVTEObj *pEntry;

	sbNVTRCHECKVALIDOBJECT(pRoot);
	assert(pEltname != NULL);

	if((bMustBeOnly == TRUE) && (pRoot->pFirst != pRoot->pLast))
		return NULL;

	if((pEntry = sbNVTSearchKeySZ(pRoot, NULL, pEltname)) == NULL)
		return NULL;

	return pEntry;
}
