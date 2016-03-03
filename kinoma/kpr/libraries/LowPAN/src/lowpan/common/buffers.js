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
 * Kinoma LowPAN Framework: Buffer classes
 */

class Buffer {
	constructor(capacity) {
		this._capacity = capacity;
		this._limit = capacity;
		this._markPos = 0;
		this._position = 0;
	}
	getCapaciy() {
		return this._capacity;
	}
	getPosition() {
		return this._position;
	}
	setPosition(position) {
		if (position < this._markPos || position > this._limit) {
			throw "Out Of Range";
		}
		this._position = position;
	}
	getLimit() {
		return this._limit;
	}
	setLimit(limit) {
		if (limit < this._position || limit > this._capacity) {
			throw "Out Of Range";
		}
		this._limit = limit;
	}
	skip(n) {
		this._position += n;
	}
	mark() {
		this._markPos = this._position;
	}
	reset() {
		this._position = this._markPos;
	}
	rewind() {
		this._markPos = 0;
		this._position = 0;
	}
	remaining() {
		return this._limit - this._position;
	}
	flip() {
		this._limit = this._position;
		this.rewind();
	}
	clear() {
		this._limit = this._capacity;
		this.rewind();
	}
}
exports.Buffer = Buffer;

class ByteBuffer extends Buffer {
	constructor(array, offset, length, littleEndian) {
		super(length);
		if (littleEndian === undefined) {
			littleEndian = true;
		}
		this._array = array;
		this._offset = offset;
		this._littleEndian = littleEndian;
	}
	static allocate(capacity, littleEndian) {
		return new ByteBuffer(new Array(capacity), 0, capacity, littleEndian);
	}
	static allocateUint8Array(capacity, littleEndian) {
		return new ByteBuffer(new Uint8Array(capacity).fill(0), 0, capacity, littleEndian);	// XXX: XS6 init bug
	}
	static wrap(array, offset = 0, length = array.length, littleEndian) {
		return new ByteBuffer(array, offset, length, littleEndian);
	}
	get littleEndian() {
		return this._littleEndian;
	}
	set littleEndian(littleEndian) {
		this._littleEndian = littleEndian;
	}
	get array() {
		return this._array;
	}
	put(src) {
		this._array[this._offset + this._position] = src;
		this._position++;
	}
	putByteArray(src, offset = 0, length = src.length) {
		for (let i = 0; i < length; i++) {
			this.put(src[i + offset]);
		}
	}
	putInt8(i) {
		this.put(i & 0xFF);
	}
	putInt16(i) {
		if (this.littleEndian) {
			this.put(i & 0xFF);
			this.put((i >> 8) & 0xFF);
		} else {
			this.put((i >> 8) & 0xFF);
			this.put(i & 0xFF);
		}
	}
	putInt32(i) {
		if (this.littleEndian) {
			this.put(i & 0xFF);
			this.put((i >> 8) & 0xFF);
			this.put((i >> 16) & 0xFF);
			this.put((i >> 24) & 0xFF);
		} else {
			this.put((i >> 24) & 0xFF);
			this.put((i >> 16) & 0xFF);
			this.put((i >> 8) & 0xFF);
			this.put(i & 0xFF);
		}
	}
	get(peek) {
		if (this.remaining() < 1) {
			throw "BufferUnderflow";
		}
		let b = this._array[this._offset + this._position];
		if (peek === undefined || !peek) {
			this._position++;
		}
		return b;
	}
	peek() {
		return this.get(true);
	}
	getByteArray(octets) {
		if (octets === undefined) {
			octets = this.remaining();
		}
		let dst = new Uint8Array(octets);
		for (let i = 0; i < octets; i++) {
			dst[i] = this.get();
		}
		return dst;
	}
	getInt8() {
		return this.get() & 0xFF;
	}
	getInt16() {
		let s = 0;
		if (this.littleEndian) {
			s |= (this.get() & 0xFF);
			s |= (this.get() & 0xFF) << 8;
		} else {
			s |= (this.get() & 0xFF) << 8;
			s |= (this.get() & 0xFF);
		}
		return s;
	}
	getInt32() {
		let s = 0;
		if (this.littleEndian) {
			s |= (this.get() & 0xFF);
			s |= (this.get() & 0xFF) << 8;
			s |= (this.get() & 0xFF) << 16;
			s |= (this.get() & 0xFF) << 24;
		} else {
			s |= (this.get() & 0xFF) << 24;
			s |= (this.get() & 0xFF) << 16;
			s |= (this.get() & 0xFF) << 8;
			s |= (this.get() & 0xFF);
		}
		return s;
	}
}
exports.ByteBuffer = ByteBuffer;

class SerialBuffer extends ByteBuffer {
	constructor(serial, bufferSize, littleEndian) {
		super(new Uint8Array(bufferSize), 0, bufferSize, littleEndian);
		this._serial = serial;
		this._serialPosition = 0;
		this._debugEnabled = false;
	}
	read() {
		let buffer = this._serial.read("ArrayBuffer");
		this._pushRXData(new Uint8Array(buffer));
	}
	_pushRXData(data) {
		if (this._debugEnabled) {
			trace("[SerialBuffer] read: { ");
			for (let i = 0; i < data.length; i++) {
				trace(data[i].toString(16) + " ");
			}
			trace("}\n");
		}
		let totalLength = this._offset + this._serialPosition + data.length;
		if (this._array.length < totalLength) {
			let extended = new Uint8Array(totalLength);
			for (let i = 0; i < this._array.length; i++) {
				extended[i] = this._array[i];
			}
			this._array = extended;
			trace("[SerialBuffer] Buffer size has been extended to " + this._array.length + "\n");
		}
		for (let i = 0; i < data.length; i++) {
			this._array[this._offset + this._serialPosition] = data[i];
			this._serialPosition++;
		}
		this._limit = this._serialPosition;
	}
	flush(octets) {
		if (octets === undefined) {
			octets = (this._position - this._serialPosition);
		}
		let buffer = new ArrayBuffer(octets);
		let data = new Uint8Array(buffer);		// Uint8Array view
		for (let i = 0; i < octets; i++) {
			data[i] = this._array[this._offset + this._serialPosition];
			this._serialPosition++;
		}
		if (this._debugEnabled) {
			trace("[SerialBuffer] flush: { ");
			for (let i = 0; i < octets; i++) {
				trace(data[i].toString(16) + " ");
			}
			trace("}\n");
		}
		this._serial.write(buffer);
	}
	clear() {
		if (this._serialPosition != this._position) {
			if (this._serialPosition > this._position) {
				/* Reader usage: Clear but keep remaining */
				let data = this.getByteArray();
				this._serialPosition = 0;
				super.clear();
				this._pushRXData(data);
			} else {
				/* Writer usage: Flush all */
				this.flush();
				this._serialPosition = 0;
				super.clear();
			}
		} else {
			this._serialPosition = 0;
			super.clear();
		}
	}
}
exports.SerialBuffer = SerialBuffer;
