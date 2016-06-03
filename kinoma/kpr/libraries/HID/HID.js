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

class Keyboard @ "KPR_HID_keyboard" {
	constructor(path) @ "KPR_HID_Keyboard"
	close() @ "KPR_HID_closeKeyboard"

	sendKey(key, modifiers) @ "KPR_HID_sendKey"
	sendString(string, modifiers) @ "KPR_HID_sendString"
	sendSpecial(code, times, modifiers) @ "KPR_HID_sendSpecial"

	keysDown(keys, modifiers) @ "KPR_HID_keysDown"
	keysUp() @ "KPR_HID_keysUp"
}

class Mouse @ "KPR_HID_mouse" {
	constructor(path) @ "KPR_HID_Mouse"
	close() @ "KPR_HID_closeMouse"

	mouseDown(button1, button2, button3) @ "KPR_HID_mouseDown"
	mouseUp() @ "KPR_HID_mouseUp"
	click(button1, button2, button3) @ "KPR_HID_clickMouse"
	move(xDelta, yDelta) @ "KPR_HID_moveMouse"
}

class Gamepad @ "KPR_HID_gamepad" {
	constructor(path) @ "KPR_HID_Gamepad"
	close() @ "KPR_HID_closeGamepad"

	sendPosition(leftJoystick, rightJoystick) @ "KPR_HID_sendGamepadPosition"
	pressButtons(buttons) @ "KPR_HID_pressGamepadButtons"
	releaseButtons() @ "KPR_HID_releaseAllGamepadButtons"
}

export default  {
	Keyboard: Keyboard,
	Mouse: Mouse,
	Gamepad: Gamepad
}
