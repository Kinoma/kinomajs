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
#ifndef __K5_RINGBUF_H__
#define __K5_RINGBUF_H__

#include <stdint.h>

typedef struct {
	uint32_t size;
	uint32_t head;
	uint32_t tail;
	uint8_t *buffer;
} Ringbuf_t;

/* Static Allocation */
#define RingbufDefine(name, capacity) \
uint8_t ringbuf_buffer_##name[capacity + 1]; \
Ringbuf_t ringbuf_##name = \
{(capacity + 1), (0), (0), (ringbuf_buffer_##name)}
#define RingbufGet(name) \
&ringbuf_##name

/* Macros */
#define RingbufCapacity(rb)						(rb->size - 1)
#define RingbufNextIndex(index, len, size)		((index + len) % size)
#define RingbufNextHead(rb, len)				RingbufNextIndex(rb->head, len, rb->size)
#define RingbufNextTail(rb, len)				RingbufNextIndex(rb->tail, len, rb->size)
#define RingbufIsEmpty(rb)						(rb->head == rb->tail)
#define RingbufIsFull(rb)						(RingbufNextHead(rb, 1) == rb->tail)
#define RingbufAvailable(rb)					((rb->head >= rb->tail) ? \
													(rb->head - rb->tail) : \
													((rb->size - rb->tail) + rb->head))

/* Functions */
int RingBufReadByte(Ringbuf_t *rb);
int RingbufRead(Ringbuf_t *rb, uint8_t *buf, int off, int len);
int RingbufWriteByte(Ringbuf_t *rb, uint8_t b);
int RingbufWrite(Ringbuf_t *rb, uint8_t *buf, int off, int len);

#endif
