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
#include "mc_xs.h"
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "ext_spi.xs.c"
MC_MOD_DECL(SPI);
#endif


#if mxMC
#include <wm_os.h>
#include <mdev_ssp.h>

#define SSP_VERBOSE 1

typedef struct {
	mdev_t *mdev;
	uint8_t port;
	uint8_t dma;
	uint32_t freq;
} mc_ssp_t;

#define MC_SSP_MAX_ID	3
#define BUF_LEN	128
static int mc_ssp_ids[MC_SSP_MAX_ID] = {0};

void
xs_ssp_constructor(xsMachine *the)
{
	int port = xsToInteger(xsArg(0));
	int cs = xsToInteger(xsArgc) > 1 ? xsToInteger(xsArg(1)) : -1;
	int freq = xsToInteger(xsArgc) > 2 ? xsToInteger(xsArg(2)) : -1;
	int cpha = xsToInteger(xsArgc) > 3 ? xsToInteger(xsArg(3)) : -1;
	int cpol = xsToInteger(xsArgc) > 4 ? xsToInteger(xsArg(4)) : -1;

	mc_ssp_t *ssp;
	if(!mc_ssp_ids[port]){
#if SSP_VERBOSE
		mc_log_debug("[ssp]: ssp_drv_init: port = %d, cs = %d, freq = %d, cpha = %d, cpol = %d\n", port,cs, freq, cpha, cpol);
#endif		
		ssp_drv_init(port);
		mc_ssp_ids[port]++;
	}

	if((ssp = mc_malloc(sizeof(mc_ssp_t))) == NULL)
		mc_xs_throw(the, "ssp: no mem");
	
	ssp->port = port;
	ssp->dma = DMA_DISABLE;
	ssp->freq = freq;

	if(freq != -1){
		if(WM_FAIL == ssp_drv_set_clk(port, freq)){
			mc_free(ssp);
			mc_xs_throw(the, "ssp: ssp_drv_set_clk failed");
		}	
	}
	
	// ssp_drv_rxbuf_size(port, BUF_LEN);
	if((ssp->mdev = ssp_drv_open(port, SSP_FRAME_SPI, SSP_MASTER, ssp->dma, -1, 0/*, cpha, cpol*/)) == NULL){
		mc_free(ssp);
		mc_xs_throw(the, "ssp: ssp_drv_open failed");
	}

	xsSetHostData(xsThis, ssp);
}

void
xs_ssp_destructor(void *data)
{
	if(data){
		mc_free(data);
		data = NULL;
	}
}

void
xs_ssp_close(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
	if(ssp && ssp->mdev)
		ssp_drv_close(ssp->mdev);
	if(ssp){
		ssp_drv_deinit(ssp->port);
		if(mc_ssp_ids[ssp->port]) --mc_ssp_ids[ssp->port];
	}
		
}

void 
xs_ssp_cs_activate(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
#if SSP_VERBOSE
	mc_log_debug("xs_ssp_cs_activate\n");
#endif		
	ssp_drv_cs_activate(ssp->mdev);	
}


void xs_ssp_cs_deactivate(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
#if SSP_VERBOSE
	mc_log_debug("xs_ssp_cs_deactivate\n");
#endif	
	ssp_drv_cs_deactivate(ssp->mdev);	
}

void
xs_ssp_read(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
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
	if(reg != -1)
		ssp_drv_write(ssp->mdev, (uint8_t *)&reg, NULL, 1, 0);

	if(WM_FAIL == ssp_drv_read(ssp->mdev, xsToArrayBuffer(xsResult), nbytes))
		xsSetUndefined(xsResult);
}



void
xs_ssp_readChar(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
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


	if(reg != -1){
#if SSP_VERBOSE
	mc_log_debug("ssp_drv_write: reg = 0x%X\n", reg);
#endif	
		ssp_drv_write(ssp->mdev, (uint8_t*)&reg, NULL, 1, 0);
	}
	
	ssp_drv_read(ssp->mdev, &c, 1);
#if SSP_VERBOSE
	mc_log_debug("ssp_drv_read: c = 0x%X\n", c);
#endif	

	xsSetInteger(xsResult, c);
}

void
xs_ssp_readWord(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
	int reg;
	int lsbFirst = xsToInteger(xsArgc) > 1 ? xsToBoolean(xsArg(1)):0;
	uint8_t data[2];
	uint16_t res;

	switch (xsTypeOf(xsArg(0))) {
	case xsNumberType:
	case xsIntegerType:
		reg = xsToInteger(xsArg(0));
		break;
	default:
		reg = -1;
		break;
	}

#if SSP_VERBOSE
	mc_log_debug("xs_ssp_readWord\n");
#endif	

	if(reg != -1)
		ssp_drv_write(ssp->mdev, (uint8_t*)&reg, NULL, 1, 0);
	ssp_drv_read(ssp->mdev, data, 2);
	if(lsbFirst)
		res = data[0] + (data[1] << 8);
	else
		res = (data[0] << 8) + data[1];
	xsSetInteger(xsResult, res);
}

void
xs_ssp_write(xsMachine *the)
{
	mc_ssp_t *ssp = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc), reg = -1, datasize, dma = 0;
	uint8_t *data, num, *allocated = NULL;

	if(ac < 3) return;

	switch(xsTypeOf(xsArg(0))){
		case xsNumberType:
		case xsIntegerType:
			reg = xsToInteger(xsArg(0));
			break;
		default: 
			break;
	}

	switch(xsTypeOf(xsArg(2))){
		case xsNumberType:
		case xsIntegerType:
			dma = xsToInteger(xsArg(2));
			break;
		default: break;
	}

	switch(xsTypeOf(xsArg(1))){
		case xsIntegerType:
		case xsNumberType:
			num = (uint8_t) xsToInteger(xsArg(1));
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
				// datasize = xsGetArrayBufferLength(xsArg(1));
				datasize = xsToInteger(xsArg(3));
				data = xsToArrayBuffer(xsArg(1));
				// mc_log_debug("xs_ssp_write: arrayBuffer: datasize: %d\n", datasize);
			}
			break;
		default:
			mc_xs_throw(the, "args");
			return;	/* NOT REACHED */
	}
	if (ac > 3) {
		int n = xsToInteger(xsArg(3));
		// mc_log_debug("ac = %d, n = %d\n", ac, n);
		if (datasize > n)
			datasize = n;

		if(ac > 4)
			data += xsToInteger(xsArg(4));
	}

	if(reg != -1)
		ssp_drv_write(ssp->mdev, (uint8_t*)&reg, NULL, 1, 1);


	if(ssp->dma != dma){
		ssp->dma = dma;
		ssp_drv_close(ssp->mdev);
		ssp_drv_set_clk(ssp->port, ssp->freq);
		if((ssp->mdev = ssp_drv_open(ssp->port, SSP_FRAME_SPI, SSP_MASTER, ssp->dma, -1, 0/*, cpha, cpol*/)) == NULL){
			mc_free(ssp);
			mc_xs_throw(the, "ssp: ssp_drv_open failed");
		}
	}

// #if SSP_VERBOSE
// 	mc_log_debug("xs_ssp_write: dma = %d, datasize: %d\n", ssp->dma, datasize);
// #endif	
	// reg = os_ticks_to_msec(os_ticks_get());
	ssp_drv_write(ssp->mdev, (uint8_t *)data, NULL, datasize, 1);
	// mc_log_debug("time: %d\n", os_ticks_to_msec(os_ticks_get()) - reg );
	if(allocated) mc_free(allocated);
	xsSetTrue(xsResult);
}


#else

void
xs_ssp_constructor(xsMachine *the)
{
}

void
xs_ssp_destructor(void *data)
{
}

void
xs_ssp_close(xsMachine *the)
{
}

void
xs_ssp_cs_activate(xsMachine *the)
{
}

void
xs_ssp_cs_deactivate(xsMachine *the)
{
}

void
xs_ssp_read(xsMachine *the)
{
}

void
xs_ssp_readChar(xsMachine *the)
{
}

void
xs_ssp_readWord(xsMachine *the)
{
}

void
xs_ssp_write(xsMachine *the)
{
}

#endif
