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
 * Kinoma LowPAN Framework: Common Utilities
 */

const INT_64_SIZE = 8;
const INT_32_SIZE = 4;
const INT_16_SIZE = 2;
const INT_8_SIZE = 1;
const BYTE_SIZE = 8;

exports.INT_32_SIZE = INT_32_SIZE;
exports.INT_16_SIZE = INT_16_SIZE;
exports.INT_8_SIZE = INT_8_SIZE;

function copy(src, srcOff, dst, dstOff, len) {
	if (srcOff != 0 || (srcOff + len) != src.len) {
		src = src.slice(srcOff, srcOff + len);
	}
	dst.set(src, dstOff);
}

/******************************************************************************
 * Byte manipulations
 ******************************************************************************/
function toInt(src, off, len, littleEndian) {
	if (littleEndian === undefined) {
		littleEndian = true;
	}
	let dest = 0;
	for (let p = 0; p < len; p++) {
		let d = src[off + p] & 0xff;
		if (littleEndian) {
			dest |= (d << (BYTE_SIZE * p));
		} else {
			dest |= (d << (BYTE_SIZE * (len - 1 - p)));
		}
	}
	return dest;
}
exports.toInt = toInt;

exports.toInt16 = function (src, littleEndian) {
	return toInt(src, 0, INT_16_SIZE, littleEndian);
};

exports.toInt32 = function (src, littleEndian) {
	return toInt(src, 0, INT_32_SIZE, littleEndian);
};

function multiIntToByteArray(src, size, length, littleEndian) {
	if (littleEndian === undefined) {
		littleEndian = true;
	}
	let dest = new Uint8Array(size * length);
	for (let index = 0; index < length; index++) {
		for (let p = 0; p < size; p++) {
			if (littleEndian) {
				dest[p + size * index] = (src[index] >> (BYTE_SIZE * p)) & 0xff;
			} else {
				dest[p + size * index] = (src[index] >> (BYTE_SIZE * (size - 1 - p))) & 0xff;
			}
		}
	}
	return dest;
}
exports.multiIntToByteArray = multiIntToByteArray;

exports.toByteArray = function (src, size, littleEndian) {
	return multiIntToByteArray([src], size, 1, littleEndian);
};

function toHexString0(b) {
    let hex = b.toString(16);
    let len = hex.length;
    if (len == 2) {
        return hex;
    } else if (len == 1) {
        return "0" + hex;
    } else {
        return hex.substring(len - 2);
    }
}

function toHexString(src, size = 1, prefix) {
	if (prefix === undefined) {
		prefix = "0x";
	}
	let dst = prefix;
	for (let p = 0; p < size; p++) {
		dst = dst.concat(toHexString0((src >> (BYTE_SIZE * (size - 1 - p))) & 0xff));
	}
	return dst;
}
exports.toHexString = toHexString;

exports.toFrameString = function (chunk, offset = 0, length = chunk.length) {
	let str = "[ ";
	for (let i = 0; i < length; i++) {
		str += (toHexString(chunk[i + offset]) + ", ");
	}
	str += "]";
	return str;
};

exports.toSignedByte = function (b) {
	if ((b & 0x80) > 0) {
		return b | ~(0xFF);
	}
	return b;
};

class Sequence {
	constructor(bits) {
		this._sequence = 0;
		if (bits < 0) {
			throw "IllegalArgument";
		}
		this._mask = 0x01;
		for (let i = 0; i < (bits - 1); i++) {
			this._mask |= (this._mask << 1);
		}
	}
	nextSequence() {
		let next = this._sequence;
		this._sequence = (this._sequence + 1) & this._mask;
		return next;
	}
}
exports.Sequence = Sequence;

const Level = {
	TRACE: {
		name: "TRACE",
		level: 7
	},
	DEBUG: {
		name: "DEBUG",
		level: 5
	},
	INFO: {
		name: "INFO",
		level: 2
	},
	WARN: {
		name: "WARN",
		level: 1
	},
	ERROR: {
		name: "ERROR",
		level: 0
	}
};

class Logger {
	constructor(name, binding = null) {
		this._name = name;
		this._loggingLevel = Level.INFO;
		if (binding == null) {
			if (Logger._defaultBinding != null) {
				binding = Logger._defaultBinding;
			} else {
				binding = msg => trace(msg + "\n");
			}
		}
		this._onLogging = binding;
		Logger._loggers[name] = this;	// Register
	}
	static setOutputEnabled(enabled) {
		Logger._outputEnabled = enabled;
	}
	static setDefaultBinding(binding) {
		Logger._defaultBinding = binding;
	}
	static getLogger(name) {
		if (!Logger._loggers.hasOwnProperty(name)) {
			Logger._loggers[name] = new Logger(name);
		}
		return Logger._loggers[name];
	}
	set loggingLevel(loggingLevel) {
		this._loggingLevel = loggingLevel;
	}
	log(level, str) {
		if ((level.level <= this._loggingLevel.level) && Logger._outputEnabled) {
			this._onLogging("[" + level.name + "] - [" + this._name + "] " + str);
		}
	}
	trace(str) {
		this.log(Level.TRACE, str);
	}
	debug(str) {
		this.log(Level.DEBUG, str);
	}
	info(str) {
		this.log(Level.INFO, str);
	}
	warn(str) {
		this.log(Level.WARN, str);
	}
	error(str) {
		this.log(Level.ERROR, str);
	}
}
Logger._loggers = {};
Logger._outputEnabled = true;
Logger._defaultBinding = null;
Logger.Level = Level;
exports.Logger = Logger;

class Ringbuffer {
	constructor(size) {
		this._size = size + 1;
		this._buffer = new Uint8Array(size + 1);
		this._head = 0;
		this._tail = 0;
	}
	get size() {
		return (this._size - 1);
	}
	available() {
		if (this._head >= this._tail) {
			return this._head - this._tail;
		} else {
			return (this._size - this._tail) + this._head;
		}
	}
	isFull() {
		return ((this._head + 1) % this._size) == this._tail;
	}
	readByte() {
		if (this.available() == 0) {
			return -1;
		}

		let b = this._buffer[this._tail];
		this._tail = (this._tail + 1) % this._size;
		return b & 0xFF;
	}
	read(b, off, length) {
		let available = this.available();
		if (length != 0 && length > available) {
			length = available;
		}

		if (length == 0) {
			return 0;
		}

		if (this._head >= this._tail) {
			copy(this._buffer, this._tail, b, off, length);
		} else {
			let front = this._size - this._tail;
			if (length <= front) {
				copy(this._buffer, this._tail, b, off, length);
			} else {
				copy(this._buffer, this._tail, b, off, front);
				copy(this._buffer, 0, b, off + front, length - front);
			}
		}
		this._tail = (this._tail + length) % this._size;
		return length;
	}
	writeByte(b) {
		let nextHead = (this._head + 1) % this._size;
		if (nextHead == this._tail) {
			throw "No more space left";
		}
		this._buffer[this._head] = b & 0xFF;
		this._head = nextHead;
	}
	write(b, off, length) {
		let space = (this._size - 1) - this.available();
		if (length > space) {
			throw "No more space left";
		}

		if (this._head >= this._tail) {
			let front = this._size - this._head;
			if (length > front) {
				copy(b, off, this._buffer, this._head, front);
				copy(b, off + front, this._buffer, 0, (length - front));
			} else {
				copy(b, off, this._buffer, this._head, length);
			}
		} else {
			copy(b, off, this._buffer, this._head, length);
		}
		this._head = (this._head + length) % this._size;
	}
}
exports.Ringbuffer = Ringbuffer;
