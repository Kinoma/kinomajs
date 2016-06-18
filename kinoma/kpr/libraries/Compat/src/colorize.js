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
export let RED = 91;
export let GREEN = 92;
export let YELLOW = 93;
export let BLUE = 94;
export let MAGENTA = 95;
export let CYAN = 96;
export let BOLD = 1;
let END = 0;

function escape(color) {
	return "\x1b[" + color + 'm';
}

export function colored(s, color) {
	return escape(color) + s + escape(END);
}

export default {
	colored,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	BOLD
};

