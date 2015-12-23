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

#include "FskPin.h"

#include "kprPins.h"

static Boolean createSerialCanHandle(SInt32 rxNumber, SInt32 txNumber, const char *name, char **remappedName);

FskPinSerialDispatchRecord gCreatePinSerial = {
	createSerialCanHandle,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

Boolean createSerialCanHandle(SInt32 rxNumber, SInt32 txNumber, const char *name, char **remappedName)
{
	int ttyNumrx = -1, ttyNumtx = -1, ttyNum = -1;
	char *ttyPath;

	if (name)
		return false;

	if ((0 == rxNumber) && (0 == txNumber))
		return false;

	if (rxNumber) {
		ttyNumrx = FskHardwarePinsMux(rxNumber, kFskHardwarePinUARTRX);
		if (ttyNumrx < 0)
			ttyNumrx = FskHardwarePinsMux(rxNumber, kFskHardwarePinUARTTX);
	}
	if (txNumber) {
		ttyNumtx = FskHardwarePinsMux(txNumber, kFskHardwarePinUARTTX);
		if (ttyNumtx < 0) {
			ttyNumtx = FskHardwarePinsMux(txNumber, kFskHardwarePinUARTRX);
		}
	}

	if (((rxNumber && txNumber) && (ttyNumrx != ttyNumtx)) || (rxNumber && (ttyNumrx < 0)) || (txNumber && (ttyNumtx < 0)))
		return false;

    if (-1 != ttyNumrx)
        ttyNum = ttyNumrx;
    else if (-1 != ttyNumtx)
        ttyNum = ttyNumtx;
	else
		return false;

	if (kFskErrNone != FskMemPtrNew(60, &ttyPath))
		return false;

	sprintf(ttyPath, "/dev/ttyS%d", ttyNum);

	*remappedName = ttyPath;

	return true;
}

/*
	Extension
*/

FskExport(FskErr) FskPinSerialCreate_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinSerial, &gCreatePinSerial);
}

FskExport(FskErr) FskPinSerialCreate_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinSerial, &gCreatePinSerial);
}
