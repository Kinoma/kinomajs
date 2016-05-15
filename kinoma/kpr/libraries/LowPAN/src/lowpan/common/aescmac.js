//@module
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

/**
 * Kinoma LowPAN Framework: RFC4493 AES-CMAC
 */

var Utils = require("./utils");
var BTUtils = require("../bluetooth/core/btutils");

var logger = new Utils.Logger("AES-CMAC", msg => console.log(msg));
logger.loggingLevel = Utils.Logger.Level.INFO.level;

const BLOCK_SIZE = 16;
const MSB_IDX = 15;
const MSB_MASK = 0x80;
const ZERO = new Uint8Array(16).fill(0);
const RB = new Uint8Array(16).fill(0);
RB[0] = 0x87;

/**
 * Subkey Generation
 * - K must be LSB first
 * - Results will be LSB first
 */
function generateSubkey(encrypt, k) {
	return encrypt(k, ZERO).then(l => {
		let _generate = a => {
			let k = BTUtils.arrayLeftShift(a, 1);
			if ((a[MSB_IDX] & MSB_MASK) != 0) {
				k = BTUtils.arrayXOR(k, RB);
			}
			return k;
		};
		let k1 = _generate(l);
		let k2 = _generate(k1);
		return {k1, k2};
	});
}
exports.generateSubkey = generateSubkey;

/**
 * AES-CMAC Function
 * - K must be LSB first
 * - Results will be LSB first
 */
function cmac(encrypt, k, m, len = m.length) {
	return generateSubkey(encrypt, k).then(subkey => {
		let n = Math.ceil(len / BLOCK_SIZE);
		if (n == 0) {
			n = 1;
		}
		logger.trace("n=" + n);
		let _loop = ctx => {
			let block;
			let i = ctx.index * BLOCK_SIZE;
			logger.trace("index=" + ctx.index + ", i=" + i);
			if (ctx.index == (n - 1)) {
				let tmp = m.slice(i);
				let sk;
				if (tmp.length != BLOCK_SIZE) {
					/* Padding */
					block = new Uint8Array(BLOCK_SIZE).fill(0);
					block.set(tmp);
					block[BLOCK_SIZE - 1] = 0x80;
					sk = subkey.k2;
				} else {
					block = tmp;
					sk = subkey.k1;
				}
				/* LSB First */
				block = BTUtils.arraySwap(block);
				block = BTUtils.arrayXOR(block, sk);
			} else {
				block = m.slice(i, i + BLOCK_SIZE);
				/* LSB First */
				block = BTUtils.arraySwap(block);
			}
			logger.trace("Block: " + Utils.toFrameString(block));
			return encrypt(k, BTUtils.arrayXOR(ctx.x, block)).then(e => {
				ctx.x = e;
				ctx.index++;
				if (ctx.index < n) {
					return _loop(ctx);
				} else {
					return ctx.x;
				}
			});
		};
		return _loop({
			index: 0,
			x: ZERO
		});
	});
}
exports.cmac = cmac;
