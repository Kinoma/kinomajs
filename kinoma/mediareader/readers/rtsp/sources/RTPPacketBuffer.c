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
#include "RTPPacketBuffer.h"
#include "FskInstrumentation.h"

FskInstrumentedSimpleType(RTPPacketBuffer, rtppacketbuffer);

long UInt16Diff( long aa, long bb )
{
	long a = (long)aa;
	long b = (long)bb;
	long diff = a - b;

	if( diff < -32767 )
		diff += 65536;
	else if( diff > 32767 )
		diff -= 65536;

	return diff;
}


FskErr RTPPacketBufferNew( RTPPacketParser parser, RTPPacketBuffer **bufferOut, long total )
{
	RTPPacketBuffer *buffer; 
	FskErr err = kFskErrNone;
	
	err = FskMemPtrNewClear(sizeof(RTPPacketBuffer), &buffer);
	if (0 != err) 
		goto bail;
	
	buffer->total = total;
	buffer->lastSequenceNumber =-1;
	buffer->parser = parser;

	err = FskMemPtrNewClear(sizeof(RTPPacketBufferPacket)*total, (FskMemPtr*)&buffer->packets);
	if (0 != err) 
		goto bail;

bail:
	*bufferOut = buffer;

	return err;
}

FskErr RTPPacketBufferDispose(RTPPacketBuffer *buffer )
{
	FskErr err = 0;
	SInt32 i;
	
	if( buffer == NULL )
		return err;

	if( buffer->packets == NULL )
		return err;

	for( i = 0; i < buffer->total; i++ )
	{
		RTPPacketBufferPacket *thisPacket = &buffer->packets[i];

		if( thisPacket->isValid )
			RTPPacketDispose(buffer->parser, thisPacket->rtpPacket, true);
	}
	
	FskMemPtrDispose(buffer->packets);
	FskMemPtrDispose(buffer);

	return err;
}


FskErr RTPPacketBufferPullPacket( RTPPacketBuffer *buffer, long forceOut, RTPPacket *packetOut )
{
	long		i;
	long		validTotal = 0;
	RTPPacketBufferPacket		*earliestPacket   = NULL;
	FskErr		err = 0;
	
	*packetOut = NULL;

	if( buffer == NULL )
		return err;

	if( buffer->packets == NULL || buffer->total < 1 )
		return err;
	
	for( i = 0; i < buffer->total; i++ )
	{
		RTPPacketBufferPacket *thisPacket = &buffer->packets[i];
		long	dif = 0;

		if( !thisPacket->isValid )
			continue;

		validTotal++;
		if( NULL == earliestPacket )
			earliestPacket = thisPacket;
		
		dif = UInt16Diff(thisPacket->rtpPacket->sequenceNumber, earliestPacket->rtpPacket->sequenceNumber );
		if( dif < 0 )
			earliestPacket = thisPacket;
	}

	if( earliestPacket != NULL )
	{
		long dif = UInt16Diff( earliestPacket->rtpPacket->sequenceNumber, buffer->lastSequenceNumber );
		
		if( dif == 1 || dif == 0 || validTotal == buffer->total || forceOut )
		{
			*packetOut = earliestPacket->rtpPacket;
			earliestPacket->isValid = 0;  //release it
			FskInstrumentedTypePrintfDebug(&gRTPPacketBufferTypeInstrumentation, "Pulled:: out, seq#:%d, last seq#: %d", earliestPacket->rtpPacket->sequenceNumber, buffer->lastSequenceNumber);
			buffer->lastSequenceNumber = earliestPacket->rtpPacket->sequenceNumber;
		}
		else
		{
			FskInstrumentedTypePrintfDebug(&gRTPPacketBufferTypeInstrumentation, "Pull:: wait, seq#:%d, last seq#: %d", earliestPacket->rtpPacket->sequenceNumber, buffer->lastSequenceNumber);
		}

		if( dif != 1 && dif != 0 )
			FskInstrumentedTypePrintfDebug(&gRTPPacketBufferTypeInstrumentation, "Pull:: *******************************************gap found, hold************************");

		if( buffer->lastSequenceNumber == -1 )
			buffer->lastSequenceNumber = earliestPacket->rtpPacket->sequenceNumber;
	}
	else
	{
		FskInstrumentedTypePrintfDebug(&gRTPPacketBufferTypeInstrumentation, "Pull:: no frame to pull out");
	}

	return err;
}

FskErr RTPPacketBufferFlush( RTPPacketBuffer *buffer )
{
	long   i;
	FskErr err = 0;
	
	for( i = 0; i < buffer->total; i++ )
	{
		RTPPacketBufferPacket    *thisPacket       = &buffer->packets[i];

		if( thisPacket->isValid )
			RTPPacketDispose(buffer->parser, thisPacket->rtpPacket, true);
			
		thisPacket->isValid = 0;
	}
	
	return err;
}

FskErr RTPPacketBufferPushPacket( RTPPacketBuffer *buffer, RTPPacket packetIn )
{
	long   i;
	long   succeed = 0;
	FskErr err = 0;
	
	FskInstrumentedTypePrintfDebug(&gRTPPacketBufferTypeInstrumentation, "Push:: seq#:%d, last#: %d", packetIn->sequenceNumber, buffer->lastSequenceNumber);
	if( (SInt32)packetIn->sequenceNumber < buffer->lastSequenceNumber && buffer->lastSequenceNumber != -1 )
	{
		FskInstrumentedTypePrintfDebug(&gRTPPacketBufferTypeInstrumentation, "Push:: TOOOOOOOOOOO LAAAAAATE !!!!! seq#:%d#################################", packetIn->sequenceNumber);
	}

	for( i = 0; i < buffer->total; i++ )
	{
		RTPPacketBufferPacket    *thisPacket = &buffer->packets[i];

		if( thisPacket->isValid )
			continue;

		thisPacket->rtpPacket = packetIn;
		thisPacket->isValid = 1;
		succeed = 1;
		break;
	}

	if( succeed == 0 )
		err = -1;

	return err;
}






