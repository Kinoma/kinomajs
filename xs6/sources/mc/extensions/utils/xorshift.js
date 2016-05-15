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
// https://en.wikipedia.org/wiki/Xorshift

function bsl32(val, n) {
	while (n-- > 0) val *= 2;
	return val % 0x100000000;
}

function bsr32(val, n) {
	while (n-- > 0) val = (val / 2) | 0;
	return val;
}

function sep32(a) {
	return [((a >> 16) & 0xffff), (a & 0xffff)];
}

function mix32(ah, al) {
	return ah * 0x10000 + al;
}

function xor32(a, b) {
	const [ah, al] = sep32(a);
	const [bh, bl] = sep32(b);
	return mix32(ah ^ bh, al ^ bl);
}

export function xorshift32(seed=undefined) {
	let y = seed !== undefined ? seed : Math.abs(0 | Math.random() * 0x100000000);
	return () => {
		y = xor32(y, bsl32(y, 13));
		y = xor32(y, bsr32(y, 17));
		y = xor32(y, bsl32(y, 15));
		return y;
	}
}

export default xorshift32;

// seed of 2463534242
// 901999875  3371835698  2675058524  1053936272  3811264849  472493137  3856898176  2131710969  2312157505  4125513277
