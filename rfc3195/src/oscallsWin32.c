/**\file oscallsWin32.c
 * \brief OS API calls wrapper for Win32.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-02
 *			Coding begun.
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

#include <windows.h>


/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/* As this is not a "real" object, there won't be any private
 * members in this file.
 */

/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

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
	SYSTEMTIME stTime;
	TIME_ZONE_INFORMATION tzInfo;
	long lBias;

	GetLocalTime(&stTime);
	*year = stTime.wYear;
	*month = stTime.wMonth;
	*day = stTime.wDay;
	*hour = stTime.wHour;
	*minute = stTime.wMinute;
	*second = stTime.wSecond;
	*secfrac = stTime.wMilliseconds;
	*secfracPrecison = 3;

	GetTimeZoneInformation(&tzInfo);
	lBias = tzInfo.Bias;
	if(lBias < 0)
	{
		*pcOffsetMode = '-';
		lBias *= -1;
	}
	else
		*pcOffsetMode = '+';
	*pOffsetHour = lBias / 60;
	*pOffsetMinute = lBias % 60;

	return SR_RET_OK;
}
