/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#include "xs6All.h"

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

static void fx_Date(txMachine* the);
static void fx_Date_now(txMachine* the);
static void fx_Date_parse(txMachine* the);
static txInteger fx_Date_parse_number(txByte* theCharacter, txString* theString);
static txInteger fx_Date_parse_fraction(txByte* theCharacter, txString* theString);
static void fx_Date_UTC(txMachine* the);
static txSlot* fx_Date_prototype_get_aux(txMachine* the, txTimeDescription* td);
static void fx_Date_prototype_getMilliseconds(txMachine* the);
static void fx_Date_prototype_getSeconds(txMachine* the);
static void fx_Date_prototype_getMinutes(txMachine* the);
static void fx_Date_prototype_getHours(txMachine* the);
static void fx_Date_prototype_getDay(txMachine* the);
static void fx_Date_prototype_getDate(txMachine* the);
static void fx_Date_prototype_getMonth(txMachine* the);
static void fx_Date_prototype_getYear(txMachine* the);
static void fx_Date_prototype_getFullYear(txMachine* the);
static txSlot* fx_Date_prototype_getUTC_aux(txMachine* the, txTimeDescription* td);
static void fx_Date_prototype_getUTCMilliseconds(txMachine* the);
static void fx_Date_prototype_getUTCSeconds(txMachine* the);
static void fx_Date_prototype_getUTCMinutes(txMachine* the);
static void fx_Date_prototype_getUTCHours(txMachine* the);
static void fx_Date_prototype_getUTCDay(txMachine* the);
static void fx_Date_prototype_getUTCDate(txMachine* the);
static void fx_Date_prototype_getUTCMonth(txMachine* the);
static void fx_Date_prototype_getUTCFullYear(txMachine* the);
static void fx_Date_prototype_getTimezoneOffset(txMachine* the);
static void fx_Date_prototype_set_aux(txMachine* the, txTimeDescription* td, txSlot* slot);
static void fx_Date_prototype_setMilliseconds(txMachine* the);
static void fx_Date_prototype_setSeconds(txMachine* the);
static void fx_Date_prototype_setMinutes(txMachine* the);
static void fx_Date_prototype_setHours(txMachine* the);
static void fx_Date_prototype_setDate(txMachine* the);
static void fx_Date_prototype_setMonth(txMachine* the);
static void fx_Date_prototype_setYear(txMachine* the);
static void fx_Date_prototype_setFullYear(txMachine* the);
static void fx_Date_prototype_setTime(txMachine* the);
static void fx_Date_prototype_setUTC_aux(txMachine* the, txTimeDescription* td, txSlot* slot);
static void fx_Date_prototype_setUTCMilliseconds(txMachine* the);
static void fx_Date_prototype_setUTCSeconds(txMachine* the);
static void fx_Date_prototype_setUTCMinutes(txMachine* the);
static void fx_Date_prototype_setUTCHours(txMachine* the);
static void fx_Date_prototype_setUTCDate(txMachine* the);
static void fx_Date_prototype_setUTCMonth(txMachine* the);
static void fx_Date_prototype_setUTCFullYear(txMachine* the);
static void fx_Date_prototype_toDateString(txMachine* the);
static void fx_Date_prototype_toISOString(txMachine* the);
static void fx_Date_prototype_toJSON(txMachine* the);
static void fx_Date_prototype_toPrimitive(txMachine* the);
static void fx_Date_prototype_toString(txMachine* the);
static void fx_Date_prototype_toTimeString(txMachine* the);
static void fx_Date_prototype_toUTCString(txMachine* the);
static void fx_Date_prototype_valueOf(txMachine* the);
static txSlot* fxCheckDate(txMachine* the, txSlot* it);

void fxBuildDate(txMachine* the)
{
    static const txHostFunctionBuilder gx_Date_prototype_builders[] = {
		{ fx_Date_prototype_getMilliseconds, 0, _getMilliseconds },
		{ fx_Date_prototype_getSeconds, 0, _getSeconds },
		{ fx_Date_prototype_getMinutes, 0, _getMinutes },
		{ fx_Date_prototype_getHours, 0, _getHours },
		{ fx_Date_prototype_getDay, 0, _getDay },
		{ fx_Date_prototype_getDate, 0, _getDate },
		{ fx_Date_prototype_getMonth, 0, _getMonth },
		{ fx_Date_prototype_getYear, 0, _getYear },
		{ fx_Date_prototype_getFullYear, 0, _getFullYear },
		{ fx_Date_prototype_getUTCMilliseconds, 0, _getUTCMilliseconds },
		{ fx_Date_prototype_getUTCSeconds, 0, _getUTCSeconds },
		{ fx_Date_prototype_getUTCMinutes, 0, _getUTCMinutes },
		{ fx_Date_prototype_getUTCHours, 0, _getUTCHours },
		{ fx_Date_prototype_getUTCDay, 0, _getUTCDay },
		{ fx_Date_prototype_getUTCDate, 0, _getUTCDate },
		{ fx_Date_prototype_getUTCMonth, 0, _getUTCMonth },
		{ fx_Date_prototype_getUTCFullYear, 0, _getUTCFullYear },
		{ fx_Date_prototype_valueOf, 0, _getTime },
		{ fx_Date_prototype_getTimezoneOffset, 0, _getTimezoneOffset },
		{ fx_Date_prototype_setMilliseconds, 1, _setMilliseconds },
		{ fx_Date_prototype_setSeconds, 1, _setSeconds },
		{ fx_Date_prototype_setMinutes, 1, _setMinutes },
		{ fx_Date_prototype_setHours, 1, _setHours },
		{ fx_Date_prototype_setDate, 1, _setDate },
		{ fx_Date_prototype_setMonth, 1, _setMonth },
		{ fx_Date_prototype_setYear, 1, _setYear },
		{ fx_Date_prototype_setFullYear, 1, _setFullYear },
		{ fx_Date_prototype_setTime, 1, _setTime },
		{ fx_Date_prototype_setUTCMilliseconds, 1, _setUTCMilliseconds },
		{ fx_Date_prototype_setUTCSeconds, 1, _setUTCSeconds },
		{ fx_Date_prototype_setUTCMinutes, 1, _setUTCMinutes },
		{ fx_Date_prototype_setUTCHours, 1, _setUTCHours },
		{ fx_Date_prototype_setUTCDate, 1, _setUTCDate },
		{ fx_Date_prototype_setUTCMonth, 1, _setUTCMonth },
		{ fx_Date_prototype_setUTCFullYear, 1, _setUTCFullYear },
		{ fx_Date_prototype_toDateString, 0, _toDateString },
		{ fx_Date_prototype_toUTCString, 0, _toGMTString },
		{ fx_Date_prototype_toISOString, 0, _toISOString },
		{ fx_Date_prototype_toJSON, 1, _toJSON },
		{ fx_Date_prototype_toDateString, 0, _toLocaleDateString },
		{ fx_Date_prototype_toString, 0, _toLocaleString },
		{ fx_Date_prototype_toTimeString, 0, _toLocaleTimeString },
		{ fx_Date_prototype_toPrimitive, 0, _Symbol_toPrimitive },
		{ fx_Date_prototype_toString, 0, _toString },
		{ fx_Date_prototype_toTimeString, 0, _toTimeString },
		{ fx_Date_prototype_toUTCString, 0, _toUTCString },
		{ fx_Date_prototype_valueOf, 0, _valueOf },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Date_builders[] = {
		{ fx_Date_now, 0, _now },
		{ fx_Date_parse, 1, _parse },
		{ fx_Date_UTC, 2, _UTC },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	txTimeDescription td;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewDateInstance(the));
	for (builder = gx_Date_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "Date", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxDatePrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Date, 1, mxID(_Date), XS_GET_ONLY));
	for (builder = gx_Date_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
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
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = fxNextNumberProperty(the, instance, 0, XS_NO_ID, XS_GET_ONLY);
	property->kind = XS_DATE_KIND;
	return instance;
}

void fx_Date(txMachine* the)
{
	txTimeDescription td;
	c_timeval tv;
	char buffer[256];
	c_time_t time;
	txNumber number;

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
                time = c_mktime(&(td.tm));	
                if (time == -1)
                    number = NAN;
                else
                    number = (time * 1000.0) + td.ms;
            }
            else if (mxArgc > 0) {
                number = fxToNumber(the, mxArgv(0));
            }
            else {
                c_gettimeofday(&tv, NULL);
                // According to 15.9.1.1, time precision in JS is milliseconds
                number = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec / 1000));
            }
            anInstance->next->value.number = number;
            *mxResult = *mxThis;
        }
        return;
    }

    {
		c_time_t sec;

		c_gettimeofday(&tv, NULL);
		sec = tv.tv_sec;				// necessary because on Windows tv.tv_sec is a long (32 bits), which is diffrent from time_t (64 bits)
		td.tm = *c_localtime(&sec);
		c_strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S", &(td.tm));
		fxCopyStringC(the, mxResult, buffer);
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

txInteger fx_Date_parse_fraction(txByte* theCharacter, txString* theString)
{
	txByte c = *theCharacter;
	txString p = *theString;
	txNumber fraction = 100;
	txNumber aResult = ((c - '0') * fraction);
	c = *p++;
	while (('0' <= c) && (c <= '9')) {
		fraction /= 10;
		aResult = aResult + ((c - '0') * fraction);
		c = *p++;
	}
	*theCharacter = c;
	*theString = p;
	return (txInteger)c_round(aResult);
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
	char buffer[10];	/* base type should be the same as txString */
	txInteger aComment;
	txInteger aDelta;
	txInteger aValue;
	txInteger aLength;
	txInteger i;
	c_time_t time;
	txNumber number;
	txInteger hasSlash = 0;
		
	if (mxArgc < 1)
		goto fail;
	aString = fxToString(the, mxArgv(0));
	
	td.tm.tm_sec = -1;
	td.tm.tm_min = -1;
	td.tm.tm_hour = -1;
	td.tm.tm_mday = -1;
	td.tm.tm_mon = -1;
	td.tm.tm_year = -1;
	td.ms = 0;
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
						if (('0' <= c) && (c <= '9')) {
							td.tm.tm_sec = fx_Date_parse_number(&c, &aString);
							if (c == '.') {
								c = *aString++;
								if (('0' <= c) && (c <= '9')) {
									td.ms = fx_Date_parse_fraction(&c, &aString);
								}
							}
						}
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
			p = buffer;
			q = p + sizeof(buffer) - 1;
			do {
				if (p == q) goto fail;
				*p++ = c_tolower(c);
				c = *aString++;
			} while ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')));
			*p = 0;
			aLength = p - (txString)buffer;
			cmpLength = (aLength >=3)? aLength: 3;
			if (c_strcmp("am", buffer) == 0) {
				if ((td.tm.tm_hour < 0) || (12 <  td.tm.tm_hour))
					goto fail;
				if (td.tm.tm_hour == 12)
					td.tm.tm_hour = 0;
				continue;
			}
			if (c_strcmp("pm", buffer) == 0) {
				if ((td.tm.tm_hour < 0) || (12 <  td.tm.tm_hour))
					goto fail;
				if (td.tm.tm_hour != 12)
					td.tm.tm_hour += 12;
				continue;
			}
			for (i = 0; i < mxDayCount; i++)
				if (c_strncmp(gxDays[i], buffer, cmpLength) == 0)
					break;
			if (i < mxDayCount)
				continue;
			for (i = 0; i < mxMonthCount; i++)
				if (c_strncmp(gxMonths[i], buffer, cmpLength) == 0)
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
				if (c_strcmp(gxZones[i], buffer) == 0)
					break;
			if (i < mxZoneCount) {
				if (aDelta == -1) {
					aDelta = gxDeltas[i] * 60;
					continue;
				}
				else
					goto fail;
			}
			if (c_strcmp("t", buffer) == 0) {
				if (td.tm.tm_year < 0) 
					goto fail;
				continue;
			}
			if (c_strcmp("z", buffer) == 0) {
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
	
	time = c_mktime(&(td.tm));	
	if (time == -1) {
		// Check again for real NaN : 1969-12-31T23:59:59:999 (ecma/Date/15.9.4.2.js)
		td.tm.tm_sec = 0;
		time = c_mktime(&(td.tm));
		if(-60 == time) {
			number = -1000.0;
			if (aDelta != -1) {
				number -= gxDeltaTime;
				number -= (txNumber)aDelta * 60000.0;
			}
		}
		else
			number = NAN;
	}
	else {
		number = (time * 1000.0);
		if (aDelta != -1) {
			number -= gxDeltaTime;
			number -= (txNumber)aDelta * 60000.0;
		}
	}
	number += td.ms;
	mxResult->value.number = number;
	mxResult->kind = XS_NUMBER_KIND;
	return;
fail:
	mxResult->value.number = C_NAN;
	mxResult->kind = XS_NUMBER_KIND;
	//mxSyntaxError("invalid parameter");
}

void fx_Date_UTC(txMachine* the)
{
	txTimeDescription td;
	c_time_t time;
	txNumber number;

	if (mxArgc < 1)
		mxSyntaxError("no year parameter");
	td.tm.tm_year = fxToInteger(the, mxArgv(0));
	if ((td.tm.tm_year < 0) || (99 < td.tm.tm_year))
		td.tm.tm_year -= 1900;
	if (mxArgc < 2)
		mxSyntaxError("no month parameter");
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
	time = c_mktime(&(td.tm));	
	if (time == -1)
		number = NAN;
	else {
		number = (time * 1000.0) + td.ms;
		number -= gxDeltaTime;
	}
	mxResult->value.number = number;
	mxResult->kind = XS_NUMBER_KIND;
}

txSlot* fx_Date_prototype_get_aux(txMachine* the, txTimeDescription* td)
{
	txSlot* slot = fxCheckDate(the, mxThis);
	txNumber number;
	c_time_t time;
	txInteger year = 0;

	if (!slot) mxTypeError("this is no date");
	number = slot->value.number;
	if (c_isnan(number)) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
		return C_NULL;
	}
	c_memset(td, 0, sizeof(txTimeDescription));
	td->tm.tm_mday = 1;
	td->ms = (int)c_floor(c_fmod(number, 1000.0));
	number /= 1000.0;
	//if ((number < gxMinTime) || (gxMaxTime < number))
	//	mxTypeError("invalid time");

	if(number > TIME_2032) { // 2032 <= number <= 2099
    	year = (int)((number - TIME_2032)/TIME_FourYear);
    	number -= TIME_FourYear * year;
    	year *= 4;
	}
	time = (c_time_t)c_floor(number);
	td->tm = *c_localtime(&time);
	td->tm.tm_year += year;
	if( (td->ms < 0) && (td->ms > -1000) )
		td->ms = 1000 + td->ms;
	mxResult->kind = XS_INTEGER_KIND;
	return slot;
}

void fx_Date_prototype_getMilliseconds(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.ms;
}

void fx_Date_prototype_getSeconds(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_sec;
}

void fx_Date_prototype_getMinutes(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_min;
}

void fx_Date_prototype_getHours(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_hour;
}

void fx_Date_prototype_getDay(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_wday;
}

void fx_Date_prototype_getDate(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_mday;
}

void fx_Date_prototype_getMonth(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_mon;
}

void fx_Date_prototype_getYear(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = td.tm.tm_year;
}

void fx_Date_prototype_getFullYear(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_get_aux(the, &td))
		mxResult->value.integer = 1900 + td.tm.tm_year;
}

txSlot* fx_Date_prototype_getUTC_aux(txMachine* the, txTimeDescription* td)
{
	txSlot* slot = fxCheckDate(the, mxThis);
	txNumber number;
	c_time_t time;
	txInteger year = 0;

	if (!slot) mxTypeError("this is no date");
	number = slot->value.number;
	if (c_isnan(number)) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
		return C_NULL;
	}
	c_memset(td, 0, sizeof(txTimeDescription));
	td->tm.tm_mday = 1;
	td->ms = (int)c_floor(c_fmod(number, 1000.0));
	number /= 1000.0;
	//if ((number < gxMinTime) || (gxMaxTime < number))
	//	mxTypeError("invalid time");
	
	if(number > TIME_2032) { // 2032 <= number <= 2099
    	year = (int)((number - TIME_2032)/TIME_FourYear);
    	number -= TIME_FourYear * year;
    	year *= 4;
	}
	time = (c_time_t)c_floor(number);
	td->tm = *c_gmtime(&time);
	td->tm.tm_year += year;
	if( (td->ms < 0) && (td->ms > -1000) )
		td->ms = 1000 + td->ms;
	mxResult->kind = XS_INTEGER_KIND;	
	return slot;
}

void fx_Date_prototype_getUTCMilliseconds(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = td.ms;
}

void fx_Date_prototype_getUTCSeconds(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = td.tm.tm_sec;
}

void fx_Date_prototype_getUTCMinutes(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = td.tm.tm_min;
}

void fx_Date_prototype_getUTCHours(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = td.tm.tm_hour;
}

void fx_Date_prototype_getUTCDay(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = td.tm.tm_wday;
}

void fx_Date_prototype_getUTCDate(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = td.tm.tm_mday;
}

void fx_Date_prototype_getUTCMonth(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer =  td.tm.tm_mon;
}

void fx_Date_prototype_getUTCFullYear(txMachine* the)
{
	txTimeDescription td;
	if (fx_Date_prototype_getUTC_aux(the, &td))
		mxResult->value.integer = 1900 + td.tm.tm_year;
}

void fx_Date_prototype_getTimezoneOffset(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		struct timezone tz;
		c_gettimeofday(C_NULL, &tz);
		mxResult->value.integer = /*(tz.tz_dsttime * 60) - */tz.tz_minuteswest;
	}
}

void fx_Date_prototype_set_aux(txMachine* the, txTimeDescription* td, txSlot* slot)
{
	c_time_t time;
	txNumber number;
	
	td->tm.tm_isdst = -1;
	time = c_mktime(&(td->tm));	
	if (time == -1)
		number = C_NAN;
	else {
		number = (time * 1000.0) + td->ms;
		if (c_fpclassify(number) != FP_NAN) {
			if (c_fabs(number) > 8.64e15)
				number = C_NAN;
			else
				number = c_trunc(number);
		}
	}
	mxResult->value.number = number;
	mxResult->kind = XS_NUMBER_KIND;
	slot->value.number = number;
}

void fx_Date_prototype_setMilliseconds(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.ms = fxToInteger(the, mxArgv(0));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setSeconds(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_sec = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.ms = fxToInteger(the, mxArgv(1));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setMinutes(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_min = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.tm.tm_sec = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.ms = fxToInteger(the, mxArgv(2));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setHours(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_hour = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.tm.tm_min = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.tm.tm_sec = fxToInteger(the, mxArgv(2));
			if (mxArgc > 3)
				td.ms = fxToInteger(the, mxArgv(3));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setDate(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_mday = fxToInteger(the, mxArgv(0));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setMonth(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_mon = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.tm.tm_mday = fxToInteger(the, mxArgv(1));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setYear(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			txInteger year = fxToInteger(the, mxArgv(0));
			if ((year < 0) || (99 < year))
				year -= 1900;
			td.tm.tm_year = year;
			if (mxArgc > 1)
				td.tm.tm_mon = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.tm.tm_mday = fxToInteger(the, mxArgv(2));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setFullYear(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_year = fxToInteger(the, mxArgv(0)) - 1900;
			if (mxArgc > 1)
				td.tm.tm_mon = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.tm.tm_mday = fxToInteger(the, mxArgv(2));
			fx_Date_prototype_set_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setTime(txMachine* the)
{
	txSlot* slot = fxCheckDate(the, mxThis);
	if (!slot) mxTypeError("this is no date");
	if (mxArgc < 1)
		slot->value.number = C_NAN;
	else {
		txNumber number = fxToNumber(the, mxArgv(0));
		int fpclass = c_fpclassify(number);
		if (fpclass != FP_NAN) {
			if (c_fabs(number) > 8.64e15)
				number = C_NAN;
			else
				number = c_trunc(number);
		}
		slot->value.number = number;
	}
	mxResult->value.number = slot->value.number;
	mxResult->kind = XS_NUMBER_KIND;
}

void fx_Date_prototype_setUTC_aux(txMachine* the, txTimeDescription* td, txSlot* slot)
{
	c_time_t time;
	txNumber number;
	
	td->tm.tm_isdst = 0;
	time = c_mktime(&(td->tm));	
	if (time == -1)
		number = C_NAN;
	else {
		number = (time * 1000.0) + td->ms;
		number -= gxDeltaTime;
	}
	mxResult->value.number = number;
	mxResult->kind = XS_NUMBER_KIND;
	slot->value.number = number;
}

void fx_Date_prototype_setUTCMilliseconds(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.ms = fxToInteger(the, mxArgv(0));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setUTCSeconds(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_sec = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.ms = fxToInteger(the, mxArgv(1));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setUTCMinutes(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_min = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.tm.tm_sec = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.ms = fxToInteger(the, mxArgv(2));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setUTCHours(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_hour = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.tm.tm_min = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.tm.tm_sec = fxToInteger(the, mxArgv(2));
			if (mxArgc > 3)
				td.ms = fxToInteger(the, mxArgv(3));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setUTCDate(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_mday = fxToInteger(the, mxArgv(0));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setUTCMonth(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_mon = fxToInteger(the, mxArgv(0));
			if (mxArgc > 1)
				td.tm.tm_mday = fxToInteger(the, mxArgv(1));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_setUTCFullYear(txMachine* the)
{
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		if (mxArgc < 1)
			slot->value.number = C_NAN;
		else {
			td.tm.tm_year = fxToInteger(the, mxArgv(0)) - 1900;
			if (mxArgc > 1)
				td.tm.tm_mon = fxToInteger(the, mxArgv(1));
			if (mxArgc > 2)
				td.tm.tm_mday = fxToInteger(the, mxArgv(2));
			fx_Date_prototype_setUTC_aux(the, &td, slot);
		}
	}
}

void fx_Date_prototype_toDateString(txMachine* the)
{
	char buffer[256];
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot)
		c_strftime(buffer, sizeof(buffer), "%a %b %d %Y", &(td.tm));
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toISOString(txMachine* the)
{
	char buffer[256]; int c;
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		c_strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &(td.tm));
		c = c_strlen(buffer);
		snprintf(buffer + c, sizeof(buffer) - c, ".%03dZ", td.ms);
	}
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toJSON(txMachine* the)
{
	char buffer[256]; int c;
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot) {
		c_strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &(td.tm));
		c = c_strlen(buffer);
		snprintf(buffer + c, sizeof(buffer) - c, ".%03dZ", td.ms);
	}
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toPrimitive(txMachine* the)
{
	if (mxThis->kind == XS_REFERENCE_KIND) {
		txInteger hint = ((mxArgc > 0) && (c_strcmp(fxToString(the, mxArgv(0)), "number") == 0)) ? XS_NUMBER_HINT : XS_STRING_HINT;
		if (hint == XS_STRING_HINT) {
			mxPushInteger(0);
			mxPushSlot(mxThis);
			fxCallID(the, mxID(_toString));
			if (mxIsReference(the->stack)) {
        		the->stack++;
				mxPushInteger(0);
				mxPushSlot(mxThis);
				fxCallID(the, mxID(_valueOf));
			}
		}
		else {
			mxPushInteger(0);
			mxPushSlot(mxThis);
			fxCallID(the, mxID(_valueOf));
			if (mxIsReference(the->stack)) {
        		the->stack++;
				mxPushInteger(0);
				mxPushSlot(mxThis);
				fxCallID(the, mxID(_toString));
			}
		}
        if (mxIsReference(the->stack)) {
            if (hint == XS_STRING_HINT)
                mxTypeError("Cannot coerce object to string");
            else
                mxTypeError("Cannot coerce object to number");
        }
        mxResult->kind = the->stack->kind;
        mxResult->value = the->stack->value;
        the->stack++;
	}
	else {
		mxResult->kind = mxThis->kind;
		mxResult->value = mxThis->value;
	}
}

void fx_Date_prototype_toString(txMachine* the)
{
	char buffer[256];
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot)
		c_strftime(buffer, sizeof(buffer), "%a %b %d %Y %H:%M:%S GMT%z (%Z)", &(td.tm));
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toTimeString(txMachine* the)
{
	char buffer[256];
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_get_aux(the, &td);
	if (slot)
		c_strftime(buffer, sizeof(buffer), "%H:%M:%S GMT%z (%Z)", &(td.tm));
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toUTCString(txMachine* the)
{
	char buffer[256];
	txTimeDescription td;
	txSlot* slot = fx_Date_prototype_getUTC_aux(the, &td);
	if (slot)
		c_strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &(td.tm));
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckDate(the, mxThis);
	if (!slot) mxTypeError("this is no date");
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value = slot->value;
}

txSlot* fxCheckDate(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_REFERENCE_KIND) {
		it = it->value.reference;
		if (it->flag & XS_VALUE_FLAG) {
			it = it->next;
			if (it->kind == XS_DATE_KIND)
				result = it;
		}
	}
	return result;
}


