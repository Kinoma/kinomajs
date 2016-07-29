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
#ifndef __MC_USB_H__
#define __MC_USB_H__

#include <stddef.h>

/* K5 Driver Library */
#include "usb/cdc.h"

extern int mc_usb_init();
extern void mc_usb_fin();
#define mc_usb_write(buf, n)	CdcWrite((uint8_t *)buf, 0, n)
#define mc_usb_read(buf, n)		CdcRead((uint8_t *)buf, 0, n)

#endif /* __MC_USB_H__ */
