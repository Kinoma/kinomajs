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
#define I2C_SUPPORT_CONCURRENCY	1
#define I2C_VERBOSE	1

#include "mc_xs.h"
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "ext_i2c.xs.c"
MC_MOD_DECL(I2C);
#endif

#if mxMC
#include <wmstdio.h>
#include <wm_os.h>

#if defined(CONFIG_CPU_MC200)
#include <mc200_pinmux.h>
#include <mc200.h>
#elif defined(CONFIG_CPU_MW300)
#include <mw300_pinmux.h>
#include <mw300.h>
#else
#endif

#include <i2c_reg.h>
#include <board.h>
#include <limits.h>

typedef struct {
	int port;
	int slave_addr;
	int fast;
	uint32_t txtout, rxtout;
	uint32_t scl_low, scl_high;
	uint32_t sticks;
	i2c_reg_t *reg;
} wm_i2c;

#define DEFAULT_TOUT	(50)	/* in msec */

#if defined(CONFIG_CPU_MC200)
static const uint32_t i2cAddr[3] = {I2C0_BASE, I2C1_BASE, I2C2_BASE};
#elif defined(CONFIG_CPU_MW300)
static const uint32_t i2cAddr[2] = {I2C0_BASE, I2C1_BASE};
#else
#endif

#define MIN_SCL_HIGH_TIME_STANDARD	4000	/* 4000ns for 100KHz */
#define MIN_SCL_LOW_TIME_STANDARD	4700	/* 4700ns for 100KHz */
#define MIN_SCL_HIGH_TIME_FAST		600	/* 600ns for 400KHz  */
#define MIN_SCL_LOW_TIME_FAST		1300	/* 1300ns for 400KHz */
#define CLK_DIV	3
#define NS2CNT(ns)	((((board_cpu_freq() / CLK_DIV) / 1000000) * ns) / 1000)

static int i2c_current_slave_addr = -1;

static inline void
i2c_timer_start(wm_i2c *i2c)
{
	i2c->sticks = os_ticks_get();
}

static inline int
i2c_timeout(wm_i2c *i2c, uint32_t tout)
{
	uint32_t t = os_ticks_get();
	uint32_t tt = os_msec_to_ticks(tout);

	return tout != 0 && (t >= i2c->sticks ? (t - i2c->sticks) : ((UINT_MAX - i2c->sticks) + t)) >= tt;
}

static inline uint32_t
i2c_get_abort_cause(wm_i2c *i2c)
{
	return i2c->reg->TX_ABRT_SOURCE.WORDVAL;
}

static inline void
i2c_clear_abort(wm_i2c *i2c)
{
	volatile uint32_t junk __attribute__((unused)) = i2c->reg->CLR_TX_ABRT.WORDVAL;
}

static int
i2c_wait_for_ack(wm_i2c *i2c, uint32_t tout)
{
	i2c_timer_start(i2c);
	do {
		if (!(i2c->reg->RAW_INTR_STAT.WORDVAL & 0x40 /* TX ABRT */))
			return true;
		i2c_clear_abort(i2c);
	} while (!i2c_timeout(i2c, tout));
	return false;
}

static int
i2c_wait_tx_fifo(wm_i2c *i2c)
{
	int tcnt;
	uint32_t toticks = os_msec_to_ticks(10);

	/* sending the stop sequence by emptying the TX FIFO */
	for (tcnt = 10; !(i2c->reg->STATUS.WORDVAL & 0x04 /* TFE */) && --tcnt >= 0;)
		os_thread_sleep(toticks);
#if I2C_VERBOSE
	if (tcnt < 0)
		mc_log_debug("I2C: TX FIFO can't be empty! STATUS = 0x%x\n", i2c->reg->STATUS.WORDVAL);
#endif
	return tcnt >= 0;
}

static int
i2c_xmit_ready(wm_i2c *i2c, uint32_t tout)
{
	i2c_timer_start(i2c);
	while (!(i2c->reg->STATUS.WORDVAL & 0x02 /* TFNF */)) {
		if (i2c_timeout(i2c, tout)) {
#if I2C_VERBOSE
			mc_log_debug("I2C: TX FIFO full! STATUS = 0x%x\n", i2c->reg->STATUS.WORDVAL);
#endif
			return false;
		}
	}
	return true;
}

static void
i2c_read_start(wm_i2c *i2c)
{
	int retry;
	uint32_t abrt;

	for (retry = 10; !(i2c->reg->STATUS.WORDVAL & 0x02 /* TFNF */);) {
		if (--retry < 0) {
			mc_log_debug("I2C: read: cannot write READ CMD\n");
			return;
		}
		os_thread_sleep(os_msec_to_ticks(10));
	}
	i2c->reg->DATA_CMD.WORDVAL = 0x100;	/* READ CMD */
	if (i2c->reg->RAW_INTR_STAT.WORDVAL & 0x40 /* TX ABRT */) {
		abrt = i2c_get_abort_cause(i2c);
		if (abrt & 0x0f) {
#if I2C_VERBOSE
			mc_log_debug("I2C: read: clear abort: 0x%x\n", abrt);
#endif
			i2c_clear_abort(i2c);
		}
	}
}

static int
i2c_write_byte(wm_i2c *i2c, int b)
{
	if (!i2c_xmit_ready(i2c, i2c->txtout))
		return false;
	i2c->reg->DATA_CMD.WORDVAL = b;
	return true;
}

static int
i2c_write_bytes(wm_i2c *i2c, int reg, uint8_t *data, int datasize)
{
	int retry = 60;
	int i;

	do {
		if (reg >= 0) {
			if (!i2c_write_byte(i2c, reg))
				return false;
		}
		for (i = 0; i < datasize; i++)
			if (!i2c_write_byte(i2c, data[i]))
				return false;
		
		if (!(i2c->reg->RAW_INTR_STAT.WORDVAL & 0x40 /* TX ABRT */))
			break;
		os_thread_sleep(os_msec_to_ticks(10));
#if I2C_VERBOSE > 1
		mc_log_debug("I2C: xmit aborted [%d]: cause = 0x%x.\n", retry, i2c_get_abort_cause(i2c));
#endif
		i2c_clear_abort(i2c);
		
	} while (--retry >= 0);
#if I2C_VERBOSE
	if (retry < 0)
		mc_log_debug("I2C: xmit aborted!\n");
#endif
	return (retry >= 0);
}

static void
i2c_config(wm_i2c *i2c)
{
	i2c_reg_t *reg = i2c->reg;

	reg->ENABLE.WORDVAL = 0;	/* disable I2C */
	if (i2c->fast) {
		reg->FS_SCL_LCNT.BF.FS_SCL_LCNT = NS2CNT(i2c->scl_low);
		reg->FS_SCL_HCNT.BF.FS_SCL_HCNT = NS2CNT(i2c->scl_high);
	}
	else {
		reg->SS_SCL_LCNT.BF.SS_SCL_LCNT = NS2CNT(i2c->scl_low);
		reg->SS_SCL_HCNT.BF.SS_SCL_HCNT = NS2CNT(i2c->scl_high);
	}
	reg->CON.WORDVAL = 0x40 /* slave disable */ | 0x20 /* restart enable */ | 0x01 /* master enable */ | (i2c->fast ? 0x04 : 0x02) /* speed */;
	reg->TAR.WORDVAL = i2c->slave_addr;	/* 7 bit addr mode | addr */
	reg->ENABLE.WORDVAL = 1;	/* enable I2C */
	i2c_current_slave_addr = i2c->slave_addr;
	os_thread_sleep(os_msec_to_ticks(35));
#if I2C_VERBOSE
	if (i2c_get_abort_cause(i2c) & 0x0f)
		mc_log_debug("I2C: init: NO ACK!\n");
#endif
}

static void
i2c_init(wm_i2c *i2c)
{
	static int clock_inited = 0;
	
	if (!clock_inited) {
		CLK_Module_Type clkmod;
		switch (i2c->port) {
		case 0:
		default:
			clkmod = CLK_I2C0;
			break;
		case 1:
			clkmod = CLK_I2C1;
			break;
		case 2:
			clkmod = CLK_I2C2;
			break;
		}
		CLK_ModuleClkEnable(clkmod);
		CLK_ModuleClkDivider(clkmod, CLK_DIV);
		clock_inited = true;
	}
	i2c_config(i2c);
}

static void
i2c_fin(wm_i2c *i2c)
{
	i2c->reg->ENABLE.WORDVAL = 0;	/* disable I2C */
}

void
xs_i2c_constructor(xsMachine *the)
{
	wm_i2c *i2c;
	int ac = xsToInteger(xsArgc);
	int port = xsToInteger(xsArg(0));
	uint8_t addr = (uint8_t)xsToInteger(xsArg(1));
	int fast = ac > 2 ? xsToBoolean(xsArg(2)) : false;

	if ((i2c = mc_malloc(sizeof(wm_i2c))) == NULL)
		mc_xs_throw(the, "I2C: no mem");
	i2c->port = port;
	i2c->reg = (i2c_reg_t *)i2cAddr[port];
	i2c->slave_addr = addr;
	i2c->fast = fast;
	i2c->txtout = i2c->rxtout = DEFAULT_TOUT;
	i2c->scl_low = fast ? MIN_SCL_LOW_TIME_FAST : MIN_SCL_LOW_TIME_STANDARD;
	i2c->scl_high = fast ? MIN_SCL_HIGH_TIME_FAST : MIN_SCL_HIGH_TIME_STANDARD;
	i2c_init(i2c);
	xsSetHostData(xsThis, i2c);
}

void
xs_i2c_destructor(void *data)
{
	wm_i2c *i2c = data;

	if (i2c != NULL) {
		i2c_fin(i2c);
		mc_free(i2c);
	}
}

static int
mc_i2c_read(wm_i2c *i2c, int reg, uint8_t *data, int nbytes)
{
	int i, nwritten = 0;

#if I2C_SUPPORT_CONCURRENCY
	if (i2c->slave_addr != i2c_current_slave_addr)
		i2c_config(i2c);
#endif
	if (reg >= 0) {
		if (!i2c_write_bytes(i2c, reg, NULL, 0))
			goto bail;
	}
	if (nbytes > 0) {
		i2c_read_start(i2c);	/* issue the first READ CMD in advance to keep it always in the txFIFO */
		nwritten++;
	}
	for (i = 0; i < nbytes; i++) {
		i2c_timer_start(i2c);
		if (nwritten < nbytes) {
			i2c_read_start(i2c);
			nwritten++;
		}
		while (!(i2c->reg->STATUS.WORDVAL & 0x08 /* RFNE */)) {
#if 0
			if (nwritten < nbytes) {
				if ((i2c->reg->STATUS.WORDVAL & (0x08|0x02)) == 0x02  /* RFNE | TFNF */) {
					i2c->reg->DATA_CMD.WORDVAL = 0x100;	/* READ CMD */
					nwritten++;
				}
			}
#endif
			if (i2c_timeout(i2c, i2c->rxtout)) {
#if I2C_VERBOSE
				mc_log_debug("I2C: read: timeout [%d]! STATUS = 0x%x, RISTAT = 0x%x, ABRT = 0x%x\n", i, i2c->reg->STATUS.WORDVAL, i2c->reg->RAW_INTR_STAT.WORDVAL, i2c->reg->TX_ABRT_SOURCE.WORDVAL);
#endif
				goto bail;
			}
		}
		data[i] = i2c->reg->DATA_CMD.WORDVAL;
	}
	return 0;
bail:
	return -1;
}

void
xs_i2c_read(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);
	int nbytes = xsToInteger(xsArg(1));
	int reg;

	switch (xsTypeOf(xsArg(0))) {
	case xsNumberType:
	case xsIntegerType:
		reg = xsToInteger(xsArg(0));
		break;
	default:
		reg = -1;
		break;
	}
	xsResult = xsArrayBuffer(NULL, nbytes);
	if (mc_i2c_read(i2c, reg, xsToArrayBuffer(xsResult), nbytes) != 0)
		xsSetUndefined(xsResult);
}


void
xs_i2c_readChar(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);
	int reg;
	uint8_t c;

	switch (xsTypeOf(xsArg(0))) {
	case xsNumberType:
	case xsIntegerType:
		reg = xsToInteger(xsArg(0));
		break;
	default:
		reg = -1;
		break;
	}
	if (mc_i2c_read(i2c, reg, &c, 1) == 0)
		xsSetInteger(xsResult, c);
}

void
xs_i2c_readWord(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);
	int reg;
	int nbytes = xsToInteger(xsArg(1));
	int lsbFirst = xsToInteger(xsArgc) > 2 ? xsToBoolean(xsArg(2)) : 0;
	uint8_t data[4];

	switch (xsTypeOf(xsArg(0))) {
	case xsNumberType:
	case xsIntegerType:
		reg = xsToInteger(xsArg(0));
		break;
	default:
		reg = -1;
		break;
	}
	if (nbytes > 4) nbytes = 4;
	if (mc_i2c_read(i2c, reg, data, nbytes) == 0) {
		uint32_t res = 0;
		int i;
		if (lsbFirst) {
			for (i = 0; i < nbytes; i++)
				res |= data[i] << (i * 8);
		}
		else {
			for (i = 0; i < nbytes; i++)
				res = (res << 8) | data[i];
		}
		xsSetInteger(xsResult, res);
	}
}

void
xs_i2c_write(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	int reg;
	uint8_t *data;
	int datasize;
	uint8_t num;
	uint8_t *allocated = NULL;

	if (ac < 2)
		return;
#if I2C_SUPPORT_CONCURRENCY
	if (i2c->slave_addr != i2c_current_slave_addr)
		i2c_config(i2c);
#endif
	switch (xsTypeOf(xsArg(0))) {
	case xsNumberType:
	case xsIntegerType:
		reg = xsToInteger(xsArg(0));
		break;
	default:
		reg = -1;
		break;
	}
	switch (xsTypeOf(xsArg(1))) {
	case xsIntegerType:
	case xsNumberType:
		num = (uint8_t)xsToInteger(xsArg(1));
		data = &num;
		datasize = 1;
		break;
	case xsReferenceType:
		if (xsIsInstanceOf(xsArg(1), xsArrayPrototype)) {
			int i;
			xsVars(1);
			xsGet(xsVar(0), xsArg(1), xsID("length"));
			datasize = xsToInteger(xsVar(0));
			if ((allocated = mc_malloc(datasize)) == NULL)
				mc_xs_throw(the, "no mem");
			for (i = 0; i < datasize; i++) {
				xsGet(xsVar(0), xsArg(1), (xsIndex)i);
				allocated[i] = (uint8_t)xsToInteger(xsVar(0));
			}
			data = allocated;
		}
		else {
			datasize = xsGetArrayBufferLength(xsArg(1));
			data = xsToArrayBuffer(xsArg(1));
		}
		break;
	default:
		mc_xs_throw(the, "args");
		return;	/* NOT REACHED */
	}
	if (ac > 2) {
		int n = xsToInteger(xsArg(2));
		if (datasize > n)
			datasize = n;
	}

	if (!i2c_wait_for_ack(i2c, i2c->txtout))
		mc_log_debug("I2C: write: no ack!\n");
	if (!i2c_write_bytes(i2c, reg, data, datasize))
		goto bail;
	(void)i2c_wait_tx_fifo(i2c);	/* send the stop sequence */

	if (allocated != NULL)
		mc_free(allocated);
	xsSetTrue(xsResult);
bail:
	return;
}

void
xs_i2c_close(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);

	i2c_fin(i2c);
}

void
xs_i2c_setTimeout(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);
	i2c->txtout = xsToInteger(xsArg(0));
	i2c->rxtout = xsToInteger(xsArg(1));
}

void
xs_i2c_setClock(xsMachine *the)
{
	wm_i2c *i2c = xsGetHostData(xsThis);
	i2c->scl_high = xsToInteger(xsArg(0));
	i2c->scl_low = xsToInteger(xsArg(1));
	i2c_config(i2c);
}
#else
void
xs_i2c_constructor(xsMachine *the)
{
}

void
xs_i2c_destructor(void *data)
{
}

void
xs_i2c_read(xsMachine *the)
{
}

void
xs_i2c_readChar(xsMachine *the)
{
}

void
xs_i2c_readWord(xsMachine *the)
{
}

void
xs_i2c_write(xsMachine *the)
{
}

void
xs_i2c_close(xsMachine *the)
{
}

void
xs_i2c_setTimeout(xsMachine *the)
{
}

void
xs_i2c_setClock(xsMachine *the)
{
}
#endif
