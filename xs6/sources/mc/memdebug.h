/*! \file memdebug.c
 *  \brief Dynamic memory debugger
 *
 * This code adds dynamic memory guard bytes to find out memory corruption
 * issues. 
 *
 *  Copyright (C) 2011, Marvell International Ltd.
 *  All Rights Reserved.
 */


static const unsigned PREV_MAGIC = 0xCAFED00D;
static const unsigned NEXT_MAGIC = 0x00C0FFEEL;

#define GUARD_BYTES 4

/*
 * Though the guard bytes are only 4 using memcpy and memcpy to avoid
 * alignment issues.
 */
#ifdef DEBUG_HEAP_EXTRA

#define SET_CALLER_ADDR(pxBlock) \
	pxBlock->xCallerAddr = __builtin_return_address(0) - 2;

#define GET_CALLER_ADDR(pxBlock) \
	pxBlock->xCallerAddr

#define SET_ACTUAL_SIZE(pxBlock)	\
	pxBlock->xActualBlockSize = ActualSize

#define GET_ACTUAL_SIZE(pxBlock)		\
	pxBlock->xActualBlockSize

//#define TRACE_GUARDS

#ifdef TRACE_GUARDS
#define GTRACE DTRACE
#else /* ! TRACE_GUARDS */
#define GTRACE(...)
#endif /* TRACE_GUARDS */

#define dump_guard_bytes(addr, context)				\
	{							\
		int i;						\
		unsigned char *caddr = (unsigned char*)addr;	\
		DTRACE("GUARD word : ");				\
		for(i = 0 - context ; i < GUARD_BYTES + (2 * context); i++) \
			DTRACE("%x ", *(caddr + i));		\
		DTRACE("\n\r");				\
	}

#define pre_alloc_hook(xWantedSize)					\
	GTRACE("Size req: %d ", xWantedSize);				\
	/* We need for both next and previous */			\
	xWantedSize += 2 * GUARD_BYTES;					\
	size_t ActualSize = xWantedSize;				\
	GTRACE("Allocated: %d\n\r", ActualSize);

#define post_alloc_hook(ptr)						\
	GTRACE("Adding magic bytes prev @ %x\n\r", ptr);	\
	memcpy(ptr, &PREV_MAGIC, GUARD_BYTES);				\
	void *magic_next_addr = (void*)((unsigned)ptr + ActualSize -	\
					GUARD_BYTES);			\
	GTRACE("Adding magic bytes next @ %x\n\r", magic_next_addr); \
	memcpy(magic_next_addr,	&NEXT_MAGIC, GUARD_BYTES);		\
	ptr = (void*)((unsigned)ptr + GUARD_BYTES)
	
#define pre_free_hook(ptr) {					\
		if(ptr) {					\
			ptr = (void*)((unsigned)ptr - GUARD_BYTES);	\
			GTRACE("PRE_FREE @ %x\n\r", ptr);	\
		}							\
	}

#define post_free_hook(ptr, actual_size)				\
	{								\
		GTRACE("Checking guard bytes @ %x actual_size: %d\n\r", \
			       ptr, actual_size);			\
		if (memcmp((void*)ptr, &PREV_MAGIC, GUARD_BYTES))	{ \
			DTRACE("CORRUPT: Prev memory corrupt @ %x\n\r", ptr); \
			dump_guard_bytes(ptr, 4);				\
		}							\
		void *magic_next_addr = (void*)((unsigned)ptr + actual_size - \
						GUARD_BYTES);		\
		if (memcmp(magic_next_addr, &NEXT_MAGIC, GUARD_BYTES)){	\
			DTRACE("CORRUPT: Next memory corrupt @ %x\n\r", \
				       magic_next_addr); \
			dump_guard_bytes(magic_next_addr, 4);	\
		}					 \
	}

#else /* ! DEBUG_HEAP_EXTRA */


#define SET_CALLER_ADDR(pxBlock)
#define GET_CALLER_ADDR(pxBlock)

#define SET_ACTUAL_SIZE(...)
#define GET_ACTUAL_SIZE(...)

#define pre_alloc_hook(...)
#define post_alloc_hook(...)
#define pre_free_hook(...)
#define post_free_hook(...)

#endif /* DEBUG_HEAP */
