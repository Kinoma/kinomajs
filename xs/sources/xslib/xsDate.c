/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/

#include "xsAll.h"

#define	TIME_2032		(1956528000)		/*2032-01-01 00:00:00*/
#define TIME_FourYear	(126230400)			/* 4years */
#define TIME_2100		(4102444800.0)		/*2100-01-01 00:00:00*/
typedef struct sxDateTime {
	c_tm tm;
	int ms;
} txTimeDescription;

txNumber gxDeltaTime;
static txNumber gxMaxTime;
static txNumber gxMinTime;

#define mxCheckDate(THE_SLOT) \
	if (!mxIsDate(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no Date")

static void fx_Date(txMachine* the);
static void fx_Date_now(txMachine* the);
static void fx_Date_parse(txMachine* the);
static txInteger fx_Date_parse_number(txByte* theCharacter, txString* theString);
static void fx_Date_UTC(txMachine* the);
static void fx_Date_get_aux(txMachine* the, txTimeDescription* td);
static void fx_Date_getMilliseconds(txMachine* the);
static void fx_Date_getSeconds(txMachine* the);
static void fx_Date_getMinutes(txMachine* the);
static void fx_Date_getHours(txMachine* the);
static void fx_Date_getDay(txMachine* the);
static void fx_Date_getDate(txMachine* the);
static void fx_Date_getMonth(txMachine* the);
static void fx_Date_getFullYear(txMachine* the);
static void fx_Date_getUTC_aux(txMachine* the, txTimeDescription* td);
static void fx_Date_getUTCMilliseconds(txMachine* the);
static void fx_Date_getUTCSeconds(txMachine* the);
static void fx_Date_getUTCMinutes(txMachine* the);
static void fx_Date_getUTCHours(txMachine* the);
static void fx_Date_getUTCDay(txMachine* the);
static void fx_Date_getUTCDate(txMachine* the);
static void fx_Date_getUTCMonth(txMachine* the);
static void fx_Date_getUTCFullYear(txMachine* the);
static void fx_Date_getTimezoneOffset(txMachine* the);
static void fx_Date_set_aux(txMachine* the, txTimeDescription* td);
static void fx_Date_setMilliseconds(txMachine* the);
static void fx_Date_setSeconds(txMachine* the);
static void fx_Date_setMinutes(txMachine* the);
static void fx_Date_setHours(txMachine* the);
static void fx_Date_setDate(txMachine* the);
static void fx_Date_setMonth(txMachine* the);
static void fx_Date_setFullYear(txMachine* the);
static void fx_Date_setTime(txMachine* the);
static void fx_Date_setUTC_aux(txMachine* the, txTimeDescription* td);
static void fx_Date_setUTCMilliseconds(txMachine* the);
static void fx_Date_setUTCSeconds(txMachine* the);
static void fx_Date_setUTCMinutes(txMachine* the);
static void fx_Date_setUTCHours(txMachine* the);
static void fx_Date_setUTCDate(txMachine* the);
static void fx_Date_setUTCMonth(txMachine* the);
static void fx_Date_setUTCFullYear(txMachine* the);
static void fx_Date_toDateString(txMachine* the);
static void fx_Date_toISOString(txMachine* the);
static void fx_Date_toJSON(txMachine* the);
static void fx_Date_toString(txMachine* the);
static void fx_Date_toTimeString(txMachine* the);
static void fx_Date_toUTCString(txMachine* the);
static void fx_Date_valueOf(txMachine* the);

void fxBuildDate(txMachine* the)
{
	txTimeDescription td;

	mxPush(mxGlobal);
			
	mxPush(mxObjectPrototype);
	fxNewDateInstance(the);
	fxNewHostFunction(the, fx_Date_getMilliseconds, 0);
	fxQueueID(the, fxID(the, "getMilliseconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getSeconds, 0);
	fxQueueID(the, fxID(the, "getSeconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getMinutes, 0);
	fxQueueID(the, fxID(the, "getMinutes"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getHours, 0);
	fxQueueID(the, fxID(the, "getHours"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getDay, 0);
	fxQueueID(the, fxID(the, "getDay"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getDate, 0);
	fxQueueID(the, fxID(the, "getDate"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getMonth, 0);
	fxQueueID(the, fxID(the, "getMonth"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getFullYear, 0);
	fxQueueID(the, fxID(the, "getFullYear"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCMilliseconds, 0);
	fxQueueID(the, fxID(the, "getUTCMilliseconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCSeconds, 0);
	fxQueueID(the, fxID(the, "getUTCSeconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCMinutes, 0);
	fxQueueID(the, fxID(the, "getUTCMinutes"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCHours, 0);
	fxQueueID(the, fxID(the, "getUTCHours"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCDay, 0);
	fxQueueID(the, fxID(the, "getUTCDay"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCDate, 0);
	fxQueueID(the, fxID(the, "getUTCDate"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCMonth, 0);
	fxQueueID(the, fxID(the, "getUTCMonth"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getUTCFullYear, 0);
	fxQueueID(the, fxID(the, "getUTCFullYear"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_valueOf, 0);
	fxQueueID(the, fxID(the, "getTime"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_getTimezoneOffset, 0);
	fxQueueID(the, fxID(the, "getTimezoneOffset"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setMilliseconds, 1);
	fxQueueID(the, fxID(the, "setMilliseconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setSeconds, 1);
	fxQueueID(the, fxID(the, "setSeconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setMinutes, 1);
	fxQueueID(the, fxID(the, "setMinutes"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setHours, 1);
	fxQueueID(the, fxID(the, "setHours"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setDate, 1);
	fxQueueID(the, fxID(the, "setDate"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setMonth, 1);
	fxQueueID(the, fxID(the, "setMonth"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setFullYear, 1);
	fxQueueID(the, fxID(the, "setFullYear"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setTime, 1);
	fxQueueID(the, fxID(the, "setTime"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCMilliseconds, 1);
	fxQueueID(the, fxID(the, "setUTCMilliseconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCSeconds, 1);
	fxQueueID(the, fxID(the, "setUTCSeconds"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCMinutes, 1);
	fxQueueID(the, fxID(the, "setUTCMinutes"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCHours, 1);
	fxQueueID(the, fxID(the, "setUTCHours"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCDate, 1);
	fxQueueID(the, fxID(the, "setUTCDate"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCMonth, 1);
	fxQueueID(the, fxID(the, "setUTCMonth"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_setUTCFullYear, 1);
	fxQueueID(the, fxID(the, "setUTCFullYear"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toDateString, 0);
	fxQueueID(the, fxID(the, "toDateString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toISOString, 0);
	fxQueueID(the, fxID(the, "toISOString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toJSON, 1);
	fxQueueID(the, fxID(the, "toJSON"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toDateString, 0);
	fxQueueID(the, fxID(the, "toLocaleDateString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toTimeString, 0);
	fxQueueID(the, fxID(the, "toLocaleTimeString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toTimeString, 0);
	fxQueueID(the, fxID(the, "toTimeString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_toUTCString, 0);
	fxQueueID(the, fxID(the, "toUTCString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_valueOf, 0);
	fxQueueID(the, fxID(the, "valueOf"), XS_DONT_ENUM_FLAG);
	
	fxAliasInstance(the, the->stack);
	mxDatePrototype = *the->stack;
	fxNewHostConstructor(the, fx_Date, 1);
	fxNewHostFunction(the, fx_Date_now, 0);
	fxQueueID(the, fxID(the, "now"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Date_UTC, 2);
	fxQueueID(the, fxID(the, "UTC"), XS_DONT_ENUM_FLAG);
	//fxAliasInstance(the, the->stack);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxDatePrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Date"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	the->stack++;
			
	c_memset(&td, 0, sizeof(txTimeDescription));
    td.tm.tm_mday = 2;
    td.tm.tm_year = 70;
	td.tm.tm_isdst = -1;
	gxDeltaTime = 1000.0 * ((long)c_mktime(&(td.tm)) - (24L * 3600L));
	gxMaxTime = TIME_2100; //extend from LONG_MAX to this number to support year upto 2099
	gxMinTime = LONG_MIN;
}

txSlot* fxNewDateInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	anInstance = fxNewSlot(the);
	anInstance->next = C_NULL;
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	if (the->stack->kind == XS_ALIAS_KIND)
		anInstance->ID = the->stack->value.alias;
	else
		anInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->next = C_NULL;
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_DATE_KIND;
	aProperty->value.number = 0;
	
	return anInstance;
}

void fx_Date(txMachine* the)
{
	txTimeDescription td;
	c_timeval tv;
	char aBuffer[256];
	c_time_t aTime;
	txNumber aNumber;

	if ((mxThis->kind == XS_REFERENCE_KIND) && ((mxThis->value.reference->flag & XS_SHARED_FLAG) == 0)) {
		txSlot* anInstance = mxThis->value.reference;
        if (mxIsDate(anInstance)) {
            if (mxArgc > 1) {
                td.tm.tm_year = fxToInteger(the, mxArgv(0));
                if ((td.tm.tm_year < 0) || (99 < td.tm.tm_year))
                    td.tm.tm_year -= 1900;
                td.tm.tm_mon = fxToInteger(the, mxArgv(1));
                if (mxArgc > 2)
                    td.tm.tm_mday = fxToInteger(the, mxArgv(2));
                else
                    td.tm.tm_mday = 1;
                if (mxArgc > 3)
                    td.tm.tm_hour = fxToInteger(the, mxArgv(3));
                else
                    td.tm.tm_hour = 0;
                if (mxArgc > 4)
                    td.tm.tm_min = fxToInteger(the, mxArgv(4));
                else
                    td.tm.tm_min = 0;
                if (mxArgc > 5)
                    td.tm.tm_sec = fxToInteger(the, mxArgv(5));
                else
                    td.tm.tm_sec = 0;
                if (mxArgc > 6)
                    td.ms = fxToInteger(the, mxArgv(6));
                else
                    td.ms = 0;
                td.tm.tm_isdst = -1;
                aTime = c_mktime(&(td.tm));	
                if (aTime == -1)
                    aNumber = NAN;
                else
                    aNumber = (aTime * 1000.0) + td.ms;
            }
            else if (mxArgc > 0) {
                aNumber = fxToNumber(the, mxArgv(0));
            }
            else {
                c_gettimeofday(&tv, NULL);
                // According to 15.9.1.1, time precision in JS is milliseconds
                aNumber = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec / 1000));
            }
            anInstance->next->value.number = aNumber;
            *mxResult = *mxThis;
        }
        return;
    }

    {
		c_time_t sec;

		c_gettimeofday(&tv, NULL);
		sec = tv.tv_sec;				// necessary because on Windows tv.tv_sec is a long (32 bits), which is diffrent from time_t (64 bits)
		td.tm = *c_localtime(&sec);
		c_strftime(aBuffer, sizeof(aBuffer), "%a, %d %b %Y %H:%M:%S", &(td.tm));
		fxCopyStringC(the, mxResult, aBuffer);
	}
}

void fx_Date_now(txMachine* the)
{
	c_timeval tv;

	c_gettimeofday(&tv, NULL);
	mxResult->value.number = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0);
	mxResult->kind = XS_NUMBER_KIND;
}

txInteger fx_Date_parse_number(txByte* theCharacter, txString* theString)
{
	txByte c = *theCharacter;
	txString p = *theString;
	txInteger aResult = c - '0';
	c = *p++;
	while (('0' <= c) && (c <= '9')) {
		aResult = (aResult * 10) + c - '0';
		c = *p++;
	}
	*theCharacter = c;
	*theString = p;
	return aResult;
}

void fx_Date_parse(txMachine* the)
{
	#define mxDayCount 7
	static char* gxDays[mxDayCount] = {
		"monday", 
		"tuesday", 
		"wednesday", 
		"thursday", 
		"friday",
		"saturday", 
		"sunday"
	};
	#define mxMonthCount 12
	static char* gxMonths[mxMonthCount] = {
		"january",
		"february",
		"march",
		"april",
		"may",
		"june",
		"july",
		"august",
		"september",
		"october",
		"november",
		"december"
	};
	#define mxZoneCount 11
	static char* gxZones[mxZoneCount] = {
		"gmt", "ut", "utc",
		"est", "edt",
		"cst", "cdt",
		"mst", "mdt",
		"pst", "pdt"
	};
	static int gxDeltas[mxZoneCount] = {
		0, 0, 0,
		-5, -4,
		-6, -5,
		-7, -6,
		-8, -7
	};
	
	txString aString;
	txTimeDescription td;
	txString p;
	txString q;
	txByte c;
	char aBuffer[10];	/* base type should be the same as txString */
	txInteger aComment;
	txInteger aDelta;
	txInteger aValue;
	txInteger aLength;
	txInteger i;
	c_time_t aTime;
	txNumber aNumber;
	txInteger hasSlash = 0;
		
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.parse: no string parameter");
	aString = fxToString(the, mxArgv(0));
	
	td.tm.tm_sec = -1;
	td.tm.tm_min = -1;
	td.tm.tm_hour = -1;
	td.tm.tm_mday = -1;
	td.tm.tm_mon = -1;
	td.tm.tm_year = -1;
	aComment = 0;
	aDelta = -1;
	
	c = *aString++;
	while (c) {
		if (c == '(') {
			aComment++;
			c = *aString++;
			continue;
		}
		else if (c == ')') {
			if (aComment) {
				aComment--;
				c = *aString++;
				continue;
			}
			else
				goto fail;
		}
		else if (aComment) {
			c = *aString++;
			continue;
		}	
		
		if ((c <= ' ') || (c == ',')) {
			c = *aString++;
			continue;
		}
			
		else if ((c == '-') | (c == '+')) {
            txInteger aSign;
			if ((aDelta != 0) && (aDelta != -1))
				goto fail;
			if (c == '-')	
				aSign = -1;
			else
				aSign = 1;
			c = *aString++;
			if (('0' <= c) && (c <= '9')) {
				aValue = fx_Date_parse_number(&c, &aString);
				if (c == ':') {
					aDelta = 60 * aValue;
					c = *aString++;
					if (('0' <= c) && (c <= '9')) {
						aDelta += fx_Date_parse_number(&c, &aString);
					}
				}
				else {
					if (aValue < 24)
						aDelta = aValue * 60;
					else
						aDelta = (aValue % 100) + ((aValue / 100) * 60);
				}
			}
			else
				goto fail;
			aDelta *= aSign;
		}		
		else if (('0' <= c) && (c <= '9')) {
			aValue = fx_Date_parse_number(&c, &aString);
			if (c == ':') {
				if (td.tm.tm_hour >= 0) 
					goto fail;
				td.tm.tm_hour = aValue;	
				c = *aString++;
				if (('0' <= c) && (c <= '9')) {
					td.tm.tm_min = fx_Date_parse_number(&c, &aString);
					if (c == ':') {
						c = *aString++;
						if (('0' <= c) && (c <= '9'))
							td.tm.tm_sec = fx_Date_parse_number(&c, &aString);
						else
							td.tm.tm_sec = 0;
					}
				}
				else
					td.tm.tm_sec = 0;
			}
			else if (c == '/') {
				if (td.tm.tm_year >= 0) 
					goto fail;
				td.tm.tm_year = (aValue < 100) ? aValue : aValue - 1900;
				c = *aString++;
				if (('0' <= c) && (c <= '9')) {
					td.tm.tm_mon = fx_Date_parse_number(&c, &aString) - 1;
					if (c == '/') {
						c = *aString++;
						if (('0' <= c) && (c <= '9')) {
							td.tm.tm_mday = fx_Date_parse_number(&c, &aString);
							hasSlash = 1;
						}
						else
							td.tm.tm_mday = 1;
					}
				}
				else
					td.tm.tm_mon = 0;
			}
			else if (c == '-') {
				if (td.tm.tm_year >= 0) 
					goto fail;
				td.tm.tm_year = (aValue < 100) ? aValue : aValue - 1900;
				c = *aString++;
				if (('0' <= c) && (c <= '9')) {
					td.tm.tm_mon = fx_Date_parse_number(&c, &aString) - 1;
					if (c == '-') {
						c = *aString++;
						if (('0' <= c) && (c <= '9'))
							td.tm.tm_mday = fx_Date_parse_number(&c, &aString);
						else
							td.tm.tm_mday = 1;
					}
				}
				else
					td.tm.tm_mon = 0;
			}
			else {
				if (aValue < 70) {
					if (td.tm.tm_mday < 0)
						td.tm.tm_mday = aValue;
					else
						goto fail;
				}
				else {
					if (td.tm.tm_year < 0)
						td.tm.tm_year = (aValue < 100) ? aValue : aValue - 1900;
					else
						goto fail;
				}
			}
		}				
		else if ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z'))) {
			txInteger	cmpLength;
			p = aBuffer;
			q = p + sizeof(aBuffer) - 1;
			do {
				if (p == q) goto fail;
				*p++ = c_tolower(c);
				c = *aString++;
			} while ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')));
			*p = 0;
			aLength = p - (txString)aBuffer;
			cmpLength = (aLength >=3)? aLength: 3;
			if (c_strcmp("am", aBuffer) == 0) {
				if ((td.tm.tm_hour < 0) || (12 <  td.tm.tm_hour))
					goto fail;
				if (td.tm.tm_hour == 12)
					td.tm.tm_hour = 0;
				continue;
			}
			if (c_strcmp("pm", aBuffer) == 0) {
				if ((td.tm.tm_hour < 0) || (12 <  td.tm.tm_hour))
					goto fail;
				if (td.tm.tm_hour != 12)
					td.tm.tm_hour += 12;
				continue;
			}
			for (i = 0; i < mxDayCount; i++)
				if (c_strncmp(gxDays[i], aBuffer, cmpLength) == 0)
					break;
			if (i < mxDayCount)
				continue;
			for (i = 0; i < mxMonthCount; i++)
				if (c_strncmp(gxMonths[i], aBuffer, cmpLength) == 0)
					break;
			if (i < mxMonthCount) {
				if (td.tm.tm_mon < 0) {
					td.tm.tm_mon = i;
					continue;
				}
				else
					goto fail;
			}
			for (i = 0; i < mxZoneCount; i++)
				if (c_strcmp(gxZones[i], aBuffer) == 0)
					break;
			if (i < mxZoneCount) {
				if (aDelta == -1) {
					aDelta = gxDeltas[i] * 60;
					continue;
				}
				else
					goto fail;
			}
			if (c_strcmp("t", aBuffer) == 0) {
				if (td.tm.tm_year < 0) 
					goto fail;
				continue;
			}
			if (c_strcmp("z", aBuffer) == 0) {
				if (td.tm.tm_hour < 0) 
					goto fail;
				aDelta = 0;
				continue;
			}
			goto fail;
		}
		else
			goto fail;
	}
	if (td.tm.tm_year < 0)
		goto fail;
	if (td.tm.tm_mon < 0)
		goto fail;
	if (td.tm.tm_mday < 0)
		td.tm.tm_mday = 1;
	if (td.tm.tm_hour < 0)
		td.tm.tm_hour = 0;
	if (td.tm.tm_min < 0)
		td.tm.tm_min = 0;
	if (td.tm.tm_sec < 0)
		td.tm.tm_sec = 0;
	if (aDelta != -1)
		td.tm.tm_isdst = 0;
	else
		td.tm.tm_isdst = -1;
	// Check "30/3/1999" format
	if (hasSlash) {
    	if ((td.tm.tm_year < 32) && (td.tm.tm_mday >= 32)) {
        	hasSlash = td.tm.tm_year;
        	td.tm.tm_year = (td.tm.tm_mday < 100) ? td.tm.tm_mday : td.tm.tm_mday - 1900;
        	td.tm.tm_mday = hasSlash;
    	}
	}
	
	aTime = c_mktime(&(td.tm));	
	if (aTime == -1) {
		// Check again for real NaN : 1969-12-31T23:59:59:999 (ecma/Date/15.9.4.2.js)
		td.tm.tm_sec = 0;
		aTime = c_mktime(&(td.tm));
		if(-60 == aTime) {
			aNumber = -1000.0;
			if (aDelta != -1) {
				aNumber -= gxDeltaTime;
				aNumber -= (txNumber)aDelta * 60000.0;
			}
		}
		else
			aNumber = NAN;
	}
	else {
		aNumber = (aTime * 1000.0);
		if (aDelta != -1) {
			aNumber -= gxDeltaTime;
			aNumber -= (txNumber)aDelta * 60000.0;
		}
	}
	mxResult->value.number = aNumber;
	mxResult->kind = XS_NUMBER_KIND;
	return;
fail:
	mxDebug0(the, XS_SYNTAX_ERROR, "Date.parse: invalid parameter");
}

void fx_Date_UTC(txMachine* the)
{
	txTimeDescription td;
	c_time_t aTime;
	txNumber aNumber;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.UTC: no year parameter");
	td.tm.tm_year = fxToInteger(the, mxArgv(0));
	if ((td.tm.tm_year < 0) || (99 < td.tm.tm_year))
		td.tm.tm_year -= 1900;
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.UTC: no month parameter");
	td.tm.tm_mon = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.tm.tm_mday = fxToInteger(the, mxArgv(2));
	else
		td.tm.tm_mday = 1;
	if (mxArgc > 3)
		td.tm.tm_hour = fxToInteger(the, mxArgv(3));
	else
		td.tm.tm_hour = 0;
	if (mxArgc > 4)
		td.tm.tm_min = fxToInteger(the, mxArgv(4));
	else
		td.tm.tm_min = 0;
	if (mxArgc > 5)
		td.tm.tm_sec = fxToInteger(the, mxArgv(5));
	else
		td.tm.tm_sec = 0;
	if (mxArgc > 6)
		td.ms = fxToInteger(the, mxArgv(6));
	else
		td.ms = 0;
	td.tm.tm_isdst = 0;
	aTime = c_mktime(&(td.tm));	
	if (aTime == -1)
		aNumber = NAN;
	else {
		aNumber = (aTime * 1000.0) + td.ms;
		aNumber -= gxDeltaTime;
	}
	mxResult->value.number = aNumber;
	mxResult->kind = XS_NUMBER_KIND;
}

void fx_Date_get_aux(txMachine* the, txTimeDescription* td)
{
	txSlot* aDate;
	txNumber aNumber;
	c_time_t aTime;
	txInteger year = 0;

	aDate = fxGetInstance(the, mxThis);
	mxCheckDate(aDate);
	c_memset(td, 0, sizeof(txTimeDescription));
	td->tm.tm_mday = 1;
	aNumber = aDate->next->value.number;
	if (c_isnan(aNumber)) {
		mxResult->value.number = NAN;
		mxResult->kind = XS_NUMBER_KIND;
		return;
	}
	td->ms = (int)c_floor(c_fmod(aNumber, 1000.0));
	aNumber /= 1000.0;
	if ((aNumber < gxMinTime) || (gxMaxTime < aNumber))
		mxDebug0(the, XS_TYPE_ERROR, "invalid time");

	if(aNumber > TIME_2032) { // 2032 <= aNumber <= 2099
    	year = (int)((aNumber - TIME_2032)/TIME_FourYear);
    	aNumber -= TIME_FourYear * year;
    	year *= 4;
	}
	aTime = (c_time_t)c_floor(aNumber);
	td->tm = *c_localtime(&aTime);
	td->tm.tm_year += year;
	if( (td->ms < 0) && (td->ms > -1000) )
		td->ms = 1000 + td->ms;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Date_getMilliseconds(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.ms;
}

void fx_Date_getSeconds(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_sec;
}

void fx_Date_getMinutes(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_min;
}

void fx_Date_getHours(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_hour;
}

void fx_Date_getDay(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_wday;
}

void fx_Date_getDate(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_mday;
}

void fx_Date_getMonth(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_mon;
}

void fx_Date_getFullYear(txMachine* the)
{
	txTimeDescription td;

	fx_Date_get_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = 1900 + td.tm.tm_year;
}

void fx_Date_getUTC_aux(txMachine* the, txTimeDescription* td)
{
	txSlot* aDate;
	txNumber aNumber;
	c_time_t aTime;
	txInteger year = 0;

	aDate = fxGetInstance(the, mxThis);
	mxCheckDate(aDate);
	c_memset(td, 0, sizeof(txTimeDescription));
	td->tm.tm_mday = 1;
	aNumber = aDate->next->value.number;
	if (c_isnan(aNumber)) {
		mxResult->value.number = NAN;
		mxResult->kind = XS_NUMBER_KIND;
		return;
	}
	td->ms = (int)c_floor(c_fmod(aNumber, 1000.0));
	aNumber /= 1000.0;
	if ((aNumber < gxMinTime) || (gxMaxTime < aNumber))
		mxDebug0(the, XS_TYPE_ERROR, "invalid time");
	
	if(aNumber > TIME_2032) { // 2032 <= aNumber <= 2099
    	year = (int)((aNumber - TIME_2032)/TIME_FourYear);
    	aNumber -= TIME_FourYear * year;
    	year *= 4;
	}
	aTime = (c_time_t)c_floor(aNumber);
	td->tm = *c_gmtime(&aTime);
	td->tm.tm_year += year;
	if( (td->ms < 0) && (td->ms > -1000) )
		td->ms = 1000 + td->ms;
	mxResult->kind = XS_INTEGER_KIND;	
}

void fx_Date_getUTCMilliseconds(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.ms;
}

void fx_Date_getUTCSeconds(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_sec;
}

void fx_Date_getUTCMinutes(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_min;
}

void fx_Date_getUTCHours(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_hour;
}

void fx_Date_getUTCDay(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_wday;
}

void fx_Date_getUTCDate(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = td.tm.tm_mday;
}

void fx_Date_getUTCMonth(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer =  td.tm.tm_mon;
}

void fx_Date_getUTCFullYear(txMachine* the)
{
	txTimeDescription td;

	fx_Date_getUTC_aux(the, &td);
	if (mxResult->kind == XS_INTEGER_KIND)
		mxResult->value.integer = 1900 + td.tm.tm_year;
}

//TODO: 15.9.5.25: if t is NaN, we should return NaN: ecma/Date/15.9.5.22-1.js
void fx_Date_getTimezoneOffset(txMachine* the)
{
	struct timezone tz;
	c_gettimeofday(C_NULL, &tz);
	mxResult->value.integer = /*(tz.tz_dsttime * 60) - */tz.tz_minuteswest;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Date_set_aux(txMachine* the, txTimeDescription* td)
{
	c_time_t aTime;
	txNumber aNumber;
	txSlot* aDate;
	
	td->tm.tm_isdst = -1;
	aTime = c_mktime(&(td->tm));	
	if (aTime == -1)
		aNumber = NAN;
	else
		aNumber = (aTime * 1000.0) + td->ms;
	mxResult->value.number = aNumber;
	mxResult->kind = XS_NUMBER_KIND;
	aDate = fxGetOwnInstance(the, mxThis);
	mxCheckDate(aDate);
	aDate->next->value.number = aNumber;
}

void fx_Date_setMilliseconds(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setMilliseconds: no ms parameter");
	fx_Date_get_aux(the, &td);
	td.ms = fxToInteger(the, mxArgv(0));
	fx_Date_set_aux(the, &td);
}

void fx_Date_setSeconds(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setSeconds: no sec parameter");
	fx_Date_get_aux(the, &td);
	td.tm.tm_sec = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.ms = fxToInteger(the, mxArgv(1));
	fx_Date_set_aux(the, &td);
}

void fx_Date_setMinutes(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setMinutes: no min parameter");
	fx_Date_get_aux(the, &td);
	td.tm.tm_min = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.tm.tm_sec = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.ms = fxToInteger(the, mxArgv(2));
	fx_Date_set_aux(the, &td);
}

void fx_Date_setHours(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setHours: no hour parameter");
	fx_Date_get_aux(the, &td);
	td.tm.tm_hour = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.tm.tm_min = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.tm.tm_sec = fxToInteger(the, mxArgv(2));
	if (mxArgc > 3)
		td.ms = fxToInteger(the, mxArgv(3));
	fx_Date_set_aux(the, &td);
}

void fx_Date_setDate(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setDate: no date parameter");
	fx_Date_get_aux(the, &td);
	td.tm.tm_mday = fxToInteger(the, mxArgv(0));
	fx_Date_set_aux(the, &td);
}

void fx_Date_setMonth(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setMonth: no month parameter");
	fx_Date_get_aux(the, &td);
	td.tm.tm_mon = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.tm.tm_mday = fxToInteger(the, mxArgv(1));
	fx_Date_set_aux(the, &td);
}

void fx_Date_setFullYear(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setYear: no year parameter");
	fx_Date_get_aux(the, &td);
	td.tm.tm_year = fxToInteger(the, mxArgv(0)) - 1900;
	if (mxArgc > 1)
		td.tm.tm_mon = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.tm.tm_mday = fxToInteger(the, mxArgv(2));
	fx_Date_set_aux(the, &td);
}

// According to 15.9.5.27: do TimeClip
void fx_Date_setTime(txMachine* the)
{
	txNumber aNumber;
	txSlot* aDate;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setTime: no time parameter");
	aNumber = fxToNumber(the, mxArgv(0));
	aNumber = c_floor(aNumber);
	mxResult->value.number = aNumber;
	mxResult->kind = XS_NUMBER_KIND;
	aDate = fxGetOwnInstance(the, mxThis);
	mxCheckDate(aDate);
	aDate->next->value.number = aNumber;
}

void fx_Date_setUTC_aux(txMachine* the, txTimeDescription* td)
{
	c_time_t aTime;
	txNumber aNumber;
	txSlot* aDate;
	
	td->tm.tm_isdst = 0;
	aTime = c_mktime(&(td->tm));	
	if (aTime == -1)
		aNumber = NAN;
	else {
		aNumber = (aTime * 1000.0) + td->ms;
		aNumber -= gxDeltaTime;
	}
	mxResult->value.number = aNumber;
	mxResult->kind = XS_NUMBER_KIND;
	aDate = fxGetOwnInstance(the, mxThis);
	mxCheckDate(aDate);
	aDate->next->value.number = aNumber;
}

void fx_Date_setUTCMilliseconds(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCMilliseconds: no ms parameter");
	fx_Date_getUTC_aux(the, &td);
	td.ms = fxToInteger(the, mxArgv(0));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_setUTCSeconds(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCSeconds: no sec parameter");
	fx_Date_getUTC_aux(the, &td);
	td.tm.tm_sec = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.ms = fxToInteger(the, mxArgv(1));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_setUTCMinutes(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCMinutes: no min parameter");
	fx_Date_getUTC_aux(the, &td);
	td.tm.tm_min = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.tm.tm_sec = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.ms = fxToInteger(the, mxArgv(2));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_setUTCHours(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCHours: no hour parameter");
	fx_Date_getUTC_aux(the, &td);
	td.tm.tm_hour = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.tm.tm_min = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.tm.tm_sec = fxToInteger(the, mxArgv(2));
	if (mxArgc > 3)
		td.ms = fxToInteger(the, mxArgv(3));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_setUTCDate(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCDate: no date parameter");
	fx_Date_getUTC_aux(the, &td);
	td.tm.tm_mday = fxToInteger(the, mxArgv(0));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_setUTCMonth(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCMonth: no month parameter");
	fx_Date_getUTC_aux(the, &td);
	td.tm.tm_mon = fxToInteger(the, mxArgv(0));
	if (mxArgc > 1)
		td.tm.tm_mday = fxToInteger(the, mxArgv(1));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_setUTCFullYear(txMachine* the)
{
	txTimeDescription td;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Date.setUTCYear: no year parameter");
	fx_Date_getUTC_aux(the, &td);
	td.tm.tm_year = fxToInteger(the, mxArgv(0)) - 1900;
	if (mxArgc > 1)
		td.tm.tm_mon = fxToInteger(the, mxArgv(1));
	if (mxArgc > 2)
		td.tm.tm_mday = fxToInteger(the, mxArgv(2));
	fx_Date_setUTC_aux(the, &td);
}

void fx_Date_toDateString(txMachine* the)
{
	txTimeDescription td;
	char aBuffer[256];
	
	fx_Date_get_aux(the, &td);
	c_strftime(aBuffer, sizeof(aBuffer), "%Y/%m/%d", &(td.tm));
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Date_toISOString(txMachine* the)
{
	txTimeDescription td;
	char aBuffer[256];
	
	fx_Date_getUTC_aux(the, &td);
	c_strftime(aBuffer, sizeof(aBuffer), "%Y-%m-%dT%H:%M:%SZ", &(td.tm));
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Date_toJSON(txMachine* the)
{
	txTimeDescription td;
	char aBuffer[256];
	
	fx_Date_getUTC_aux(the, &td);
	c_strftime(aBuffer, sizeof(aBuffer), "%Y-%m-%dT%H:%M:%SZ", &(td.tm));
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Date_toString(txMachine* the)
{
	txTimeDescription td;
	char aBuffer[256];
	
	fx_Date_get_aux(the, &td);
	c_strftime(aBuffer, sizeof(aBuffer), "%a, %d %b %Y %H:%M:%S", &(td.tm));
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Date_toTimeString(txMachine* the)
{
	txTimeDescription td;
	char aBuffer[256];
	
	fx_Date_get_aux(the, &td);
	c_strftime(aBuffer, sizeof(aBuffer), "%H:%M:%S", &(td.tm));
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Date_toUTCString(txMachine* the)
{
	txTimeDescription td;
	char aBuffer[256];
	
	fx_Date_getUTC_aux(the, &td);
	c_strftime(aBuffer, sizeof(aBuffer), "%a, %d %b %Y %H:%M:%S GMT", &(td.tm));
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Date_valueOf(txMachine* the)
{
	txSlot* aDate;
	txSlot* aProperty;
	
	aDate = fxGetInstance(the, mxThis);
	mxCheckDate(aDate);
	aProperty = aDate->next;
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value = aProperty->value;
}



