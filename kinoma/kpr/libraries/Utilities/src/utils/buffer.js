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
/* Reader */

// private methods

/* convert integer array representing UTF8 string char codes to string
 * @param bytes array of uint
 * @return string composet
 */
function bytesToCharacter(bytes) {
	var data = new Uint8Array(bytes);
	var c = String.fromArrayBuffer(data.buffer);
	return c;
}

export const LittleEndian = true;
export const BigEndian = false;
export const NetworkByteOrder = false;

function byteOrderToBeUsed(obj, byteOrder) {
	return byteOrder !== undefined ? byteOrder : obj.defaultByteOrder;
}

export class Reader {
	constructor(byteOrder = NetworkByteOrder) {
		this.buffer = new ArrayBuffer(0);
		this.defaultByteOrder = byteOrder; // NetworkOrder
		this.readCount = 0;
	}

	get length() {
		return this.buffer.byteLength;
	}

	resetReadCount() {
		this.readCount = 0;
	}

	feed(buffer) {
		if (buffer.byteLength > 0) {
			this.buffer = this.buffer.concat(buffer);
		}
	}

	read(size) {
		var buffer = this.buffer.slice(0, size);
		this.skip(size);
		return buffer;
	}

	skip(size) {
		this.buffer = this.buffer.slice(size);
		this.readCount += parseInt(size);
		return size;
	}

	peek(pos) {
		if (pos < this.length) {
			var view = new Uint8Array(this.buffer);
			return view[pos];
		}
	}

	peekCharacterBytes(pos) {
		var c = this.peek(pos);
		if (c === undefined) return;

		let bytes = [c];
		if (c < 0x80) return bytes;

		var size;
		if (c <= 0xdf) size = 2;
		else if (c <= 0xef) size = 3;
		else if (c <= 0xf7) size = 4;
		else return;

		if ((pos + size - 1) >= this.length) return;

		for (var i = 1; i < size; i++) {
			c = this.peek(pos + i);
			bytes.push(c);
		}

		return bytes;
	}

	readInt8() {
		if (this.length < 1) return;
		var view = new DataView(this.buffer);
		var result = view.getInt8(0);
		this.skip(1);
		return result;
	}

	readInt16(byteOrder) {
		if (this.length < 2) return;
		var view = new DataView(this.buffer);
		var result = view.getInt16(0, byteOrderToBeUsed(this, byteOrder));
		this.skip(2);
		return result;
	}

	readInt32(byteOrder) {
		if (this.length < 4) return;
		var view = new DataView(this.buffer);
		var result = view.getInt32(0, byteOrderToBeUsed(this, byteOrder));
		this.skip(4);
		return result;
	}

	readUint8() {
		if (this.length < 1) return;
		var view = new DataView(this.buffer);
		var result = view.getUint8(0);
		this.skip(1);
		return result;
	}

	readUint16(byteOrder) {
		if (this.length < 2) return;
		var view = new DataView(this.buffer);
		var result = view.getUint16(0, byteOrderToBeUsed(this, byteOrder));
		this.skip(2);
		return result;
	}

	readUint32(byteOrder) {
		if (this.length < 4) return;
		var view = new DataView(this.buffer);
		var result = view.getUint32(0, byteOrderToBeUsed(this, byteOrder));
		this.skip(4);
		return result;
	}

	readFloat32(byteOrder) {
		if (this.length < 4) return;
		var view = new DataView(this.buffer);
		var result = view.getFloat32(0, byteOrderToBeUsed(this, byteOrder));
		this.skip(4);
		return result;
	}

	readFloat64(byteOrder) {
		if (this.length < 8) return;
		var view = new DataView(this.buffer);
		var result = view.getFloat64(0, byteOrderToBeUsed(this, byteOrder));
		this.skip(8);
		return result;
	}

	readLine(eol = "\r\n") {
		let eolCount = eol.length;
		var str = "";
		var pos = 0;

		while (str.substring(str.length - eolCount) != eol) {
			var bytes = this.peekCharacterBytes(pos);
			if (bytes === undefined) return;
			str += bytesToCharacter(bytes);
			pos += bytes.length;
		}

		this.skip(pos);
		return str.substring(0, str.length - eolCount);
	}

	readString(length) {
		var str = "";
		var pos = 0;
		while (length === undefined || length-- > 0) {
			var bytes = this.peekCharacterBytes(pos);
			if (bytes === undefined) {
				if (length === undefined && pos > 0) break;
				return;
			}
			str += bytesToCharacter(bytes);
			pos += bytes.length;
		}

		this.skip(pos);
		return str;
	}

	readByte() {
		if (this.length == 0) return;

		var c = this.peek(0);
		this.skip(1);
		return c;
	}

	readBytes(byteLength) {
		if (this.length < byteLength) return;

		return this.read(byteLength);
	}

	readBytesWhile(matchTest) {
		var pos = 0;
		var length = this.length;

		while (pos < length) {
			var c = this.peek(pos);
			if (!matchTest(c, pos)) {
				return this.read(pos);
			}

			pos += 1;
		}

		return this.read(length);
	}

	readBytesUntil(endTest) {
		var pos = 0;
		var length = this.length;

		while (pos < length) {
			var c = this.peek(pos);
			if (endTest(c, pos)) {
				return this.read(pos);
			}

			pos += 1;
		}
	}

	skipBytes(byteLength) {
		if (this.length < byteLength) return;
		return this.skip(byteLength);
	}

	skipBytesWhile(matchTest) {
		var pos = 0;
		var length = this.length;

		while (pos < length) {
			var c = this.peek(pos);
			if (!matchTest(c, pos)) {
				return this.skip(pos);
			}

			pos += 1;
		}

		return this.skip(length);
	}

	skipBytesUntil(endTest) {
		var pos = 0;
		var length = this.length;

		while (pos < length) {
			var c = this.peek(pos);
			if (endTest(c, pos)) {
				return this.skip(pos);
			}

			pos += 1;
		}
	}
};

Reader.LittleEndian = LittleEndian;
Reader.BigEndian = BigEndian;
Reader.NetworkByteOrder = NetworkByteOrder;
Reader.bytesToCharacter = bytesToCharacter;

/* Writer */

function shortcutWriter(writer, func, value, opt) {
	if (Array.isArray(value)) {
		for (var i = 0; i < value.length; i++) {
			func.call(writer, value[i], opt);
		}
	} else {
		func.call(writer, value, opt);
	}
	return writer;
}

export class Writer {
	constructor(byteOrder = NetworkByteOrder) {
		this.bytes = [];
		this.defaultByteOrder = byteOrder; // NetworkOrder
	}

	get buffer() {
		if (this.bytes.length == 0) return new ArrayBuffer(0);
		return (new Uint8Array(this.bytes)).buffer;
	}

	get length() {
		return this.bytes.length;
	}

	writeString(str) {
		return this.writeBytes(str);
	}

	writeByte(byte) {
		this.bytes.push(byte);
		return this
	}

	writeBytes(bytes) {
		if (typeof bytes === 'string') {
			bytes = ArrayBuffer.fromString(bytes);
		}

		for (var c of new Uint8Array(bytes)) {
			this.bytes.push(c);
		}
		return this
	}

	writeInt8(value) {
		return this.writeByte(value);
	}

	writeInt16(value, byteOrder) {
		var buffer = new ArrayBuffer(2);
		var view = new DataView(buffer);
		view.setInt16(0, value, byteOrderToBeUsed(this, byteOrder));
		return this.writeBytes(buffer);
	}

	writeInt32(value, byteOrder) {
		var buffer = new ArrayBuffer(4);
		var view = new DataView(buffer);
		view.setInt32(0, value, byteOrderToBeUsed(this, byteOrder));
		return this.writeBytes(buffer);
	}

	writeUint8(value) {
		return this.writeByte(value);
	}

	writeUint16(value, byteOrder) {
		var buffer = new ArrayBuffer(2);
		var view = new DataView(buffer);
		view.setUint16(0, value, byteOrderToBeUsed(this, byteOrder));
		return this.writeBytes(buffer);
	}

	writeUint32(value, byteOrder) {
		var buffer = new ArrayBuffer(4);
		var view = new DataView(buffer);
		view.setUint32(0, value, byteOrderToBeUsed(this, byteOrder));
		return this.writeBytes(buffer);
	}

	string(str) {
		return shortcutWriter(this, this.writeBytes, str);
	}

	byte(byte) {
		return shortcutWriter(this, this.writeByte, byte);
	}

	int8(value) {
		return shortcutWriter(this, this.writeByte, value);
	}

	int16(value, byteOrder) {
		return shortcutWriter(this, this.writeInt16, value, byteOrder);
	}

	int32(value, byteOrder) {
		return shortcutWriter(this, this.writeInt32, value, byteOrder);
	}

	uint8(value) {
		return shortcutWriter(this, this.writeByte, value);
	}

	uint16(value, byteOrder) {
		return shortcutWriter(this, this.writeUint16, value, byteOrder);
	}

	uint32(value, byteOrder) {
		return shortcutWriter(this, this.writeUint32, value, byteOrder);
	}

	concat(other) {
		let result = new Writer();
		result.bytes = this.bytes.concat(other.bytes);
		return result;
	}
}

Writer.LittleEndian = LittleEndian;
Writer.BigEndian = BigEndian;
Writer.NetworkByteOrder = NetworkByteOrder;

export default {
	Reader,
	Writer,

	LittleEndian,
	BigEndian,
	NetworkByteOrder,
};

