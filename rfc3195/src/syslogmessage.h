/*! \file syslogmessage.h
 *  \brief The syslog message API.
 *
 * This file describes the syslog message object. This
 * object is also part of the liblogging API and as such
 * a upper-layer caller may directly invoke all public
 * methods of this object.
 *
 * The syslog message object (srSLMGObj) is the container
 * wrapping the actual syslog message. It wraps any message,
 * be it plain RFC 3164, 3195, -sign or whatever is to come.
 * The message object contains properties used by any and all
 * syslog protocols. The idea is to have the message object
 * independent from the transport (which is taken care of by
 * \ref srAPI.h).
 *
 * \note This object supports 8-bit syslog message. When sending,
 * the object looks at its settings to see how non-US-ASCII
 * messages should be processed. Idially, it uses syslog-international
 * (which, of course, is not available at the time of this writing).
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
#ifndef __LIB3195_SYSLOGMESSAGE_H_INCLUDED__
#define __LIB3195_SYSLOGMESSAGE_H_INCLUDED__ 1

#ifdef __cplusplus
extern "C" {
#endif

#define srSLMGCHECKVALIDOBJECT_API(x) {if((x) == NULL) return SR_RET_NULL_POINTER_PROVIDED; if((x)->OID != OIDsrSLMG) return SR_RET_INVALID_HANDLE;}
#define srSLMGCHECKVALIDOBJECT(x) {assert((x) != NULL);assert((x)->OID == OIDsrSLMG);}

#if (FEATURE_LISTENER == 1) || (FEATURE_MSGAPI == 1)

enum srSLMGFormat_
{
	srSLMGFmt_Invalid = 0,		/**< should never occur, probaly indicates leftover from calloc() 
								 ** If a message is in this format, nothing but the raw message
								 ** and the facility and priority is valid
								 **/
	srSLMGFmt_3164RAW = 100,	/**< message seems to be 3164 format, but not wellformed */
	srSLMGFmt_3164WELLFORMED = 101,	/**< message seems to be 3164 format and is wellformed */
	srSLMGFmt_SIGN_12 = 200		/**< message seems to be ID sign-12 format */
};
typedef enum srSLMGFormat_ srSLMGFormat;

enum srSLMGTimStampType_
{
	srSLMG_TimStamp_INVALID = 0,		/**< should never be used, value from calloc() */
	srSLMG_TimStamp_3164 = 1,
	srSLMG_TimStamp_3339 = 2
};
typedef enum srSLMGTimStampType_ srSLMGTimStampType;

enum srSLMGSource_
{
	srSLMG_Source_INVALID = 0,		/**< should never be used, value from calloc() */
	srSLMG_Source_OWNGenerated = 1,
	srSLMG_Source_BEEPRAW = 2,
	srSLMG_Source_BEEPCOOKED = 3,
	srSLMG_Source_UDP = 4,
	srSLMG_Source_UX_DFLT_DOMSOCK = 5,
};
typedef enum srSLMGSource_ srSLMGSource;

/**
 * The syslog message object used by the listener
 * callback and probably (later) other parts of the
 * API).
 */
struct srSLMGObject
{
	srObjID OID;					/**< object ID */
	unsigned char *pszRawMsg;		/**< the raw message provided by the sender */
	int bOwnRawMsgBuf;				/**< TRUE pszRawMsg owned by us & must be freed on destroy */
	unsigned char *pszRemoteHost;	/**< remote host we received the message from (from socket layer, NOT message) */
	int bOwnRemoteHostBuf;			/**< TRUE pszRemoteHost owned by us & must be freed on destroy */
	srSLMGFormat iFormat;			/**< the format this message (most probably) is in */
	srSLMGSource iSource;			/**< the source this syslog message was received from */
#	if FEATURE_MSGAPI == 1
	int iFacility;					/**< the facility of the syslog message */
	int iSeverity;					/**< the priority of the syslog message */
	unsigned char* pszHostname;		/**< name of the remote host (from the syslog message itself) */
	unsigned char* pszTag;			/**< pointer to the tag value of the syslog message */
	unsigned char* pszMsg;			/**< the MSG part of the syslog message */
	int bOwnMsg;					/**< TRUE pszMsg owned by us & must be freed on destroy */
	unsigned char* pszLanguage;		/**< language used inside the message (NULL, if unknown) */
	
	/* The timestamp is split through several fields, because this makes it less
	 * ambigious and is (hopefully) more portable.
	 */
	srSLMGTimStampType iTimStampType;
	int iTimStampYear;
	int iTimStampMonth;
	int iTimStampDay;
	int iTimStampHour;
	int iTimStampMinute;
	int iTimStampSecond;
	int iTimStampSecFrac;
	int iTimStampSecFracPrecision;	/**< the precision of timefrac in nmbr of digits (e.g. to differentiate ".1" from ".001" ;)) */
	int iTimStampOffsetHour;
	int iTimStampOffsetMinute;
	char cTimStampOffsetMode;		/* '+'/'-' */
	int	bTimStampIncludesTZ;		/**< indicates if the message timestamp included timezone information (TRUE=yes) */
	char *pszTimeStamp;				/**< the timestamp in string format */

#	endif
};
typedef struct srSLMGObject srSLMGObj;


/**
 * Construct a srSLMGObj. This is a utility for the lower 
 * layers.
 *
 * \param ppThis pointer to a pointer that will receive the
 * address of the newly constructed object (or NULL, if an
 * error occured).
 */
srRetVal srSLMGConstruct(srSLMGObj **ppThis);

/**
 * Destroy an SRLSMGObj.
 */
void srSLMGDestroy(srSLMGObj *pThis);

/**
 * Provide the caller back with the priority of this syslog message.
 *
 * \param piPrio Pointer to integer to receive the priority.
 */
srRetVal srSLMGGetPriority(srSLMGObj *pThis, int *piPrio);

/**
 * Provide the caller back with the facility of this syslog message.
 *
 * \param piFac Pointer to integer to receive the facility.
 */
srRetVal srSLMGGetFacility(srSLMGObj *pThis, int *piFac);

/**
 * Provide the caller back with the remote host that sent
 * us this syslog message. Please note that this is the
 * socket layer peer. It is not necessarily the same host
 * that is specified as originator in the syslog message
 * itself. If the message is relayed, these two are definitely
 * different.
 *
 * \note The returned string is read-only. It is valid
 * only until the srSLMGObj is destroyed. If the caller intends
 * to use it for a prolonged period of time, it needs to
 * copy it!
 *
 * \param ppsz Pointer to Pointer to unsigned char to 
 *             receive the return string.
 *
 * \note If the message originated via inter-process communiction on
 *       the same machine (e.g. UNIX /dev/log), the returned
 *       pointer is NULL. The caller needs to check for this!
 */
srRetVal srSLMGGetRemoteHost(srSLMGObj *pThis, char**ppsz);

/**
 * Provide the caller back with the message sender name
 * from the syslog message. Please note that this is NOT
 * necessarily the same host that physically send the
 * message to us. See \ref srSLMGSenderName for details.
 *
 * \note The returned string is read-only. It is valid
 * only until the srSLMGObj is destroyed. If the caller intends
 * to use it for a prolonged period of time, it needs to
 * copy it!
 *
 * \param ppsz Pointer to Pointer to unsigned char to 
 *             receive the return string.
 */
srRetVal srSLMGGetHostname(srSLMGObj *pThis, char**ppsz);

/**
 * Provide the caller back with the message's tag.
 * Please note that the TAG is not necessarily available,
 * e.g. it is not if we have a "raw" (non-wellformed)
 * 3164 message. SR_RET_PROPERTY_NOT_AVAILABLE is 
 * returned in this case.
 *
 * \note The returned string is read-only. It is valid
 * only until the srSLMGObj is destroyed. If the caller intends
 * to use it for a prolonged period of time, it needs to
 * copy it!
 *
 * \param ppsz Pointer to Pointer to unsigned char to 
 *             receive the return string.
 */
srRetVal srSLMGGetTag(srSLMGObj *pThis, unsigned char**ppsz);

/**
 * Provide the caller back with the message's MSG.
 * Please note that the MSG is not necessarily available,
 * e.g. it is not if we have a "raw" (non-wellformed)
 * 3164 message. SR_RET_PROPERTY_NOT_AVAILABLE is 
 * returned in this case. Use the RawMessage in this case.
 *
 * \note The returned string is read-only. It is valid
 * only until the srSLMGObj is destroyed. If the caller intends
 * to use it for a prolonged period of time, it needs to
 * copy it!
 *
 * \param ppsz Pointer to Pointer to unsigned char to 
 *             receive the return string.
 */
srRetVal srSLMGGetMSG(srSLMGObj *pThis, unsigned char**ppsz);

/**
 * Parse a syslog message into its fields. The syslog message
 * must already be stored in pszRawMesg. All other parts
 * will be extracted and brought to their own buffers.
 */
srRetVal srSLMGParseMesg(srSLMGObj *pThis);

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
srRetVal srSLMGGetRawMSG(srSLMGObj *pThis, unsigned char**ppsz);

/**
 * Set the Remote Host address, in this case to an IP address.
 * Any previously set value is replaced.
 *
 * \param pszRemHostIP String pointing to the IP address of the remote
 * host.
 * \param bCopyRemHost TRUE = copy the string, will own the copy
 *                     (and need to free it), FALSE, do not copy
 *                     caller owns & frees string. Must keep srSLMGObj
 *                     alive long enough so that all methods can access
 *                     it without danger.
 */
srRetVal srSLMGSetRemoteHostIP(srSLMGObj *pThis, char *pszRemHostIP, int bCopyRemHost);

/**
 * Set the raw message text. Any previously set value is replaced.
 *
 * \param pszRawMsg String with the raw message text.
 * \param bCopyRawMsg TRUE = copy the string, will own the copy
 *                     (and need to free it), FALSE, do not copy
 *                     caller owns & frees string. Must keep srSLMGObj
 *                     alive long enough so that all methods can access
 *                     it without danger.
 */
srRetVal srSLMGSetRawMsg(srSLMGObj *pThis, char *pszRawMsg, int bCopyRawMsg);

/**
 * Set the MSG text. Any previously set value is replaced.
 *
 * \param pszMsg String with the MSG text.
 * \param bCopyMSG     TRUE = copy the string, will own the copy
 *                     (and need to free it), FALSE, do not copy
 *                     caller owns & frees string. Must keep srSLMGObj
 *                     alive long enough so that all methods can access
 *                     it without danger.
 */
srRetVal srSLMGSetMSG(srSLMGObj *pThis, char *pszMSG, int bCopyMSG);

/** 
 * Set the TIMESTAMP to the current system time.
 */
srRetVal srSLMGSetTIMESTAMPtoCurrent(srSLMGObj *pThis);

/**
 * Set the HOSTNAME to the current hostname.
 */
srRetVal srSLMGSetHOSTNAMEtoCurrent(srSLMGObj* pThis);

/**
 * Format a raw syslog message. This object's properties are
 * used to format the message. It will be placed in the raw
 * message buffer and after that is ready for transmittal.
 *
 * If a raw message is already stored, the new raw message
 * overwrites the previous one. If the previous message resides
 * in user-allocated memory, it can not be replaced and an
 * error condition is returned. In this case, remove the link
 * to the user-supplied buffer first and then retry the operation.
 *
 * \param IFmtToUse The format to be used for this message. It
 *        must be one of the well-known formats.
 */
srRetVal srSLMGFormatRawMsg(srSLMGObj *pThis, srSLMGFormat iFmtToUse);

/**
 * Set the priority for the current message. Any
 * previously stored priority code is overwritten.
 */
srRetVal srSLMGSetSeverity(srSLMGObj* pThis, int iNewVal);

/**
 * Set the facility for the current message. Any
 * previously stored facility code is overwritten.
 */
srRetVal srSLMGSetFacility(srSLMGObj* pThis, int iNewVal);

/**
 * Set the TAG string in the syslog message. The
 * caller provided string buffer is copied to our
 * own local copy.
 *
 * \param pszNewTag New tag value (string). MUST
 *                  not be NULL. MUST be a valid tag.
 */
srRetVal srSLMGSetTAG(srSLMGObj* pThis, char* pszNewTag);


#endif /* #if (FEATURE_LISTENER == 1) || (FEATURE_MSGAPI == 1) */

#ifdef __cplusplus
};
#endif

#endif
