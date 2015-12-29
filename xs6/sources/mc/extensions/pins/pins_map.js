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
import System from "system";

export const GPIO_MASK = 0x10000;

if (System.device == "K5") {
	var pin_head = [49, 48, 47, 46, 45, 44, 43, 42, 0, 1, 2, 3, 18, 19, 20, 21];
}
else {
	var pin_head = [];
}

export function pinmap(pin)
{
	return pin >= 1 && pin <= pin_head.length ? pin_head[pin - 1] : pin & ~GPIO_MASK;
}

export default pinmap;
