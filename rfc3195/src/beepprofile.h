/*! \file beepprofile.h
 *  \brief The BEEP Profile Object
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *
 * \date    2003-09-04
 *          Updated so that a client can have multiple
 *          profiles.
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
#ifndef __LIB3195_BEEPPROFILE_H_INCLUDED__
#define __LIB3195_BEEPPROFILE_H_INCLUDED__ 1
#define sbProfCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbProf);}

struct sbMesgObject;
struct sbNVTRObject;
struct srAPIObject;
struct srSLMGObject;

#if FEATURE_LISTENER == 1
	/** event handlers */
	enum sbProfEvent_
	{
		sbPROFEVENT_ONMESGRECV,
		sbPROFEVENT_ONCHANCREAT,
		sbPROFEVENT_UNKNOWN = 0		/**< should never be used, included as saveguard (calloc()!). */
	};
	typedef enum sbProfEvent_ sbProfEvent;

#endif

struct sbProfObject
/** The BEEP Profile object. Implemented via \ref beepprofile.h and
 *  \ref beepprofile.c.
 */
{	
	srObjID OID;				/**< object ID */
	char* pszProfileURI;		/**< pointer to the URI to be used during negotiation */
#if FEATURE_LISTENER == 1
	int bDestroyOnChanClose;	/**< this profile should be destroyed when the channel is closed */
	struct srAPIObject *pAPI;	/**< pointer to associated API object */
	/* now come the event handlers */
	/** called, when the channel is initially created. (from the <start> handler). Is responsible
	 ** for sending any profile-specific greeting.
	 **/
	srRetVal (*OnChanCreate)(struct sbProfObject *pThis, struct sbSessObject* pSess, struct sbChanObject* pChan);
	/** called whenever a new complete message arrives. Must reply according to
	 ** profile spec.
	 **/
	srRetVal (*OnMesgRecv)(struct sbProfObject *pThis, int* pAbort, struct sbSessObject* pSess, struct sbChanObject* pChan, struct sbMesgObject *pMesg);
#endif
	/* now come client-side event handlers (always present) */
	/** method to call if client-side API needs to open a log channel */ 
	srRetVal (*OnClntOpenLogChan)(struct sbChanObject *pChan);
	/** method to call if client-side API needs to send a syslog message */ 
	srRetVal (*OnClntSendLogMsg)(struct sbChanObject* pChan, char* szLogmsg);
	/** method to call if client-side API needs to send a srSLMGObj */ 
	srRetVal (*OnClntSendSLMG)(struct sbChanObject* pChan, struct srSLMGObject *pSLMG);
	/** method to call if client-side API needs to close a log channel */
	srRetVal (*OnClntCloseLogChan)(struct sbChanObject* pChan);
};
typedef struct sbProfObject sbProfObj;

/** Constructor to create a sbProf.
 *
 * \param ppThis [out] pointer to a buffer that will receive
 *                     the pointer to the newly created profile
 *                     object.
 * \param szURI [in] - name of uri to be used in greeting. May
 *                     be NULL for a profile, that should not be
 *                     advertised (channel 0 is a meaningful sample).
 *
 */
srRetVal sbProfConstruct(sbProfObj** ppThis, char *pszURI);

/**
 * Return the profile URI. The returned string is read-only.
 * \retval URI profile string
 */
char* sbProfGetURI(sbProfObj* pThis);

/**
 * Profile Object Destructor.
 */
void sbProfDestroy(sbProfObj* pThis);

/**
 * Set and unset an event handler. A new handler is set
 * for the specified event. To unset it, provide NULL as
 * the handler pointer.
 *
 * \param iEvent Event ID to be modified
 * \param handler Pointer to event handler. Must follow the semantics
 *                of this specific event (will not be checked)
 */
srRetVal sbProfSetEventHandler(struct sbProfObject* pThis, sbProfEvent iEvent, srRetVal (*handler)());

/**
 * Set the associated API object.
 * \param pAPI - API object to be set.
 */
srRetVal sbProfSetAPIObj(sbProfObj *pThis, srAPIObj *pAPI);

/**
 * Find a matching profile in two lists of profiles.
 * The matching is done based on the URI.
 * If a match is found, we return the pointer to it. If it is
 * not found, we return NULL. The FIRST matching profile
 * will be returned. It is not tried to find the best match.
 *
 * \param pProfListRemote List of remote profiles, created from the
 *                        greeting. This are NOT profile objects but
 *                        rather URI strings in pszKey.
 * \param pProfList2 list (sbNVTRObj) of profiles. MAY BE NULL! (which indicates
 *                  an empty list of profiles. 
 * \retval Pointer to the first matching entry or NULL, if none
 *                 found.
 */
sbProfObj*  sbProfFindProfileMatch(struct sbNVTRObject *pProfListRemote, struct sbNVTRObject *pProfList2);

/**
 * Find a profile based on an URI in a list of profiles.
 *
 * \param pProfileList Pointer to list of Profiles. May be NULL.
 * \param pszSearch string to be compared to. Must not be NULL.
 * \retval Pointer to the profile found or NULL, if none found.
 */
sbProfObj*  sbProfFindProfile(struct sbNVTRObject *pProfList, char* pszSearch);

/**
 * Set the 4 mandatory event handlers for client profiles.
 * As all of them need to be set for the profile to work,
 * we provide a single call to set them all at once.
 *
 * The param names reflect the names of the event handlers. 
 * All event handlers MUST point to actual handlers, none
 * if them is allowed to be NULL.
 */
srRetVal sbProfSetClntEventHandlers(sbProfObj *pProf,
									srRetVal (*OnClntOpenLogChan)(struct sbChanObject *pChan),
									srRetVal (*OnClntSendLogMsg)(struct sbChanObject* pChan, char* szLogmsg),
									srRetVal (*OnClntSendSLMG)(struct sbChanObject* pChan, struct srSLMGObject *pSLMG),
									srRetVal (*OnClntCloseLogChan)(struct sbChanObject* pChan));

/**
 * Take a list of profile objects AND a list
 * of (remotely supported) profile URIs and
 * find a match in them. The matching is done
 * so that the first local profile found in the
 * remote URI list will be returned. If there
 * are multiple matches, all other matches will
 * be IGNORED.
 *
 * \param pProfListLocal List of locally supported profile
 *        objects.
 *
 * \param pURIListRemote List of remotely supported
 *        profile URIs.
 *
 * \retval The matching profile object from the
 *         local profile object.
 */
sbProfObj*  sbProfFindProfileURIMatch(struct sbNVTRObject *pProfListLocal, struct sbNVTRObject *pURIListRemote);


#endif
