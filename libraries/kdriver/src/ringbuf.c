/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

#include <ringbuf.h>

#include <stdint.h>
#include <string.h>

int RingBufReadByte(Ringbuf_t *rb)
{
	int b;
	if (RingbufIsEmpty(rb)) {
		return -1;
	}
	b = rb->buffer[rb->tail];
	rb->tail = RingbufNextTail(rb, 1);
	return b & 0xFF;
}

int RingbufRead(Ringbuf_t *rb, uint8_t *buf, int off, int len)
{
	uint32_t available = RingbufAvailable(rb);
	if (len != 0 && len > available) {
		len = available;
	}

	if (len == 0) {
		return 0;
	}

	if (rb->head > rb->tail) {
		memcpy(buf + off, rb->buffer + rb->tail, len);
	} else {
		uint32_t front = rb->size - rb->tail;
		if (len <= front) {
			memcpy(buf + off, rb->buffer + rb->tail, len);
		} else {
			memcpy(buf + off, rb->buffer + rb->tail, front);
			memcpy(buf + off + front, rb->buffer, len - front);
		}
	}
	rb->tail = RingbufNextTail(rb, len);
	return len;
}

int RingbufWriteByte(Ringbuf_t *rb, uint8_t b)
{
	uint32_t nextHead = RingbufNextHead(rb, 1);
	if (nextHead == rb->tail) {
		return -1;
	}
	rb->buffer[rb->head] = b;
	rb->head = nextHead;
	return 0;
}

int RingbufWrite(Ringbuf_t *rb, uint8_t *buf, int off, int len)
{
	uint32_t free = RingbufCapacity(rb) - RingbufAvailable(rb);
	if (len > free) {
		/* No more space left */
		return -1;
	}

	if (rb->head > rb->tail) {
		uint32_t front = rb->size - rb->head;
		if (len > front) {
			memcpy(rb->buffer + rb->head, buf + off, front);
			memcpy(rb->buffer, buf + off + off, (len - front));
		} else {
			memcpy(rb->buffer + rb->head, buf + off, len);
		}
	} else {
		memcpy(rb->buffer + rb->head, buf + off, len);
	}
	rb->head = RingbufNextHead(rb, len);
	return 0;
}
