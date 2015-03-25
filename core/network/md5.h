#ifndef __FSK_MD5__
#define __FSK_MD5__

#include "Fsk.h"

#ifndef ulong64
#define ulong64 unsigned long long	// FskUInt64
#endif
#ifndef ulong32
#define ulong32 UInt32
#endif

typedef struct md5_hash_state {
	struct md5_state {
		ulong64 length;
		ulong32 state[4], curlen;
		unsigned char buf[64];
	} md5;
} md5_hash_state;

#ifdef __cplusplus
extern "C" {
#endif

FskAPI(void) md5_init(md5_hash_state * md);
FskAPI(void) md5_process (md5_hash_state *md, const unsigned char *in, unsigned long inlen);
FskAPI(void) md5_done(md5_hash_state *md, unsigned char *out);

#ifdef __cplusplus
}
#endif

#endif
