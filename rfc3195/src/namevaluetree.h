/*! \file namevaluetree.h
 *  \brief The NameValueTree helper object (includes XML parser).
 *
 * The NameValueTree is a very important utility class in this library.
 * It is used whenever a dynamic linked list is required. This actually
 * not just a list but a tree, that is each element not only has neighbors but
 * also childs. This tree-like structure allows the list class to take
 * care of XML constructs. There is a method to convert a list into
 * BEEP XML format and vice versa.
 *
 * This list object is utilized more or less whenever there is need
 * for a dynamically assigned structure. Some examples are
 * 
 * - profile registry (active profiles)
 * - BEEP XML processing
 * - MIME headers
 *
 * As such, the class should be highly optimized. The list is only single-linked
 * as - as of now - we do not see any reason to walk the list
 * backwards or delete a list entry in the middle. Using single links
 * saves some storage as well as CPU cycles in this case.
 *
 * The list actually consists of two data structures:
 *
 * - the list root (one per tree)
 * - the list entries (multiple per list). 
 *
 * The list provides keyed access. A list can be searched based on
 * a string or unsigned key. Each entry provides string or unsigned
 * values to this key. Each entry can have on void* pointer to point
 * to whatever larger data type the caller may need.
 *
 * If XML is represented, a true tree is build. The hierarchie
 * reflects the nesting of XML elements.
 * 
 * - A XML element with subordinates does not have a value but
 *   a pointer to the root of the subordinate.
 * - A XML end node does not have a pointer to a subordinate.
 *   Instead, it has a pointer to its character representation
 *   of the XML value (that pointer may be NULL if there was no
 *   data in the XML container).
 * - If an XML element has attributes, these attributes are
 *   stored in a separate NameValueTree. However, this "tree"
 *   is not a real tree but always a "flat" list.
 * - If there is CDATA present with a given element, that
 *   CDATA is stored in a special CDATA pointer.
 *
 * The same rules are used to persists a NameValueTree to XML.
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
#ifndef __LIB3195_SBNVT_H_INCLUDED__
#define __LIB3195_SBNVT_H_INCLUDED__ 1
#include "beepsession.h"

#define sbNVTRCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbNVTR);}
#define sbNVTECHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbNVTE);}

/** 
 * The list root object.
 */
struct sbNVTEObject;
struct sbStrBObject;
struct sbNVTRObject
{	
	srObjID OID;					/**< object ID */
	struct sbNVTEObject *pFirst;	/**< first element in this list */
	struct sbNVTEObject *pLast;		/**< last element in this list */
	struct sbNVTEObject *pParent;	/**< parent of this element */
};
typedef struct sbNVTRObject sbNVTRObj;


/** 
 * The list entry object.
 *
 * Please note that keys and values are optional. There are
 * different types provided in case a caller has need for
 * different types. It is up the caller to using them.
 * All pointers are NULL if not used (and all may be NULL!).
 */
struct sbNVTEObject
{	
	srObjID OID;					/**< object ID */
	struct sbNVTEObject *pNext;		/**< next element in list */
	sbNVTRObj *pChild;				/**< pointer to child list */
	sbNVTRObj *pXMLProps;			/**< pointer to XML properties, if any */
	void *pUsr;						/**< user supplied-pointer for user use */
	void (*pUsrDestroy)(void*);		/**< pointer to user-supplied destructor for pUsr */
	char *pszKey;					/**< entry key as character */
	unsigned uKey;					/**< entry integer Key (for faster searches with integer values */
	int uKeyPresent;				/**< boolean - is uKey presen? (we can't tell from the value...) */
	char *pszValue;					/**< associated character value */
	unsigned uValue;				/**< associated unsigned value */
	int	bIsSetUValue;				/**< TRUE = uValue is set (valid), FALSE otherwise */
	char *pCDATA;					/**< associated CDATA */
};
typedef struct sbNVTEObject sbNVTEObj;


/**
 * Destructor for root elements - destroys a complete list.
 *
 * We walk the list from to to bottom and destroy every single
 * element. If an entry element contains other lists, they call
 * recursively back to this method to distroy these lists.
 */
void sbNVTRDestroy(sbNVTRObj* pThis);

/**
 * Destructor for entry elements.
 *
 * This destructor destroys all elements directly
 * associated with this entry element. As such,
 * XML Properties and Childs are destroyed, but not
 * the parent or sibling, as they may remain valid
 * in another context.
 * 
 * If there is a user pointer, the supplied free()
 * function is called on it.
 *
 */
void sbNVTEDestroy(sbNVTEObj* pThis);

/**
 * Construct a NameValueTree Root Object.
 * Use this constructor whenever a new object is created!
 *
 * \retval pointer to the new object or NULL, if
 *         an error occured.
 */
 sbNVTRObj* sbNVTRConstruct(void);

  /**
 * Construct a NameValueTree Entry Object.
 * Use this constructor whenever a new object is created!
 *
 * \retval pointer to the new object or NULL, if
 *         an error occured.
 */
 sbNVTEObj* sbNVTEConstruct(void);

	  /**
  * Unlink the first element from a name value tree.
  * The element is not destroyed, but simply unlinked.
  * \note It is the caller's responsibility to unlink the
  *	      element when he is done!
  *
  * It is valid to call this mothod on a linked list
  * WITHOUT any elements. In this case, NULL is returned.
  *
  * \retval pointer to unlinked element or NULL if list
  *         was empty.
  */
 sbNVTEObj* sbNVTUnlinkElement(sbNVTRObj* pRoot);

 /**
  * Find the next element with a given string key inside the
  * list.
  *
  * \note A list may contain multiple entries with the same key.
  * As such, this method receives a pointer to the last 
  * entry found and continues search from there. Call the method
  * until NULL is returned, in which case no further match is
  * found.
  *
  * \param pRoot Root of the list to be searched.
  * \param pStart Starting entry for the search. If NULL, search
  *        will begin at the first entry. Please note that
  *        pStart MUST belong to pRoot, otherwise results are
  *        unpredictable.
  * \param pszSearch String to search for. If this pointer
  *        is NULL, the next element is returned without
  *        any comparision. In short, with this parameter
  *        being NULL, the method can be used to walk the list.
  * \retval pointer to found element or NULL, if no (more)
  *         element is found.
  */
sbNVTEObj* sbNVTSearchKeySZ(sbNVTRObj* pRoot, sbNVTEObj* pStart, char* pszSearch);

 /**
  * Find the next element with a given integer key inside the
  * list.
  *
  * \note A list may contain multiple entries with the same key.
  * As such, this method receives a pointer to the last 
  * entry found and continues search from there. Call the method
  * until NULL is returned, in which case no further match is
  * found.
  *
  * \param pRoot Root of the list to be searched.
  * \param pStart Starting entry for the search. If NULL, search
  *        will begin at the first entry. Please note that
  *        pStart MUST belong to pRoot, otherwise results are
  *        unpredictable.
  * \param uSearch Unsinged Integer to search for.
  * \retval pointer to found element or NULL, if no (more)
  *         element is found.
  */
sbNVTEObj* sbNVTSearchKeyU(sbNVTRObj* pRoot, sbNVTEObj* pStart, unsigned uSearch);


/**
 * Set the string key for a given entry.
 * The string key is copied to a new buffer if
 * so requested.
 *
 * \param pszKey Pointer to string making up the key.
 * \param bCopy TRUE duplicate string, FALSE = use
 *              supplied buffer. In case of FALSE, the
 *              caller should no longer work with the 
 *              supplied buffer as ownership moves to the
 *              name value tree. Results would be 
 *              unpredictable at best...
 */
srRetVal sbNVTESetKeySZ(sbNVTEObj* pThis, char* pszKey, int bCopy);

/**
 * Set the string value for a given entry.
 * The string key is copied to a new buffer if so
 * requested.
 *
 * \param pszVal New string to be set.
 * \param bCopy TRUE duplicate string, FALSE = use
 *              supplied buffer. In case of FALSE, the
 *              caller should no longer work with the 
 *              supplied buffer as ownership moves to the
 *              name value tree. Results would be 
 *              unpredictable at best...
 */
srRetVal sbNVTESetValueSZ(sbNVTEObj* pThis, char* pszVal, int bCopy);

/**
 * Set the integer key for a given entry.
 * \param uKey new key
 */
srRetVal sbNVTESetKeyU(sbNVTEObj* pThis, unsigned uKey);

/**
 * Remove the integer key for a given entry.
 */
srRetVal sbNVTEUnsetKeyU(sbNVTEObj* pThis);

/**
 * Set the integer value for a given entry.
 * \param uKey new value
 */
srRetVal sbNVTESetValueU(sbNVTEObj* pThis, unsigned uVal);


/**
 * Set the user pointer for a given entry.
 * \param pPtr ponter to user supplied buffer
 * \param pPtrDestroy pointer to user-supplied method to destroy
 *        the buffer. This is necessary as the NameValueTree module
 *        may destroy the buffer at any time without the user actually
 *        knowing. As such, it must have a destructor. To avaoid
 *        coding errors and "too quick" hacks, this pointer is not
 *        allowed to be NULL. If you do not need any destructor, 
 *        provide a pointer to an empty function in this case.
 *        The prototype is void Destruct(void* ptr) with ptr
 *        being the pinter supplied in pPtr.
 */
srRetVal sbNVTESetUsrPtr(sbNVTEObj* pThis, void *pPtr, void(pPtrDestroy)(void*));


/**
 * Unlinks a root element (complete list) from its parent.
 * If the root element has no parent, nothing happens. This
 * is no error.
 */
void sbNVTRUnlinkFromParent(sbNVTRObj *pRoot);

/**
  * Add an entry to the end of a NameValueTree List.
  * A new entry element is constructed and added onto the
  * end of the list.
  *
  * \param pRoot Root of the list the element is to be
  *              added to.
  * \retval pointer to newly created list element or
  *         NULL if error occured.
  */
 sbNVTEObj* sbNVTAddEntry(sbNVTRObj* pRoot);


/**
 * Remove the user point from an element. This is needed
 * if the element got deleted by the user itself. If not done,
 * the list destructor would try to delete the object, thus
 * resulting in a double-free.
 *
 * If the user pointer is not set, nothing happens.
 */
void sbNVTEUnsetUsrPtr(sbNVTEObj *pEntry);

/**
 * Remove a keyed entry from a root element. The key
 * search is done on the unsigned key. The first entry
 * with a matching key value is removed. Do not use
 * this method on a list with potentially multiple
 * keys of the same value - or be sure to know EXACTLY
 * what you are doing and why...
 *
 * The entry shall not only be unlinked from the tree
 * but also destroyed.
 */
srRetVal sbNVTRRemoveKeyU(sbNVTRObj *pRoot, unsigned uKey);


/**
 * Checks if a list has the named element. Can also
 * check if it is the only element in the list.
 *
 * \param pEltname Name of the element to look for (e.g. "ok")
 * \param bMustBeOnly TRUE = must be the only element, FALSE, can
 *        be one of many elements. Please note that the only
 *        element acutally means the only in this whole list (not
 *        the only of this name).
 * \retval if the element could be found, a pointer to it
 *         is returned. NULL is returned if it is not found OR
 *         it is not the only entry and bMustBeOnly is TRUE.
 */
sbNVTEObj *sbNVTRHasElement(sbNVTRObj* pRoot, char *pEltname, int bMustBeOnly);


/**
  * Link a child list to the current entry.
  *
  * \param pEntry Pointer to the entry where the child is to be added.
  * \param pChildRoot Pointer to the to-be-added list.
  */
 srRetVal sbNVTESetChild(sbNVTEObj *pEntry, sbNVTRObj *pChildRoot);

 /**
 * Print out a whole tree structure. This is a debug aid
 * and ONLY AVAILABLE IN DEBUG BUILDS!
 *
 * We walk the tree and call ourselfs recursively.
 *
 * \param iLevel - the current level (tree-depth) we are in.
 */
void sbNVTDebugPrintTree(sbNVTRObj *pRoot, int iLevel);

/**
 * Removes the first entry from a list. The entry
 * will be unlinked and then destroyed. It is valid
 * to call this function on an empty list. In this case,
 * nothing will happen.
 */
 srRetVal sbNVTRRemoveFirst(sbNVTRObj* pRoot);

 /**
 * Populate a NameValueTree based on a BEEP XML stream.
 *
 * This is a mini BEEP XML parser. It parses those constructs
 * supported by BEEP XML. It does NOT check the DTD, this needs
 * to be done by the caller.
 *
 * This is a "one stop" method, which means that it will do 
 * anything needed to do the job. The method detects syntax
 * errors, only. If one is detected, an error state is
 * returned. In this case, the status of the provided
 * NameValueTree structure is undefined - probably some
 * elements have been added, others not. It is highly
 * recommended that the caller discards the NameValueTree
 * structure if this method does not return error-free.
 *
 * If a pre-populated NameValueTree is provided, the 
 * elements found in the XML stream are ADDED to it.
 * 
 * This is a single-pass parser.
 *
 * ABNF for our Mini XML Parser:
 *
 * \note This is Pseudo-ABNF, mostly following
 *       http://www.ietf.org/rfc/rfc2234.txt.
 *
 * BEEPXML			= XMLSTREAM
 * XMLSTREAM		= *WHITESPACE *XMLELEMENT
 * XMLELEMENT		= (XMLNODE / XMLCONTAINER / CDATA) *WHITESPACE
 * CDATA			= "<![CDATA[" CDATAVALUE "]]>" ; "CDATA" must be upper case, only.
 *								    ; Been to lazy to write it down in "right" ABNF ;)
 * XMLNODE			= "<" TAGwPARAMS 1*WHITESPACE "/>" ; is it really 1*WHITESPACE or just
 *                        ; *WHITESPACE? In the implementation, we use *WHITESPACE when reading
 * XMLCONTAINER		= "<" TAGwPARAMS ">" (XMLVALUE / XMLSTREAM) "</" *WHITESPACE TAG ">"
 *					  ; begin and end tag must be exactly the same.
 * TAGwPARAMS		= TAG *(1*WHITESPACE PARAM) *WHITESPACE
 * TAG				= XMLNAME
 * PARAM			= XMLNAME ["='" XMLVALUE "'"]
 * XMLNAME			= NON-WHITESPACE NO "/" / "<" / ">" / "=" / ";"
 * XMLVALUE			= *(PRINTABLECHAR / ESCSEQ)
 * CDATAVALUE		= all printable chars except "]"
 * WHITESPACE		= %d09 / %d10 / %d13 / %d32 ; C isspace()
 * PRINTABLECHAR	= all printable chars
 * ESCSEQ			= "&" XMLNAME ";"
 *
 * \param pszXML Pointer to a string containing the XML stream. May be
 *               NULL, in which case no processing takes place.
 */
srRetVal sbNVTRParseXML(sbNVTRObj *pRoot, char *pszXML);

/**
 * Remove an entry with a specific pUsr from a root element. 
 * The first entry with a matching pUsr is removed. Do not use
 * this method on a list with potentially multiple
 * identical pUsrs - or be sure to know EXACTLY
 * what you are doing and why...
 *
 * \param pUsr The pUsr to find and delete. This may
 *             be NULL but keep the above warning about
 *             multiple values in mind!
 *
 * The entry shall not only be unlinked from the tree
 * but also be destroyed.
 */
srRetVal sbNVTRRemovEntryWithpUsr(sbNVTRObj *pRoot, void* pUsr);

/**
 * Remove a keyed entry from a root element. The key
 * search is done on the unsigned key. The first entry
 * with a matching key value is removed. Do not use
 * this method on a list with potentially multiple
 * keys of the same value - or be sure to know EXACTLY
 * what you are doing and why...
 *
 * The entry shall not only be unlinked from the tree
 * but also be destroyed.
 */
srRetVal sbNVTRRemoveKeyU(sbNVTRObj *pRoot, unsigned uKey);

/**
 * Unlink an entry from the current list.
 *
 * \param pRoot pointer to the root of this list
 * \param pEntry pointer to the entry to be unlinked
 * \param pointer to the element immediately before the
 *        to be unlinked entry in the list. May be NULL,
 *        in which case the to be unlinked element is the
 *        first element of the list.
 */
void sbNVTEUnlinkFromList(sbNVTRObj *pRoot, sbNVTEObj* pEntry, sbNVTEObj* pPrev);

/**
 * This method searches for a specific pUsr and returns the
 * entry in question PLUS the previous element. This is done 
 * for performance reasons - it
 * saves us from doubly-linking the list, which would otherwise
 * be required and for sure be overdone in the current state
 * of affairs.
 * 
 * \note A list may contain multiple entries with the same key.
 * As such, this method receives a pointer to the last 
 * entry found and continues search from there. Call the method
 * until NULL is returned, in which case no further match is
 * found.
 *
 * \param pRoot Root of the list to be searched.
 * \param pStart Starting entry for the search. If NULL, search
 *        will begin at the first entry. Please note that
 *        pStart MUST belong to pRoot, otherwise results are
 *        unpredictable.
 * \param pUsr pUsr to search for.
 * \param ppPrevEntry Pointer to a pointer to the previous element.
 *                    This is needed so that that element can be
 *                    updated during unlink operations. Is NULL if
 *                    there is no previous element.
 * \retval pointer to found element or NULL, if no (more)
 *         element is found.
 */
sbNVTEObj* sbNVTSearchpUsrAndPrev(sbNVTRObj* pRoot, sbNVTEObj* pStart, void *pUsr, sbNVTEObj** ppPrevEntry);

/**
 * This is more or less a duplicate of the sbNVTSearchKeyU. The
 * difference is that this method also delivers back the
 * previous entry. This is done for performance reasons - it
 * saves us from doubly-linking the list, which would otherwise
 * be required and for sure be overdone in the current state
 * of affairs.
 * 
 * \paramm AllParams are like sbNVTSearchKeyU plus
 * \param ppPrevEntry Pointer to a pointer to the previous element.
 *                    This is needed so that that element can be
 *                    updated during unlink operations. Is NULL if
 *                    there is no previous element.
 */
sbNVTEObj* sbNVTSearchKeyUAndPrev(sbNVTRObj* pRoot, sbNVTEObj* pStart, unsigned uSearch, sbNVTEObj** ppPrevEntry);

/**
 * Return the uValue for this object.
 * If the uValue is already set, it is returned. If it is
 * not yet set, we look at the szValue and see if we can
 * convert it to an uValue. If that succeeds, we return
 * the converted value. If that does not succeed, -1 is 
 * returned and an error is flagged.
 *
 * \param puValue [out] pointer to an unsinged that should
 *                      receive the uValue. Please note
 *                      that we can not directly return this
 * as a return value because we need to convey an error state
 * which may happen at any time.
 */
srRetVal sbNVTEGetValueU(sbNVTEObj* pThis, unsigned* puValue);

/**
 * XML-Escape a string. The resulting string is suitable for use
 * in #pcdata, that is as a string BETWEEN XML tags (e.g. 
 * <tag>string</tag>. It is NOT suitable to be used inside
 * a tag parameter (e.g. <tag p="string">).
 *
 * \param pszToEscape The string to be escaped. Should not be
 * NULL. If it is NULL, the return value will also be NULL.
 * \retval Pointer to an XML-escaped string or NULL, if no
 * memory for that string could be allocated. IMPORTANT: the 
 * caller MUST free() that buffer once he is done with the
 * string, otherwise a memory leak will be left.
 */
char* sbNVTXMLEscapePCDATA(char *pszToEscape);


/**
 * XML-Escape a string. The resulting string is suitable for use
 * in #pcdata, that is as a string BETWEEN XML tags (e.g. 
 * <tag>string</tag>. It is NOT suitable to be used inside
 * a tag parameter (e.g. <tag p="string">).
 *
 * \param pszToEscape The string to be escaped. Should not be
 * NULL. If it is NULL, the provided string buffer will not be
 * modified.
 *
 * \param pStr Pointer to a sbStrBObj to be filled with
 *             the escaped string.
 */
srRetVal sbNVTXMLEscapePCDATAintoStrB(char *pszToEscape, struct sbStrBObject *pStr);

/**
 * Duplicate a string and return it. This is a general utility, it
 * was just defined first in the context of NVTE.
 *
 * \param pszStrToDup String to be duplicated. MUST not be NULL.
 * \retval duplicated string. Must be free()ed if no longer needed.
 */
char* sbNVTEUtilStrDup(char *pszStrToDup);

#endif
