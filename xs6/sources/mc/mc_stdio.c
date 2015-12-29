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
#include "mc_stdio.h"

#if mxMC

#include <wmstdio.h>

#if CONFIG_LCD_DRIVER
static mdev_t *lcd_dev = NULL;
#endif

void mc_lcd_open()
{
#if CONFIG_LCD_DRIVER
	lcd_drv_init(I2C1_PORT);
	lcd_dev = lcd_drv_open("MDEV_LCD");
#endif
}

void mc_lcd_close()
{
#if CONFIG_LCD_DRIVER
	lcd_drv_close(lcd_dev);
	lcd_dev = NULL;
#endif
}

void mc_lcd_write(const char *line1, const char *line2)
{
#if CONFIG_LCD_DRIVER
#define LCD_LINELEN	16
	char buf1[LCD_LINELEN + 1], buf2[LCD_LINELEN + 1];

	if (lcd_dev == NULL)
		return;
	if (line1 != NULL) {
		int n = strlen(line1);
		if (n > LCD_LINELEN) n = LCD_LINELEN;
		memset(buf1, ' ', 16); buf1[16] = '\0';
		strncpy(buf1, line1, n);
	}
	else
		buf1[0] = '\0';
	if (line2 != NULL) {
		int n = strlen(line2);
		if (n > LCD_LINELEN) n = LCD_LINELEN;
		memset(buf2, ' ', 16); buf2[16] = '\0';
		strncpy(buf2, line2, n);
	}
	else
		buf2[0] = '\0';
	lcd_drv_write(lcd_dev, buf1, buf2);
#endif
}

static int
wmputs(const char *str)
{
	int ret, len;

	len = strlen(str);
	ret = wmprintf("%s", str);
	if (len > 0 && str[len - 1] == '\n' && (len < 2 || str[len - 2] != '\r'))
		wmprintf("\r");
	else if (len > MAX_MSG_LEN)
		wmprintf("\r\n");
	return ret;
}

typedef int (*mc_stdio_puts_t)(const char *str);
static mc_stdio_puts_t mc_stdio_puts_f = NULL;

void
mc_stdio_register(mc_stdio_puts_t f)
{
	mc_stdio_puts_f = f;
}

void
mc_stdio_unregister(mc_stdio_puts_t f)
{
	mc_stdio_puts_f = NULL;
}

static int
mc_stdio_fputs(const char *s, MC_FILE *stream)
{
	int ret;

	if (mc_stdio_puts_f != NULL)
		(*mc_stdio_puts_f)(s);
	if (stream == stdout) {
		int f = mc_log_set_enable(0);
		ret = wmputs(s);
		mc_log_set_enable(f);
	}
	else
		ret = wmputs(s);
	return ret;
}

int
vfprintf(MC_FILE *stream, const char *format, va_list ap)
{
	int ret;
	static char buf[MAX_MSG_LEN];

	/* optimizing for the simple case */
	if (strcmp(format, "%s") == 0) {
		char *s = va_arg(ap, char *);
		return mc_stdio_fputs(s, stream);
	}
	vsnprintf(buf, sizeof(buf), format, ap);
	if (stream == stdlcd) {
		char *p1 = buf, *p2;
		ret = strlen(buf);
		if ((p2 = strchr(p1, '\n')) != NULL)
			*p2++ = '\0';
		mc_lcd_write(p1, p2);
	}
	else
		ret = mc_stdio_fputs(buf, stream);
	return ret;
}

int
fprintf(MC_FILE *stream, const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = vfprintf(stream, format, args);
	va_end(args);
	return ret;
}

void
_exit(int status)
{
	mc_exit(status);
}

#else	/* mxMC */

void
mc_stdio_register(mc_stdio_puts_t f)
{
}

void
mc_stdio_unregister(mc_stdio_puts_t f)
{
}

void
mc_exit(int status)
{
	exit(status);
}

void
mc_fatal(const char *format, ...)
{
	va_list args;
	volatile char junk __attribute__((unused));

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	abort();
}

#endif	/* mxMC */

/*
 * logging
 */
#include "mc_file.h"
#include "mc_env.h"

#define LOG_NLOG	4
#define LOG_FILE	"log"
#define LOG_SIZE	(32*1024)

static MC_FILE *mc_log_fp = NULL;
static int mc_log_enable = 0;

void
mc_log_init()
{
	const char *log = mc_env_get_default("LOG");
	int n;
	char buf[PATH_MAX];

	n = log != NULL ? atoi(log) : -1;
	if (++n >= LOG_NLOG)
		n = 0;
	snprintf(buf, sizeof(buf), "%s/%s%d", mc_get_special_dir("variableDirectory"), LOG_FILE, n);
	if ((mc_log_fp = mc_fopen(buf, "w+")) != NULL) {
		mc_log_enable = 1;
		snprintf(buf, sizeof(buf), "%d", n);
		mc_env_set_default("LOG", buf);
	}
	else
		mc_env_unset(NULL, "LOG");	/* to tell there's been an error */
	mc_env_store(NULL);
}

int
mc_log_set_enable(int f)
{
	int o = mc_log_enable;

	mc_log_enable = f;
	return o;
}

int
mc_log_get_enable()
{
	return mc_log_enable;
}

void
mc_log_write(void *data, size_t n)
{
	if (mc_log_fp != NULL && mc_log_enable) {
		if (mc_fsize(mc_log_fp) + n <= LOG_SIZE) {
			mc_fwrite(data, n, 1, mc_log_fp);
			mc_fflush(mc_log_fp);
		}
	}
}
