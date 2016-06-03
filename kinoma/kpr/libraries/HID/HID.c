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

#include <fcntl.h>
#include <unistd.h>

#include "kpr.h"

#define MIN_MOUSE -127
#define MAX_MOUSE 127
#define DEFAULT_JOYSTICK 0

static const UInt8 LEFT_CTRL = 0x01;
static const UInt8 LEFT_SHIFT = 0x02;
static const UInt8 LEFT_ALT = 0x04;
static const UInt8 LEFT_GUI = 0x08;
static const UInt8 RIGHT_CTRL = 0x10;
static const UInt8 RIGHT_SHIFT = 0x20;
static const UInt8 RIGHT_ALT = 0x40;
static const UInt8 RIGHT_GUI = 0x80;

typedef struct {
	int 	FD;
} KprHIDKeyboardRecord, *KprHIDKeyboard;

static void mapKey(int c, char *r0, char *r2){
	if (c >= 'a' && c <= 'z'){
		*r2 = c - 93;
	}else if (c == ' '){
		*r2 = 0x2c;
	}else if (c >= 'A' && c <= 'Z'){
		*r0 = LEFT_SHIFT;
		*r2 = c - 61;
	}else if (c >= '1' && c <= '9'){
		*r2 = c - 0x13;
	}else if (c == '0'){
		*r2 = 0x27;
	}else if (c >= '!' && c <= '&'){
		*r0 = LEFT_SHIFT;
		switch (c){
			case '!': *r2 = 0x1e; break;
			case '"': *r2 = 0x34; break;
			case '#': *r2 = 0x20; break;
			case '$': *r2 = 0x21; break;
			case '%': *r2 = 0x22; break;
			case '&': *r2 = 0x24; break;
		}
	}else if (c == '\''){
		*r2 = 0x34;
	}else if (c >= '(' && c <= '+'){
		*r0 = LEFT_SHIFT;
		switch (c){
			case '(': *r2 = 0x26; break;
			case ')': *r2 = 0x27; break;
			case '*': *r2 = 0x25; break;
			case '+': *r2 = 0x2E; break;
		}
	}else if (c >= ',' && c <= '/'){
		switch (c){
			case ',': *r2 = 0x36; break;
			case '-': *r2 = 0x2D; break;
			case '.': *r2 = 0x37; break;
			case '/': *r2 = 0x38; break;
		}
	}else if (c == ':'){
		*r0 = LEFT_SHIFT;
		*r2 = 0x33;
	}else if (c == ';'){
		*r2 = 0x33;
	}else if (c == '<'){
		*r0 = LEFT_SHIFT;
		*r2 = 0x36;
	}else if (c == '='){
		*r2 = 0x2e;
	}else if (c >= '>' && c <= '@'){
		*r0 = LEFT_SHIFT;
		switch (c){
			case '>': *r2 = 0x37; break;
			case '?': *r2 = 0x38; break;
			case '@': *r2 = 0x1F; break;
		}
	}else if (c == '['){
		*r2 = 0x2F;
	}else if (c == '\\'){
		*r2 = 0x31;
	}else if (c == ']'){
		*r2 = 0x30;
	}else if (c == '^'){
		*r0 = LEFT_SHIFT;
		*r2 = 0x23;
	}else if (c == '_'){
		*r0 = LEFT_SHIFT;
		*r2 = 0x2D;
	}else if (c == '`'){
		*r2 = 0x35;
	}else if (c >= '{' && c <= '~'){
		*r0 = LEFT_SHIFT;
		switch (c){
			case '{': *r2 = 0x2F; break;
			case '|': *r2 = 0x31; break;
			case '}': *r2 = 0x30; break;
			case '~': *r2 = 0x35; break;
		}
	}else if (c == '\b'){
		*r2 = 0x2a;
	}else if (c == '\t'){
		*r2 = 0x2b;
	}else if (c == '\n' || c == '\r'){
		*r2 = 0x28;
	}else if (c < 0){
		*r2 = -1 * c;
	}
}

FskErr KprHIDSendKey(int c, KprHIDKeyboard keyboard, int modifiers){
	FskErr err = kFskErrNone;
	char report[8] = {0,0,0,0,0,0,0,0};

	mapKey(c, report, report + 2);

	if (modifiers != -1) report[0] |= ((UInt8)modifiers);

	if (write(keyboard->FD, report, 8) != 8){
		err = kFskErrOperationFailed;
	}

	report[0] = 0;
	report[2] = 0;
	if (write(keyboard->FD, report, 8) != 8){
		err |= kFskErrOperationFailed;
	}
	return err;
}

FskErr KprHIDSendString(char* string, KprHIDKeyboard keyboard, int modifiers){
	FskErr err = kFskErrNone;
	while (*string != 0){
		err |= KprHIDSendKey(*string, keyboard, modifiers);
		string++;
	}
	return err;
}

FskErr KprHIDSendSpecial(long code, long times, KprHIDKeyboard keyboard, int modifiers){
	FskErr err = kFskErrNone;
	while (times > 0){
		err |= KprHIDSendKey(-1 * code, keyboard, modifiers);
		times--;
	}
	return err;
}

FskErr KprHIDKeysDown(char *keysToSend, KprHIDKeyboard keyboard, int modifiers){
	FskErr err = kFskErrNone;
	char report[8] = {0,0,0,0,0,0,0,0};
	int i;
	for (i = 0; i < 6; i++){
		mapKey(keysToSend[i], report, report + 2 + i);
	}
	if (modifiers != -1) report[0] |= ((UInt8)modifiers);
	if (write(keyboard->FD, report, 8) != 8){
		err = kFskErrOperationFailed;
	}
	return err;
}

FskErr KprHIDKeysUp(KprHIDKeyboard keyboard){
	FskErr err = kFskErrNone;
	char report[8] = {0,0,0,0,0,0,0,0};
	if (write(keyboard->FD, report, 8) != 8){
		err |= kFskErrOperationFailed;
	}
	return err;
}

static char* defaultKeyboardPath = "/dev/hidg0";

void KPR_HID_keyboard(void* kb){
	if (kb != NULL){
		KprHIDKeyboard keyboard = (KprHIDKeyboard)kb;
		if (keyboard->FD >= 0) close(keyboard->FD);
		FskMemPtrDispose(keyboard);
	}
}

void KPR_HID_closeKeyboard(xsMachine *the){
	KPR_HID_keyboard(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void KPR_HID_Keyboard(xsMachine *the){
	KprHIDKeyboard keyboard;
	char* path;
	int argc = xsToInteger(xsArgc);

	if (argc == 0){
		path = defaultKeyboardPath;
	}else{
		path = xsToString(xsArg(0));
	}

	FskMemPtrNewClear(sizeof(KprHIDKeyboardRecord), (FskMemPtr *)&keyboard);

	if ((keyboard->FD = open(path, O_RDWR, 0666)) == -1){
		xsThrowDiagnosticIfFskErr(kFskErrFileNotFound, "HID: error opening keyboard gadget file at %s", path);

		FskMemPtrDispose(keyboard);
		xsSetHostData(xsThis, NULL);
    }else{
		xsSetHostData(xsThis, keyboard);
	}

	xsSet(xsThis, xsID("LEFT_SHIFT"), xsInteger(LEFT_SHIFT));
	xsSet(xsThis, xsID("LEFT_CTRL"), xsInteger(LEFT_CTRL));
	xsSet(xsThis, xsID("LEFT_ALT"), xsInteger(LEFT_ALT));
	xsSet(xsThis, xsID("LEFT_GUI"), xsInteger(LEFT_GUI));
	xsSet(xsThis, xsID("RIGHT_SHIFT"), xsInteger(RIGHT_SHIFT));
	xsSet(xsThis, xsID("RIGHT_CTRL"), xsInteger(RIGHT_CTRL));
	xsSet(xsThis, xsID("RIGHT_ALT"), xsInteger(RIGHT_ALT));
	xsSet(xsThis, xsID("RIGHT_GUI"), xsInteger(RIGHT_GUI));
}

void KPR_HID_sendKey(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	int modifiers = -1;
	char *string = xsToString(xsArg(0));
	if (argc > 1) modifiers = xsToInteger(xsArg(1));
	xsThrowDiagnosticIfFskErr(KprHIDSendKey(string[0], (KprHIDKeyboard)xsGetHostData(xsThis), modifiers), "HID: Keyboard error sending key %c", string[0]);
}

void KPR_HID_sendString(xsMachine *the){
	int argc = xsToInteger(xsArgc);
	int modifiers = -1;
	char *string = xsToString(xsArg(0));
	if (argc > 1) modifiers = xsToInteger(xsArg(1));
	xsThrowDiagnosticIfFskErr(KprHIDSendString(string, (KprHIDKeyboard)xsGetHostData(xsThis), modifiers), "HID: Keyboard error sending string %s", string);
}

void KPR_HID_sendSpecial(xsMachine *the){
	int argc = xsToInteger(xsArgc);
	int modifiers = -1;
	long code = xsToInteger(xsArg(0));
	long times = 1;
	if (argc > 1) times = xsToInteger(xsArg(1));
	if (argc > 2) modifiers = xsToInteger(xsArg(2));

	xsThrowDiagnosticIfFskErr(KprHIDSendSpecial(code, times, (KprHIDKeyboard)xsGetHostData(xsThis), modifiers), "HID: Keyboard error sending special keycode %d", code);
}

void KPR_HID_keysDown(xsMachine *the){
	int argc = xsToInteger(xsArgc);
	int modifiers = -1;
	xsSlot array = xsArg(0);
	char keysToSend[7] = {0, 0, 0, 0, 0, 0, 0};

	if (! xsIsInstanceOf(array, xsArrayPrototype)){
		xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "HID: Keyboard error on keysDown. The first argument must be an %s", "array");
	}else{
		SInt32 length = xsToInteger(xsGet(array, xsID("length"))), i;
		if (length > 6){
			xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "HID: Keyboard error. You may only send 6 keys down at once. %s", " ");
			return;
		}
		for (i = 0; i < length; i++){
			xsSlot item = xsGet(array, i);
			xsType itemType = xsTypeOf(item);
			if (itemType == xsIntegerType || itemType == xsNumberType){
				keysToSend[i] = -1 * xsToInteger(item);
			}else if(itemType == xsStringType){
				keysToSend[i] = xsToString(item)[0];
			}
		}

		if (argc > 1) modifiers = xsToInteger(xsArg(1));
		xsThrowDiagnosticIfFskErr(KprHIDKeysDown(keysToSend, (KprHIDKeyboard)xsGetHostData(xsThis), modifiers), "HID: Keyboard error on keysDown. Tried to send keys: %s", keysToSend);
	}
}

void KPR_HID_keysUp(xsMachine *the){
	xsThrowDiagnosticIfFskErr(KprHIDKeysUp((KprHIDKeyboard)xsGetHostData(xsThis)), "HID: Keyboard error on %s", "keysUp");
}

/***************/
/*    Mouse    */
/***************/

typedef struct {
	int 	FD;
	SInt8 	status;
} KprHIDMouseRecord, *KprHIDMouse;

static char* defaultMousePath = "/dev/hidg1";

void KPR_HID_mouse(void* m){
	if (m != NULL){
		KprHIDMouse mouse = (KprHIDMouse)m;
		if (mouse->FD >= 0) close(mouse->FD);
		FskMemPtrDispose(mouse);
	}
}

void KPR_HID_closeMouse(xsMachine *the){
	KPR_HID_mouse(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void KPR_HID_Mouse(xsMachine *the){
	KprHIDMouse mouse;
	char* path;
	int argc = xsToInteger(xsArgc);

	if (argc == 0){
		path = defaultMousePath;
	}else{
		path = xsToString(xsArg(0));
	}

	FskMemPtrNewClear(sizeof(KprHIDMouseRecord), (FskMemPtr *)&mouse);

	if ((mouse->FD = open(path, O_RDWR, 0666)) == -1){
		xsThrowDiagnosticIfFskErr(kFskErrFileNotFound, "HID: error opening mouse gadget file at %s", path);

		FskMemPtrDispose(mouse);
		xsSetHostData(xsThis, NULL);
    }else{
		xsSetHostData(xsThis, mouse);
	}
}

FskErr KprHIDMoveMouse(SInt8 xDelta, SInt8 yDelta, KprHIDMouse mouse){
	FskErr err = kFskErrNone;
	SInt8 report[3];
    report[0] = mouse->status;
    report[1] = xDelta;
    report[2] = yDelta;

    if (write(mouse->FD, report, 3) != 3){
        err = kFskErrOperationFailed;
    }
	return err;
}

FskErr KprHIDMouseDown(Boolean button1, Boolean button2, Boolean button3, KprHIDMouse mouse){
	FskErr err = kFskErrNone;
	SInt8 report[3];
    report[0] = 0;
    report[1] = 0;
    report[2] = 0;

    if (button1) report[0] |= 0x01;
    if (button2) report[0] |= 0x02;
    if (button3) report[0] |= 0x04;

	mouse->status = report[0];
    if (write(mouse->FD, report, 3) != 3){
        err = kFskErrOperationFailed;
    }
	return err;
}

FskErr KprHIDMouseUp(KprHIDMouse mouse){
	FskErr err = kFskErrNone;
    SInt8 report[3] = {0, 0, 0};
	mouse->status = 0;

    if (write(mouse->FD, report, 3) != 3){
        err = kFskErrOperationFailed;
    }
	return err;
}

FskErr KprHIDMouseClick(Boolean button1, Boolean button2, Boolean button3, KprHIDMouse mouse){
	FskErr err = kFskErrNone;
    err = KprHIDMouseDown(button1, button2, button3, mouse);
    err |= KprHIDMouseUp(mouse);
	return err;
}

void KPR_HID_moveMouse(xsMachine *the){
    int xDelta = xsToInteger(xsArg(0));
    int yDelta = xsToInteger(xsArg(1));

    xsThrowDiagnosticIfFskErr(KprHIDMoveMouse(xDelta, yDelta, (KprHIDMouse)xsGetHostData(xsThis)), "HID: error moving mouse with deltas %d,%d", xDelta, yDelta);
}

void KPR_HID_clickMouse(xsMachine *the){
    Boolean button1 = xsToBoolean(xsArg(0));
    Boolean button2 = xsToBoolean(xsArg(1));
    Boolean button3 = xsToBoolean(xsArg(2));

    xsThrowDiagnosticIfFskErr(KprHIDMouseClick(button1, button2, button3, (KprHIDMouse)xsGetHostData(xsThis)), "HID: error clicking mouse buttons %d,%d,%d", button1, button2, button3);
}

void KPR_HID_mouseDown(xsMachine *the){
	Boolean button1 = xsToBoolean(xsArg(0));
	Boolean button2 = xsToBoolean(xsArg(1));
	Boolean button3 = xsToBoolean(xsArg(2));

	xsThrowDiagnosticIfFskErr(KprHIDMouseDown(button1, button2, button3, (KprHIDMouse)xsGetHostData(xsThis)), "HID: error on mouse down of buttons %d,%d,%d", button1, button2, button3);
}

void KPR_HID_mouseUp(xsMachine *the){
	xsThrowDiagnosticIfFskErr(KprHIDMouseUp((KprHIDMouse)xsGetHostData(xsThis)), "HID: error on mouse %s", "up");
}

/******************/
/*    Gamepad     */
/******************/

typedef struct {
	int 	FD;
	UInt8 	joystickValues[10];
} KprHIDGamepadRecord, *KprHIDGamepad;

static const UInt8 joystickDefaults[10] = {DEFAULT_JOYSTICK};
static char* defaultGamepadPath = "/dev/hidg2";

void KPR_HID_gamepad(void* gp){
	if (gp != NULL){
		KprHIDGamepad gamepad = (KprHIDGamepad)gp;
		if (gamepad->FD >= 0) close(gamepad->FD);
		FskMemPtrDispose(gamepad);
	}
}

void KPR_HID_closeGamepad(xsMachine *the){
	KPR_HID_gamepad(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void KPR_HID_Gamepad(xsMachine *the){
	KprHIDGamepad gp;
	char* path;
	int argc = xsToInteger(xsArgc);

	if (argc == 0){
		path = defaultGamepadPath;
	}else{
		path = xsToString(xsArg(0));
	}

	FskMemPtrNewClear(sizeof(KprHIDGamepadRecord), (FskMemPtr *)&gp);
	gp->FD = -1;
	FskMemCopy(gp->joystickValues, joystickDefaults, sizeof(joystickDefaults));

	if ((gp->FD = open(path, O_RDWR, 0666)) == -1){
		xsThrowDiagnosticIfFskErr(kFskErrFileNotFound, "HID: error opening gamepad gadget file at %s", path);

		FskMemPtrDispose(gp);
		xsSetHostData(xsThis, NULL);
    }else{
		xsSetHostData(xsThis, gp);
	}
}

static FskErr KprHIDSendGamepadValues(KprHIDGamepad gamepad){
	FskErr err = kFskErrNone;
	if (write(gamepad->FD, gamepad->joystickValues, 10) != 10){
		err = kFskErrOperationFailed;
	}
	return err;
}

void KPR_HID_sendGamepadPosition(xsMachine *the){
	int argc = xsToInteger(xsArgc);
	SInt16 value;
	xsSlot joystick;
	KprHIDGamepad gamepad = (KprHIDGamepad)xsGetHostData(xsThis);

	if (argc >= 1){
		joystick = xsArg(0);
		if (xsHas(joystick, xsID("x"))){
			value = xsToInteger(xsGet(joystick, xsID("x")));
			gamepad->joystickValues[0] = value & 0xFF;
			gamepad->joystickValues[1] = value >> 8;
		}
		if (xsHas(joystick, xsID("y"))){
			value = xsToInteger(xsGet(joystick, xsID("y")));
			gamepad->joystickValues[2] = value & 0xFF;
			gamepad->joystickValues[3] = value >> 8;
		}
	}
	if (argc >= 2){
		joystick = xsArg(1);
		if (xsHas(joystick, xsID("x"))){
			value = xsToInteger(xsGet(joystick, xsID("x")));
			gamepad->joystickValues[4] = value & 0xFF;
			gamepad->joystickValues[5] = value >> 8;
		}
		if (xsHas(joystick, xsID("y"))){
			value = xsToInteger(xsGet(joystick, xsID("y")));
			gamepad->joystickValues[6] = value & 0xFF;
			gamepad->joystickValues[7] = value >> 8;
		}
	}

	xsThrowDiagnosticIfFskErr(KprHIDSendGamepadValues(gamepad), "HID: gamepad error sending %s", "position");
}

void KPR_HID_pressGamepadButtons(xsMachine *the){
	int length;
	UInt8 i;
	KprHIDGamepad gamepad = (KprHIDGamepad)xsGetHostData(xsThis);

	if ( xsHas(xsArg(0), xsID("length")) ){
		length = xsToInteger(xsGet(xsArg(0), xsID("length")));
		for (i = 0; i < length; i++){
			if (xsTest(xsGet(xsArg(0), i))){
				if (i < 8){
					gamepad->joystickValues[8] |= (1 << i);
				}else{
					gamepad->joystickValues[9] |= (1 << (i - 8));
				}
			}else{
				if (i < 8){
					gamepad->joystickValues[8] &= ~(1 << i);
				}else{
					gamepad->joystickValues[9] &= ~(1 << (i - 8));
				}
			}
		}
		xsThrowDiagnosticIfFskErr(KprHIDSendGamepadValues(gamepad), "HID: error sending gamepad %s", "buttons");
	}else{
		xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "HID: Gamepad buttons should be %s", "an array of booleans.");
	}
}

void KPR_HID_releaseAllGamepadButtons(xsMachine *the){
	KprHIDGamepad gamepad = (KprHIDGamepad)xsGetHostData(xsThis);

	gamepad->joystickValues[4] = 0;
	gamepad->joystickValues[5] = 0;
	xsThrowDiagnosticIfFskErr(KprHIDSendGamepadValues(gamepad), "HID: error releasing gamepad %s", "buttons");
}

/*
	Extension
*/

FskExport(FskErr) HID_fskLoad(FskLibrary library)
{
    return kFskErrNone;
}

FskExport(FskErr) HID_fskUnload(FskLibrary library)
{
    return kFskErrNone;
}
