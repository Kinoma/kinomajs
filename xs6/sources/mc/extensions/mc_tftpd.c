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
#include "mc_event.h"
#include "mc_file.h"
#include "mc_env.h"
#include "mc_xs.h"
#include "mc_tftpd.h"
#include "soft_crc32.h"

#if mxMC
#include "ftfs.h"
#include "firmware_structure.h"
#else
#include "mc_compat.h"
typedef struct {
	uint8_t magic[8];
	uint32_t crc;
	uint32_t obsolete;
	uint32_t backend_version;
	uint8_t version[4];
} FT_HEADER;

struct img_hdr {
	uint32_t magic_str;
	uint32_t magic_sig;
	uint32_t time;
	uint32_t seg_cnt;
	uint32_t entry;
};

struct seg_hdr {
	uint32_t type;
	uint32_t offset;
	uint32_t len;
	uint32_t laddr;
	uint32_t crc;
};

struct wlan_fw_header {
	uint32_t        magic;
	uint32_t        length;
};

#define WLAN_FW_MAGIC          (('W' << 0)|('L' << 8)|('F' << 16)|('W' << 24))
#define PART_MAGIC	0x9e78d963

#define FW_BLK_ANCILLARY	1	/* ignores data to follow */
#define FW_BLK_LOADABLE_SEGMENT	2	/* loads data to follow */
#define FW_BLK_FN_CALL		3	/* calls function at given addr */
#define FW_BLK_POKE_DATA	4	/* pokes 32-bit value to address */

#endif

#include "tftp.h"

typedef struct {
	int s;
	MC_FILE *fp;
	unsigned short blknum;
	unsigned long nbytes;
	int timeout;	/* in msec */
	int retry;
	int retry_count;
	struct mc_timeout *interval;
	unsigned char buf[PKTSIZE];
	size_t pktlen;	/* nbytes in buf */
	enum {TFTP_FILE, TFTP_FTFS, TFTP_BOOT2, TFTP_WIFIFW, TFTP_MCUFW} filetype;
	char path[PATH_MAX];
	xsMachine *the;
	xsSlot this;
} tftp_t;

#define TFTP_TIMEOUT_INTERVAL	1000
#define TFTP_DEFAULT_TIMEOUT	5000

#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

static int
tftp_resolve_special_path(tftp_t *tftp, const char *filename)
{
	char *p;

	if ((p = strrchr(filename, '.')) == NULL)
		p = "";
	if (strcmp(p, ".ftfs") == 0) {
		strcpy(tftp->path, "/ftfs");
		tftp->filetype = TFTP_FTFS;
		return 1;
	}
	else if (strcmp(p, ".bin") == 0) {
		if (strcmp(filename, "boot2.bin") == 0) {
			strcpy(tftp->path, "/boot2");
			tftp->filetype = TFTP_BOOT2;
		}
		else if (strncmp(filename, "xsr6", 4) == 0) {
			strcpy(tftp->path, "/mcufw");
			tftp->filetype = TFTP_MCUFW;
		}
		else {
			strcpy(tftp->path, "/wififw");
			tftp->filetype = TFTP_WIFIFW;
		}
		return 1;
	}
	else
		return 0;
}

static int
tftp_open(tftp_t *tftp, struct sockaddr *from)
{
	int s;
	struct sockaddr_in sin;

	if ((s = lwip_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mc_log_error("tftpd: lwip_socket failed\n");
		return -1;
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (lwip_bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0 ||
	    lwip_connect(s, (struct sockaddr *)from, sizeof(struct sockaddr)) < 0) {
		lwip_close(s);
		mc_log_error("tftpd: lwip_bind/connect failed\n");
		return -1;
	}
	tftp->s = s;
	tftp->blknum = 0;
	tftp->timeout = TFTP_DEFAULT_TIMEOUT;
	return 0;
}

static void
tftp_close(tftp_t *tftp)
{
	xsBeginHost(tftp->the);
	{
		xsIndex id = xsID("onClose");
		if (xsHas(tftp->this, id))
			xsCall_noResult(tftp->this, id, NULL);
		else {
			xsSetHostData(tftp->this, NULL);
			tftpd_close(tftp);
		}
	}
	xsEndHost(tftp->the);
}

static int
tftp_write(tftp_t *tftp, void *buf, int sz)
{
	return lwip_send(tftp->s, buf, sz, 0);
}

static int
tftp_ack(tftp_t *tftp)
{
	struct tftphdr ack;

	// mc_log_debug("tftpd: sending ack: %d\n", tb->blknum);
	ack.th_opcode = ntohs(ACK);
	ack.th_block = ntohs(tftp->blknum);
	return tftp_write(tftp, &ack, 4) == 4;
}

static void
tftp_error(tftp_t *tftp, int code)
{
	struct tftphdr err;

	err.th_opcode = ntohs(ERROR);
	err.th_code = code;	/* not the network order? */
	err.th_data[0] = '\0';
	(void)tftp_write(tftp, &err, sizeof(err));
	tftp_close(tftp);
}

static int
check_crc_ftfs(tftp_t *tftp)
{
	MC_FILE *fp;
	FT_HEADER fth;
	size_t n, ttl = tftp->nbytes;
	unsigned char *buf = tftp->buf;	/* reuse the packet buffer */
	size_t bufsiz = PKTSIZE;
	uint32_t crc = 0;

	mc_soft_crc32_init();
	if ((fp = mc_fopen(tftp->path, "r")) == NULL) {
		mc_soft_crc32_fin();
		return 0;
	}
	mc_fread(&fth, sizeof(fth), 1, fp);
	ttl -= sizeof(fth);
	while ((n = mc_fread(buf, 1, MIN(ttl, bufsiz), fp)) > 0) {
		crc = mc_soft_crc32(buf, n, crc);
		ttl -= n;
	}
	mc_fclose(fp);
	mc_soft_crc32_fin();
	return crc == fth.crc;
}

static uint32_t
tftp_calc_crc(MC_FILE *fp, uint8_t *buf, size_t bufsiz, long offset, size_t sz)
{
	uint32_t crc = 0;
	size_t n;

	mc_fseek(fp, offset, SEEK_SET);
	while (sz > 0 && (n = mc_fread(buf, 1, MIN(sz, bufsiz), fp)) > 0) {
		crc = mc_soft_crc32(buf, n, crc);
		sz -= n;
	}
	return crc;
}

static int
check_crc_bin(tftp_t *tftp)
{
	MC_FILE *fp;
	uint32_t crc = 0;
	struct img_hdr ih;
	struct seg_hdr sh;
	uint32_t i;
	int res = 1;

	mc_soft_crc32_init();
	if ((fp = mc_fopen(tftp->path, "r")) == NULL) {
		mc_soft_crc32_fin();
		return 0;
	}
	if (tftp->filetype == TFTP_WIFIFW) {
		struct wlan_fw_header h;
		mc_fread(&h, sizeof(h), 1, fp);
	}
	mc_fread(&ih, sizeof(ih), 1, fp);
	for (i = 0; i < ih.seg_cnt; i++) {
		mc_fread(&sh, sizeof(sh), 1, fp);
		if (sh.type != FW_BLK_LOADABLE_SEGMENT)
			continue;
		crc = tftp_calc_crc(fp, tftp->buf, PKTSIZE, sh.offset, sh.len);
		if (crc != sh.crc) {
			res = 0;
			break;
		}
	}
	mc_fclose(fp);
	mc_soft_crc32_fin();
	return res;
}

static void
tftp_write_timeout_callback(xsMachine *the, struct mc_timeout *tc, void *closure)
{
	tftp_t *tftp = closure;

	if (--tftp->retry_count <= 0) {
		mc_log_error("tftpd: timeout\n");
		tftp_close(tftp);
		return;
	}
	tftp_ack(tftp);
}

static void
tftp_write_callback(int s, unsigned int flags, void *closure)
{
	tftp_t *tftp = closure;
	int n;
	struct tftphdr *tp;
	int eof;

	if ((n = lwip_recv(s, tftp->buf, PKTSIZE, 0)) < 0) {
		mc_log_error("tftpd: lwip_recv failed\n");
		tftp_close(tftp);
		return;
	}
	if (n < 4) {
		mc_log_error("tftpd: patcket size error %d\n", n);
		tftp_close(tftp);
		return;
	}
	tp = (struct tftphdr *)tftp->buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	tp->th_block = ntohs(tp->th_block);
	// mc_log_notice("tftpd: %d read, %d block\n", n, tp->th_block);
	if ((tftp->blknum % 100) == 0)
		mc_log_notice(".");
	if (tp->th_opcode != DATA) {
		if (tp->th_opcode == ERROR)
			mc_log_error("tftpd: ERROR: %d\n", tp->th_code);
		else
			mc_log_error("tftpd: illegal opcode: %d\n", tp->th_opcode);
		tftp_close(tftp);
		return;
	}
	if (tp->th_block != tftp->blknum + 1) {
		/* discard this block and send a retransmission request */
		mc_log_error("tftpd: BAD block!? retransmit...\n");
		tftp_ack(tftp);
		mc_interval_start(tftp->interval);
		tftp->retry_count = tftp->retry;
		return;
	}
	eof = n < PKTSIZE;
	n -= 4;
	if (n > 0) {
		if (mc_fwrite(tp->th_data, 1, n, tftp->fp) != (size_t)n) {
			mc_log_error("tftpd: write error\n");
			tftp_error(tftp, ENOSPACE);
			return;
		}
		tftp->nbytes += n;
	}
	tftp->blknum = tp->th_block;
	tftp_ack(tftp);
	mc_interval_start(tftp->interval);
	tftp->retry_count = tftp->retry;
	if (eof) {
		mc_log_notice(" done.\n");
		mc_fclose(tftp->fp);
		tftp->fp = NULL;
		switch (tftp->filetype) {
		case TFTP_FTFS:
			if (check_crc_ftfs(tftp)) {
				mc_log_notice("tftp: set active %s\n", tftp->path);
				mc_set_active_volume(tftp->path);
			}
			else
				mc_log_error("tftp: CRC error\n");
			break;
		case TFTP_WIFIFW: {
			MC_FILE *fp;
			struct wlan_fw_header fh;
			if ((fp = mc_fopen(tftp->path, "r+")) == NULL)
				break;
			mc_fread(&fh, sizeof(fh), 1, fp);
			fh.length = tftp->nbytes;
			mc_fseek(fp, 0, SEEK_SET);
			mc_fwrite(&fh, sizeof(fh), 1, fp);
			mc_fclose(fp);
			/* no way to check the integrity?? */
			mc_log_notice("tftp: set active %s\n", tftp->path);
			mc_set_active_volume(tftp->path);
			break;
		}
		case TFTP_MCUFW:
			if (check_crc_bin(tftp)) {
				mc_log_notice("tftp: set active %s\n", tftp->path);
				mc_set_active_volume(tftp->path);
			}
			else
				mc_log_error("tftp: CRC error\n");
			break;
		case TFTP_BOOT2:
			break;
		default:
			break;
		}
		tftp_close(tftp);
	}
}

static int
tftp_save(tftp_t *tftp, const char *filename)
{
	if (!tftp_resolve_special_path(tftp, filename)) {
		const char *partition;
		if ((partition = mc_env_get_default("TFTP_PARTITION")) == NULL)
			partition = mc_get_special_dir("documentsDirectory");
		snprintf(tftp->path, sizeof(tftp->path), "%s/%s", partition, filename);
		tftp->filetype = TFTP_FILE;
	}
	if ((tftp->fp = mc_fopen(tftp->path, "w+")) == NULL) {
		mc_log_error("tftpd: %s: open failed\n", filename);
		tftp_close(tftp);
		return 0;
	}
	if (tftp->filetype == TFTP_WIFIFW) {
		struct wlan_fw_header fh;
		fh.magic = WLAN_FW_MAGIC;
		fh.length = 0;	/* fill in later */
		mc_fwrite(&fh, sizeof(fh), 1, tftp->fp);
	}
	mc_event_register(tftp->s, MC_SOCK_READ, tftp_write_callback, tftp);
	return 1;
}

static int
tftp_send_data(tftp_t *tftp)
{
	struct tftphdr *tp;
	size_t n;
	int pktlen;

	tp = (struct tftphdr *)tftp->buf;
	tp->th_opcode = htons(DATA);
	tp->th_block = htons(tftp->blknum);
	n = mc_fread(tp->th_data, 1, PKTSIZE - 4, tftp->fp);
	if ((pktlen = lwip_send(tftp->s, tftp->buf, n + 4, 0)) < 0)
		return -1;
	tftp->pktlen = pktlen;
	mc_interval_start(tftp->interval);
	return pktlen < PKTSIZE ? -1 : 0;
}

static void
tftp_read_timeout_callback(xsMachine *the, struct mc_timeout *tc, void *closure)
{
	tftp_t *tftp = closure;

	mc_log_error("tftpd: timeout\n");
	tftp_close(tftp);
}

static void
tftp_read_callback(int s, unsigned int flags, void *closure)
{
	tftp_t *tftp = closure;
	struct tftphdr ack;

	if (lwip_recv(s, &ack, 4, 0) != 4) {
		mc_log_error("tftpd: no ACK");
		tftp_close(tftp);
		return;
	}
	ack.th_opcode = ntohs(ack.th_opcode);
	if (ack.th_opcode != ACK) {
		if (ack.th_opcode == ERROR)
			mc_log_error("tftpd: ERROR: %d\n", ack.th_opcode);
		else
			mc_log_error("tftpd: illegal opcode: %d\n", ack.th_opcode);
		tftp_close(tftp);
		return;
	}
	ack.th_block = ntohs(ack.th_block);
	if (ack.th_block != tftp->blknum) {
		mc_log_notice("tftpd: retransmit\n");
		lwip_send(s, tftp->buf, tftp->pktlen, 0);
		mc_interval_start(tftp->interval);
	}
	else {
		tftp->blknum++;
		if (tftp_send_data(tftp) != 0) {
			/* EOF */
			tftp_close(tftp);
		}
	}
}

static int
tftp_get_file(tftp_t *tftp)
{
	if ((tftp->fp = mc_fopen(tftp->path, "r")) == NULL)
		return 0;
	mc_event_register(tftp->s, MC_SOCK_READ, tftp_read_callback, tftp);
	/* send the first block */
	tftp->blknum = 1;	/* start with the first data packet */
	if (tftp_send_data(tftp) != 0)
		tftp_close(tftp);
	return 1;
}

static int
tftp_get(tftp_t *tftp, const char *filename)
{
	int ret;

	if (tftp_resolve_special_path(tftp, filename))
		ret = tftp_get_file(tftp);
	else {
		int i;
		const char *vol;
		/* walk through all partitions */
		for (i = 0; (vol = mc_get_volume(i)) != NULL; i++) {
			snprintf(tftp->path, sizeof(tftp->path), "/%s/%s", vol, filename);
			tftp->filetype = TFTP_FILE;
			if ((ret = tftp_get_file(tftp)))
				break;
		}
		if (!ret) {
			/* try the filename as-is */
			snprintf(tftp->path, sizeof(tftp->path), "%s", filename);
			tftp->filetype = TFTP_FILE;
			ret = tftp_get_file(tftp);
		}
	}
	if (!ret)
		tftp_error(tftp, ENOTFOUND);	/* file not found */
	return ret;
}

void *
tftpd_connect(int s, xsMachine *the, xsSlot *this)
{
	tftp_t *tftp;
	struct sockaddr_in from;
	socklen_t slen;
	int n;
	unsigned short opcode;
	char *filename, *mode, *option;
	unsigned short errcode;

	if ((tftp = mc_calloc(1, sizeof(tftp_t))) == NULL)
		return NULL;
	tftp->the = the;
	tftp->this = *this;
	tftp->s = -1;
	slen = sizeof(from);
	memset(&from, 0, slen);
	if ((n = lwip_recvfrom(s, tftp->buf, PKTSIZE, 0, (struct sockaddr *)&from, &slen)) <= (int)sizeof(opcode))
		goto bail;
	opcode = *(unsigned short *)tftp->buf;
	opcode = ntohs(opcode);
	mc_log_notice("tftpd: connected from: %s\n", inet_ntoa(from.sin_addr));
	switch (opcode) {
	case RRQ:
	case WRQ:
		if (tftp_open(tftp, (struct sockaddr *)&from) != 0)
			goto bail;
		filename = (char *)tftp->buf + sizeof(opcode);
		mode = filename + strlen(filename) + 1;
		option = mode + strlen(mode) + 1;
		while (n > option - (char *)tftp->buf) {
			char *optval = option + strlen(option) + 1;
			if (strcmp(option, "timeout") == 0) {
				tftp->timeout = atoi(optval) * 1000;	/* in msec */
				mc_log_notice("tftpd: set timeout to %d\n", tftp->timeout);
			}
			option = optval + strlen(optval) + 1;
		}
		switch (opcode) {
		case WRQ:
			mc_log_notice("tftpd: WRQ: %s\n", filename);
			/* divide the timeout into the constant interval and set the retry count so that the total timeout period can be the same */
			tftp->retry = tftp->timeout / TFTP_TIMEOUT_INTERVAL;
			tftp->timeout = TFTP_TIMEOUT_INTERVAL;
			tftp->interval = mc_interval_set(tftp->timeout, tftp_write_timeout_callback, tftp);
			if (!tftp_save(tftp, filename))
				return NULL;
			/* send ack back */
			tftp_ack(tftp);
			mc_interval_start(tftp->interval);
			tftp->retry_count = tftp->retry;
			break;
		case RRQ:
			mc_log_notice("tftpd: RRQ: %s\n", filename);
			tftp->interval = mc_interval_set(tftp->timeout, tftp_read_timeout_callback, tftp);
			if (!tftp_get(tftp, filename))
				return NULL;
			break;
		}
		break;
	case DATA:
	case ACK:
		mc_log_notice("tftpd: DATA or ACK?\n");
		goto bail;
	case ERROR:
		errcode = *(unsigned short *)tftp->buf + sizeof(opcode);
		mc_log_error("tftpd: got an error: %d\n", ntohs(errcode));
		goto bail;
	default:
		goto bail;
	}
	return tftp;

bail:
	if (tftp != NULL)
		mc_free(tftp);
	return NULL;
}

void
tftpd_close(void *data)
{
	tftp_t *tftp = data;

	if (data == NULL)	/* just in case... */
		return;
	if (tftp->interval != NULL)
		mc_interval_reset(tftp->interval);
	if (tftp->s >= 0) {
		mc_event_unregister(tftp->s);
		lwip_close(tftp->s);
	}
	if (tftp->fp != NULL)
		mc_fclose(tftp->fp);
	mc_free(tftp);
}

#ifdef TFTPD_STANDALONE
static int tftpd_sock = -1;
static void *tftp_instance = NULL;

static void tftp_connect_callback(int s, unsigned int flags, void *closure)
{
	xsMachine *the = closure;

	if (!(flags & MC_SOCK_READ)) {
		tftpd_stop();
		return;
	}
	tftp_instance = tftpd_connect(s, the, NULL);
}

int tftpd_start(xsMachine *the)
{
	int s;
	struct sockaddr_in sin;

	if ((s = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		mc_log_error("tftpd: lwip_socket failed\n");
		return -1;
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(69);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	if (lwip_bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		mc_log_error("tftpd: lwip_bind failed\n");
		return -1;
	}
	mc_event_register(s, MC_SOCK_READ, tftp_connect_callback, the);
	tftpd_sock = s;
	return 0;
}

void tftpd_stop()
{
	if (tftpd_sock >= 0) {
		mc_event_unregister(tftpd_sock);
		lwip_close(tftpd_sock);
		tftpd_sock = -1;
	}
	if (tftp_instance != NULL) {
		tftpd_close(tftp_instance);
		tftp_instance = NULL;
	}
}
#endif	/* TFTPD_STANDALONE */
