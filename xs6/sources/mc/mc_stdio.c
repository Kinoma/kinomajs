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
#define USB_CONSOLE
#define LOG_FILE_ENABLE

#include "mc_stdio.h"
#include "mc_misc.h"
#include "mc_env.h"
#include "mc_ipc.h"

#if mxMC
#include <wmstdio.h>
#ifdef USB_CONSOLE
#include "mc_usb.h"
#endif


#if CONFIG_LCD_DRIVER
static mdev_t *lcd_dev = NULL;

static void
mc_lcd_open()
{
	lcd_drv_init(I2C1_PORT);
	lcd_dev = lcd_drv_open("MDEV_LCD");
}

static void
mc_lcd_close()
{
	lcd_drv_close(lcd_dev);
	lcd_dev = NULL;
}

static void
mc_lcd_write(const char *line1, const char *line2)
{
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
}
#endif	/* CONFIG_LCD_DRIVER */

static stdio_funcs_t *saved_stdio_funcs = NULL;
#define MAX_STDIO_PUTS_F	2
static struct mc_stdio_puts_t {
	mc_stdio_puts_t puts;
	void *closure;
} mc_stdio_puts_f[MAX_STDIO_PUTS_F];

static mc_semaphore_t *mc_stdio_sem = NULL;
static int mc_stdio_busy = 0;

#ifdef USB_CONSOLE
static int usb_enable = 0;
#endif

#define MC_OUTPUT_LOG	0x01
#define MC_OUTPUT_AUX	0x02
#define MC_OUTPUT_CONSOLE	0x04
#define MC_OUTPUT_ALL	0x0f

static int
mc_puts(char *str, unsigned int log, int crnl)
{
	int n = strlen(str);
	int needcr = crnl && n > 0 && str[n - 1] == '\n' && (n < 2 || str[n - 2] != '\r');
	int i;

	if (mc_stdio_busy)
		return n;

	if (mc_stdio_sem)
		mc_semaphore_wait(mc_stdio_sem);
	mc_stdio_busy++;

	/* put the string to the serial console if enable */
	if (saved_stdio_funcs && saved_stdio_funcs->sf_printf) {
		(*saved_stdio_funcs->sf_printf)(str);
		if (needcr)
			(*saved_stdio_funcs->sf_printf)("\r");
	}

	/* then log it to the log file */
	if (log & MC_OUTPUT_LOG)
		mc_log_write(str, n);

	if (log & MC_OUTPUT_AUX) {
		/* and an auxiliary output (e.g. telnet) if set */
		for (i = 0; i < MAX_STDIO_PUTS_F; i++) {
			if (mc_stdio_puts_f[i].puts != NULL)
				(*mc_stdio_puts_f[i].puts)(str, mc_stdio_puts_f[i].closure);
		}
	}

	/* finally put it to the usb console */
#ifdef USB_CONSOLE
	if (usb_enable && (log & MC_OUTPUT_CONSOLE)) {
		if (needcr) {
			if (n > 1)
				if (mc_usb_write(str, n - 1) != 0)
					n = -1;
			mc_usb_write("\r\n", 2);
		}
		else {
			if (mc_usb_write(str, n) != 0)
				n = -1;
		}
	}
#endif
	mc_stdio_busy = 0;
	if (mc_stdio_sem)
		mc_semaphore_post(mc_stdio_sem);
	return n;
}

int
mc_stdio_aux(const char *str)
{
	int i;

	/* and an auxiliary output (e.g. telnet) if set */
	for (i = 0; i < MAX_STDIO_PUTS_F; i++) {
		if (mc_stdio_puts_f[i].puts != NULL)
			(*mc_stdio_puts_f[i].puts)(str, mc_stdio_puts_f[i].closure);
	}
	return strlen(str);
}

static int
mc_stdio_printf(char *str)
{
	return mc_puts(str, MC_OUTPUT_ALL, 0);
}

static int
mc_stdio_flush()
{
	if (saved_stdio_funcs && saved_stdio_funcs->sf_flush)
		(*saved_stdio_funcs->sf_flush)();
	return 0;
}

static int
mc_stdio_getchar(uint8_t *cp)
{
	int ret;

	if (saved_stdio_funcs && saved_stdio_funcs->sf_getchar) {
		if ((*saved_stdio_funcs->sf_getchar)(cp) == 1)
			return 1;
	}
#ifdef USB_CONSOLE
	if (usb_enable) {
		ret = mc_usb_read(cp, 1);
		return ret < 0 ? 0 : ret;
	}
#endif
	return -1;
}

static int
mc_stdio_putchar(char *cp)
{
	char buf[2];

	buf[0] = *cp;
	buf[1] = '\0';
	return mc_puts(buf, MC_OUTPUT_ALL, 0);
}

static stdio_funcs_t mc_stdio_funcs = {
	mc_stdio_printf,
	mc_stdio_flush,
	mc_stdio_getchar,
	mc_stdio_putchar,
};

void
mc_stdio_init()
{
	static mc_semaphore_t sem;

	if (mc_semaphore_create(&sem) == 0) {
		mc_stdio_sem = &sem;
		mc_semaphore_post(mc_stdio_sem);
	}
#if !K5
	wmstdio_init(UART0_ID, 0);
	saved_stdio_funcs = c_stdio_funcs;	/* set by wmstdio_init and it should be "console" */
#endif
#ifdef CONFIG_LCD_DRIVER
	mc_lcd_open();
#endif
#ifdef USB_CONSOLE
	usb_enable = mc_conf.usb_console && mc_usb_init() == 0;
#endif
	c_stdio_funcs = &mc_stdio_funcs;
}

void
mc_stdio_fin()
{
#ifdef USB_CONSOLE
	if (usb_enable)
		mc_usb_fin();
#endif
	if (saved_stdio_funcs != NULL) {
		c_stdio_funcs = saved_stdio_funcs;
		saved_stdio_funcs = NULL;
	}
#ifdef CONFIG_LCD_DRIVER
	mc_lcd_close();
#endif
	if (mc_stdio_sem != NULL) {
		mc_semaphore_delete(mc_stdio_sem);
		mc_stdio_sem = NULL;
	}
}

int
mc_stdio_register(mc_stdio_puts_t f, void *closure)
{
	int i;

	for (i = 0; i < MAX_STDIO_PUTS_F; i++) {
		if (mc_stdio_puts_f[i].puts == NULL) {
			mc_stdio_puts_f[i].puts = f;
			mc_stdio_puts_f[i].closure = closure;
			return i;
		}
	}
	return -1;
}

void
mc_stdio_unregister(int d)
{
	if (d < 0 || d >= MAX_STDIO_PUTS_F)
		return;
	mc_stdio_puts_f[d].puts = NULL;
}

int
vfprintf(MC_FILE *stream, const char *format, va_list ap)
{
	int ret;
	unsigned int flags;
	char buf[MAX_MSG_LEN];

	mc_check_stack();

	flags = MC_OUTPUT_AUX;
	if (stream != stdaux)
		flags |= MC_OUTPUT_CONSOLE;
	if (stream == stderr)
		flags |= MC_OUTPUT_LOG;
	/* optimizing for the simple case */
	if (strcmp(format, "%s") == 0) {
		char *s = va_arg(ap, char *);
		if (flags == MC_OUTPUT_AUX)
			return mc_stdio_aux(s);
		else
			return mc_puts(s, flags, 1);
	}
	vsnprintf(buf, sizeof(buf), format, ap);
#ifdef CONFIG_LCD_DRIVER
	if (stream == stdlcd) {
		char *p1 = buf, *p2;
		ret = strlen(buf);
		if ((p2 = strchr(p1, '\n')) != NULL)
			*p2++ = '\0';
		mc_lcd_write(p1, p2);
	}
	else
#endif
	{
		if (flags == MC_OUTPUT_AUX)
			ret = mc_stdio_aux(buf);
		else
			ret = mc_puts(buf, flags, 1);
	}
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

int
mc_printf(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = vfprintf(stdout, format, args);
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
mc_stdio_init()
{
}

void
mc_stdio_fin()
{
}

int
mc_stdio_register(mc_stdio_puts_t f, void *closure)
{
	return -1;
}

void
mc_stdio_unregister(int d)
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
#if mxMC
#include "mc_time.h"
#else
#include <time.h>
#define mc_time(tp)	time(tp)
#define mc_localtime(tp)	localtime(tp)
#define mc_strftime(b, s, f, t)	strftime(b, s, f, t)
#define mc_tm	tm
#endif

#define LOG_DIR		"/k2/"
#define LOG_FILE	LOG_DIR "log"
#define LOG_BACKUP_FILE	"log.bak"
#define LOG_SIZE	((64*1024) - 1024)
#define LOG_TIME_INTERVAL	60

static MC_FILE *mc_log_fp = NULL;
static time_t mc_log_last = (time_t)0;
static int mc_log_enable = 0;

static int
log_new_file()
{
	if (mc_log_fp != NULL)
		mc_fclose(mc_log_fp);
	mc_unlink(LOG_BACKUP_FILE);
	mc_rename(LOG_FILE, LOG_BACKUP_FILE);
	mc_log_fp = mc_fopen(LOG_FILE, "w+");
	return mc_log_fp != NULL;
}

static size_t
log_write_line(const char *msg, size_t sz)
{
	size_t n;

	if ((n = mc_fwrite(msg, sz, 1, mc_log_fp)) == 0 && errno == ENOSPC) {
		if (!log_new_file())
			return 0;
		return mc_fwrite(msg, sz, 1, mc_log_fp);
	}
	return n;
}

void
mc_log_write(void *data, size_t n)
{
#ifdef LOG_FILE_ENABLE
	time_t t;
	char buf[128];

	mc_check_stack();

	if (!mc_log_enable)
		return;
	if (mc_log_fp == NULL) {
		if ((mc_log_fp = mc_fopen(LOG_FILE, "a+")) == NULL && errno == ENOENT) {
			if ((mc_log_fp = mc_fopen(LOG_FILE, "w+")) == NULL && errno == ENOSPC)
				log_new_file();
		}
		if (mc_log_fp == NULL)
			goto bail;
	}
	if (mc_fsize(mc_log_fp) + n > LOG_SIZE) {
		/* back up the current log file and open a new one */
		if (!log_new_file())
			goto bail;
	}
	t = mc_time(NULL);
	if (mc_log_last == 0) {		/* first time -- print the FW version */
		snprintf(buf, sizeof(buf), "=== FW version: %s ===\n", mc_env_get_default("FW_VER"));
		if (log_write_line(buf, strlen(buf)) == 0)
			goto bail;
	}
	if ((t - mc_log_last) >= LOG_TIME_INTERVAL) {
		struct mc_tm *tm = mc_localtime(&t);
		mc_strftime(buf, sizeof(buf), "=== %a %b %d %Y %H:%M:%S GMT%z (%Z) ===\n", tm);
		if (log_write_line(buf, strlen(buf)) == 0)
			goto bail;
	}
	if (log_write_line(data, n) == 0)
		goto bail;
	mc_fflush(mc_log_fp);
	mc_log_last = t ? t : 1;

bail:;
#endif
}

void
mc_log_set_enable(int enable)
{
	mc_log_enable = enable;
}

int
mc_log_get_enable()
{
	return mc_log_enable;
}
