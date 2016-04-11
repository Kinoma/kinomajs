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
#ifndef __MC_MISC_H__
#define __MC_MISC_H__

#include "mc_stdio.h"

#define HOST_NAME_MAX	32 /* max length of SSID is 32 */

#define MC_MAX_LED_PINS	3
#define MC_MAX_WAKEUP_BUTTONS	2
typedef struct {
	char *deviceID;		/* NULL - no actual device (i.e. host) */
	int led_pins[MC_MAX_LED_PINS];
	int wakeup_buttons[MC_MAX_WAKEUP_BUTTONS];
	int power_ground_pinmux;
	int usb_console;
} mc_system_configuration_t;
extern mc_system_configuration_t mc_conf;

extern int mc_gethostname(char *name, size_t len);
extern int mc_sethostname(const char *name, size_t len);
extern char *mc_realpath(const char *path, char *resolved);
extern char *mc_strerror(int errnum);
extern void mc_usleep(unsigned long usec);
extern void mc_rng_init(const void *seed, size_t seedsize);
extern void mc_rng_gen(uint8_t *data, size_t size);
extern size_t mc_decode64(unsigned char *out, const char *in, size_t sz);
extern size_t mc_encode64(char *out, const unsigned char *in, size_t n);
#ifdef mxParse
extern double mc_strtod(const char *str, char **endptr);
#endif

extern void (*mc_srng_init_f)(const void *data, size_t size);
#define mc_srng_init(data, size)	(*mc_srng_init_f)(data, size)
extern void (*mc_srng_process_f)(void *buf, size_t bufsize, const uint8_t *seed, size_t seedsize);
#define mc_srng_process(data, size, seed, seedsize)	(*mc_srng_process_f)(data, size, seed, seedsize)

#endif /* __MC_MISC_H__ */
