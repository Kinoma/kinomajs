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
#include "mc_stdio.h"
#include "mc_env.h"
#include "mc_time.h"
#include "mc_file.h"
#include "mc_misc.h"
#if mxMC
#include <wm_os.h>
#else
#include <unistd.h>
#endif

#define MC_HOSTNAME_VAR	"HOST"

mc_system_configuration_t mc_conf;

int
mc_gethostname(char *name, size_t len)
{
	const char *hostname;

	if ((hostname = mc_env_get_default(MC_HOSTNAME_VAR)) == NULL) {
#if !mxMC
		if (gethostname(name, len) != 0)
#endif
		return -1;
	}
	else
		strncpy(name, hostname, len);
	if (len != 0)
		name[len - 1] = '\0';
	return 0;
}

int
mc_sethostname(const char *name, size_t len)
{
	const char *hostname;

	if ((hostname = mc_env_get_default(MC_HOSTNAME_VAR)) == NULL || strcmp(hostname, name) != 0) {
		mc_env_set_default(MC_HOSTNAME_VAR, name);
		mc_env_store(NULL);
	}
	return 0;
}

char *
mc_realpath(const char *pathname, char *resolved_path)
{
	MC_FILE *fp;

	if ((fp = mc_fopen(pathname, "r")) == NULL)
		return NULL;
	mc_fclose(fp);
	if (resolved_path == NULL) {
		if ((resolved_path = mc_malloc(strlen(pathname) + 1)) == NULL)
			return NULL;
	}
	strcpy(resolved_path, pathname);
	return resolved_path;
}

char *
mc_strerror(int errnum)
{
	static char num[13];

	snprintf(num, sizeof(num), "%d", errnum);
	return num;
}

void
mc_usleep(unsigned long usec)
{
#if mxMC
	os_thread_sleep(os_msec_to_ticks(usec / 1000));
#else
	usleep(usec);
#endif
}

/*
 * secure RNG based on RC4
 */
static uint8_t mc_rng_state[256];

static void
__rng_init(uint8_t *state, const uint8_t *seed, uint8_t seedsize)
{
	int k;
	uint8_t j, t;

	if (seedsize == 0)
		return;
	for (k = j = 0; k < 256; k++) {
		j += state[k] + seed[k % seedsize];
		t = state[k];
		state[k] = state[j];
		state[j] = t;
	}
}

static void
__rng_process(uint8_t *state, uint8_t *bp, size_t n)
{
	uint8_t i, j, t;
	uint8_t *bufend = bp + n;

	for (i = j = 0; bp < bufend;) {
		++i;
		j += state[i];
		t = state[i];
		state[i] = state[j];
		state[j] = t;
		t = state[i] + state[j];
		*bp++ ^= state[t];
	}
}

void
mc_rng_init(const void *seed, size_t seedsize)
{
	int i;
	static int inited = 0;

	if (!inited) {
		for (i = 0; i < 256; i++)
			mc_rng_state[i] = i;
		inited++;
	}
	__rng_init(mc_rng_state, seed, seedsize);
}

void
mc_rng_gen(uint8_t *bp, size_t n)
{
	memset(bp, 0, n);
	__rng_process(mc_rng_state, bp, n);
}

/* default srng functions */
static uint8_t mc_key[16];

void
mc_srng_init_default(const void *data, size_t sz)
{
	if (sz > sizeof(mc_key))
		sz = sizeof(mc_key);
	memcpy(mc_key, data, sz);
}

static void
mc_srng_process_default(void *buf, size_t bufsize, const uint8_t *seed, size_t seedsize)
{
	uint8_t *state;
	int i;

	if ((state = mc_malloc(256)) == NULL)
		return;
	for (i = 0; i < 256; i++)
		state[i] = i;
	__rng_init(state, seed, seedsize);
	__rng_init(state, mc_key, sizeof(mc_key));
	__rng_process(state, buf, bufsize);
	mc_free(state);
}

void (*mc_srng_init_f)(const void *data, size_t size) = mc_srng_init_default;
void (*mc_srng_process_f)(void *buf, size_t bufsize, const uint8_t *seed, size_t seedsize) = mc_srng_process_default;


/*
 * support functions of the crypt library
 */
#include "crypt.h"

void
crypt_rng(void *buf, size_t sz)
{
	mc_rng_gen(buf, sz);
}


/*
 * base64
 */
static const int8_t b64tab[] = {
	/* start with '+' = 0x2b */
	62, -1, -1, -1, 63,				/* + , - . / */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,		/* 0 1 2 ... */
	-1, -1, -1,					/* : ; < */
	0,						/* = */
	-1, -1, -1,					/* > ? @ */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,			/* A B C ... */
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25,
	-1, -1, -1, -1, -1, -1,				/* [ \ ] ^ _ ` */
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35,		/* a b c ... */
	36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
	46, 47, 48, 49, 50, 51,				/* ... z */
};

size_t
mc_decode64(unsigned char *out, const char *in, size_t sz)
{
	unsigned int c1, c2, c3, c4;
	unsigned char *op = out;
#define C2I(c)	(c >= '+' && c <= 'z' ? b64tab[c-'+'] : 0)
#define OUTC(c)	if (sz-- != 0) *op++ = (c); else break

	while (*in != '\0') {
		c1 = *in != '\0' ? *in++ : 0;
		c2 = *in != '\0' ? *in++ : 0;
		c3 = *in != '\0' ? *in++ : 0;
		c4 = *in != '\0' ? *in++ : 0;
		c1 = C2I(c1);
		c2 = C2I(c2);
		OUTC((c1 << 2) | (c2 >> 4));
		if (c3 != '=') {
			c3 = C2I(c3);
			OUTC((c2 << 4) | (c3 >> 2));
			if (c4 != '=') {
				c4 = C2I(c4);
				OUTC((c3 << 6) | c4);
			}
		}
	}
	return op - out;
}

static const char b64str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t
mc_encode64(char *out, const unsigned char *in, size_t sz)
{
	unsigned long l;
	long n = (long)sz;
	char *op = out;
#define ENC(x)	b64str[(x) & 0x3f]

	while (n > 0) {
		l = --n >= 0 ? *in++ : 0;
		l = (l << 8) | (--n >= 0 ? *in++ : 0);
		l = (l << 8) | (--n >= 0 ? *in++ : 0);
		*op++ = ENC(l >> 18);
		*op++ = ENC(l >> 12);
		*op++ = n < -1 ? '=' : ENC(l >> 6);
		*op++ = n < 0 ? '=' : ENC(l);
	}
	return op - out;
}

#ifdef mxParse

/*
 * strtod
 */

static int
process_exponent(const char **strp)
{
	const char *str = *strp;
	int c;
	int negative = 0;
	int e = 0;

	if (*str == '-') {
		negative++;
		str++;
	}
	else if (*str == '+')
		str++;
	while ((c = *str++) != '\0') {
		if (isdigit(c))
			e += c - '0';
		else
			break;
	}
	*strp = str - 1;
	return negative ? -e : e;
}

double
mc_strtod(const char *str, char **endptr)
{
	int c;
	int negative = 0;
	double num = 0;
	int digits = 0, decimals = -1;
	int e = 0;

	while ((c = *str++) != '\0') {
		if (c == '+')
			;
		else if (c == '-')
			negative++;
		else if (c == '.')
			decimals = digits;
		else if (c == 'e' || c == 'E')
			e = process_exponent(&str);
		else if (isdigit(c)) {
			num = num * 10 + (c - '0');
			digits++;
		}
		else {
			if (digits > 0)
				break;
			/* else ignore */
		}
	}
	if (decimals >= 0)
		e -= digits - decimals;
	if (e >= 0)
		while (--e >= 0)
			num *= 10;
	else
		while (++e <= 0)
			num /= 10;
	if (endptr)
		*endptr = (char *)str - 1;
	return negative ? -num : num;
}

#endif
