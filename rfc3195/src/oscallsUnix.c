/**\file oscallsUnix.c
 * \brief OS API calls wrapper for Unix.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 *
 * \date    2003-09-02
 *          rgerhards: coding begun
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

#include <sys/time.h>
#include <time.h>

/* ################################################################# */
/* private members                                                   */
/* ################################################################# */

/* As this is not a "real" object, there won't be any private
 * members in this file.
 */

/* ################################################################# */
/* public members                                                    */
/* ################################################################# */

/**
 * Get the current date/time in the best resolution the operating
 * system has to offer (well, actually at most down to the milli-
 * second level.
 *
 * The date and time is returned in separate fields as this is
 * most portable and removes the need for additional structures
 * (but I have to admit it is somewhat "bulky";)).
 *
 * Obviously, all caller-provided pointers must not be NULL...
 */
srRetVal getCurrTime(int* year, int* month, int* day, int *hour, int* minute, int *second,
					 int* secfrac, int *secfracPrecison, char* pcOffsetMode, int* pOffsetHour, 
					 int* pOffsetMinute)
{
	struct timeval tp;
	struct tm *tm;
	long lBias;

	gettimeofday(&tp, NULL);
	tm = localtime(&(tp.tv_sec));
	*year = tm->tm_year + 1900;	*month = tm->tm_mon + 1;	*day = tm->tm_mday;	*hour = tm->tm_hour;	*minute = tm->tm_min;	*second = tm->tm_sec;	*secfrac = tp.tv_usec;	*secfracPrecison = 6;
#ifdef SROS_Solaris 
    /* Solaris uses a different method of exporting the time zone.
     * It is UTC - localtime, which is opposite sign of min east of GMT.
     */
    lBias = -(daylight ? altzone : timezone);
#else
	lBias = tm->tm_gmtoff;#endif
	if(lBias < 0)	{		*pcOffsetMode = '-';		lBias *= -1;	}	else		*pcOffsetMode = '+';	*pOffsetHour = lBias / 3600;	*pOffsetMinute = lBias % 3600;
	return SR_RET_OK;}
