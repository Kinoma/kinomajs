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
import Arith from "arith";

export default class BER {
	constructor(buf) {
		this.i = 0;
		this.a = new Uint8Array(buf || 0);
	};
	getTag() {
		return this.a[this.i++];
	};
	getLength() {
		var len = this.a[this.i++];
		if (len & 0x80) {
			var lenlen = len & ~0x80;
			if (lenlen == 0)
				return -1;	// indefinite length
			len = 0;
			while (--lenlen >= 0)
				len = (len << 8) | this.a[this.i++];
		}
		return len;
	};
	peek() {
		return this.a[this.i];
	};
	skip(n) {
		this.i += n;
	};
	next() {
		var saveIndex = this.i;
		void this.getTag();
		var len = this.getLength();
		this.i += len;
		var res = this.a.slice(saveIndex, this.i);
		return res.buffer;
	};
	getInteger() {
		if (this.getTag() != 2)
			throw new Error("BER: not an integer");
		var len = this.getLength();
		var ai = new Arith.Integer(this.a.slice(this.i, this.i + len).buffer);
		this.i += len;
		return ai;
	};
	getBitString() {
		if (this.getTag() != 3)
			throw new Error("BER: not a bit string");
		var len = this.getLength();
		var pad = this.a[this.i++];
		if (pad == 0) {
			var bs = this.a.slice(this.i, this.i + len - 1);
			this.i += len;
		}
		else {
			var bs = new Uint8Array(len - 1);
			for (var i = 0; i < len - 1; i++)
				bs[i] = this.a[this.i++] >>> pad;
		}
		return bs.buffer;
	};
	getOctetString() {
		if (this.getTag() != 0x04)
			throw new Error("BER: not a ocret string");
		var len = this.getLength();
		return this.getChunk(len);
	};
	getObjectIdentifier() {
		if (this.getTag() != 0x06)
			throw new Error("BER: not an object identifier");
		var len = this.getLength();
		return this._getObjectIdentifier(len)
	}
	_getObjectIdentifier(len) {
		var oid = [];
		var i = this.a[this.i++];
		var rem = i % 40;
		oid.push((i - rem) / 40);
		oid.push(rem);
		--len;
		while (len > 0) {
			var v = 0;
			while (--len >= 0 && (i = this.a[this.i++]) >= 0x80)
				v = (v << 7) | (i & 0x7f);
			oid.push((v << 7) | i);
		}
		return oid;
	};
	getSequence() {
		if (this.getTag() != 0x30)
			throw new Error("BER: not a sequence");
		var len = this.getLength();
		var seq = this.a.slice(this.i, this.i + len);
		this.i += len;
		return seq.buffer;
	};
	getChunk(n) {
		var res = this.a.slice(this.i, this.i + n);
		this.i += n;
		return res.buffer;
	};
	getBuffer() {
		return this.a.slice(0, this.i).buffer;
	};

	morebuf(n) {
		if (n < 128) n = 128;
		var nbuf = new Uint8Array(this.a.length + n);
		nbuf.set(this.a);
		this.a = nbuf;
	};
	putc(c) {
		if (this.i >= this.a.length)
			this.morebuf(1);
		this.a[this.i++] = c;
	};
	putTag(tag) {
		this.putc(tag);
	};
	putLength(len) {
		if (len < 128)
			this.putc(len);
		else {
			var lenlen = 1;
			var x = len;
			while (x >>>= 8)
				lenlen++;
			this.putc(lenlen | 0x80);
			while (--lenlen >= 0)
				this.putc(len >>> (lenlen * 8));
		}
	};
	putChunk(c) {
		if (this.i + c.byteLength > this.a.length)
			this.morebuf(c.byteLength);
		this.a.set(new Uint8Array(c), this.i);
		this.i += a.byteLength;
	};
	static encode(arr) {
		var b = new BER();
		var tag = arr[0];
		var val = arr.length > 1 ? arr[1] : undefined;
		b.putTag(tag);
		switch (tag) {
		case 0x01:	// boolean
			b.putLength(1);
			b.putc(val ? 1 : 0);
			break;
		case 0x02:	// integer
			if (typeof val == "number")
				var c = (new Arith.Integer(val)).toChunk();
			else
				var c = val.toChunk();
			b.putLength(c.byteLength);
			b.putChunk(c);
			break;
		case 0x03:	// bit string
			b.putLength(val.byteLength + 1);
			var pad = val.byteLength * 8 - arr[2];
			b.putc(pad);
			if (pad != 0) {
				for (var i = 0; i < val.byteLength; i++)
					b.putc(val[i] << pad);
			}
			else
				b.putChunk(val);
			break;
		case 0x04:	// octet string
			b.putLength(val.byteLength);
			b.putChunk(val);
			break;
		case 0x05:	// null
			b.putLength(0);
			break;
		case 0x06:	// object identifier
			var t = new BER();
			t.putc(val[0] * 40 + (val.length < 2 ? 0 : val[1]));
			for (var i = 2; i < val.length; i++) {
				var x = val[i];
				var n = 1;
				while (x >>>= 7)
					n++;
				x = val[i];
				while (--n >= 1)
					t.pubc((x >>> (n * 7)) | 0x80);
				t.pubc(x & 0x7f);
			}
			b.putLength(t.i);
			b.putChunk(t.getBuffer());
			break;
		case 0x09:	// real -- not supported
			debugger;
			break;
		case 0x07:	// object descriptor
		case 0x0c:	// UTF8 string
		case 0x12:	// numeric string
		case 0x13:	// printable string
		case 0x14:	// telex string
		case 0x16:	// IA5 string
			var c = ArrayBuffer.fromString(val);
			b.putLength(c.byteLength);
			b.putChunk(c);
			break;
		case 0x17:	// UTC time
		case 0x18:	// generalized time
			var s = (val.getUTCFullYear() - (tag == 0x17 ? 1900: 0)).toString() +
				(val.getUTCMonth()).toString() +
				(val.getUTCDate()).toString() +
				(val.getUTCHours()).toString() +
				(val.getUTCMinutes()).toString() +
				(val.getUTCSeconds()).toString() +
				(tag == 0x18 ? "." + (val.getUTCMiliSedonds()).toString(): "") +
				"Z";
			var c = ArrayBuffer.fromString(s);
			b.putLength(c.byteLength);
			b.putChunk(c);
			break;
		case 0x19:	// graphics string
		case 0x1a:	// ISO64 string
		case 0x1b:	// general string
		case 0x1c:	// universal string
		case 0x1e:	// BMP string
			b.putLength(val.byteLength);
			b.putChunk(val);
			break;
		case 0x30:
		case 0x31:
			var len = 0;
			var seq = [];
			for (var i = 1; i < arr.length; i++) {
				var e = this.encode(arr[i]);
				seq.push(e);
				len += e.byteLength;
			}
			b.putLength(len);
			for (var i = 0; i < seq.length; i++)
				b.putChunk(seq[i]);
			break;
		default:
			if ((tag >> 6) == 2) {
				b.putLength(val.byteLength);
				b.putChunk(val);
			}
			break;
		}
		return b.getBuffer();
	};
	static decode(a) {
		return this._decode(new BER(a));
	}
	static _decode(b) {
		var tag = b.getTag();
		if (tag == 0) {
			// must be 00 terminator
			if (b.getc() != 0)
				throw new Error();
			return null;
		}
		if ((tag & 0x1f) == 0x1f) {
			// extended tag -- just ignore
			while (b.getc() >= 0x80)
				;
		}
		var berlen = b.getLength();
		var res = this.decodeTag(tag, b, berlen);
		if (!(res instanceof Array))
			res = [res];
		res.unshift(tag);
		return res;
	};
	static decodeTag(tag, b, len) {
		var res;
		if ((tag >> 6) == 2) {	// context specific class
			// just get a content
			return b.getChunk(len);
		}
		else if (tag & 0x20) {	// construct type
			var seq = [], r;
			if (len < 0) {
				while (r = this._decode(b))
					seq.push(r);
			}
			else {
				var endOffset = b.i + len;
				while (b.i < endOffset && (r = this._decode(b)))
					seq.push(r);
			}
			return seq;
		}
		// universal class
		if (len < 0)
			throw new Error("BER: no unspecific length");
		switch (tag) {
		case 0x01:	// boolean
			if (len != 1)
				throw new Error();
			res = b.getc() != 0;
			break;
		case 0x02:	// integer
			res = new Arith.Integer(b.getChunk(len));
			break;
		case 0x03:	// bit string
			var pad = b.getc();
			if (pad == 0) {
				res = [b.getChunk(len - 1), (len - 1) * 8];
			}
			else {
				var c = Uint8Array(len - 1);
				for (var i = 0; i < len - 1; i++)
					c[i] = b.getc() >>> pad;
				res = [c, (len - 1) * 8 - pad];
			}
			break;
		case 0x04:	// octet string
			res = b.getChunk(len);
			break;
		case 0x05:	// null
			res = null;
			break;
		case 0x06:
			res = b._getObjectIdentifier(len);
			break;
		case 0x09:	// real -- not supported
			throw new Error("BER: unsupported");
			break;
		case 0x07:	// object descriptor
		case 0x0c:	// UTF8 string
		case 0x12:	// numeric string
		case 0x13:	// printable string
		case 0x14:	// telex string
		case 0x16:	// IA5 string
			res = String.fromArrayBuffer(b.getChunk(len));
			break;
		case 0x17:	// ITC time
		case 0x18:	// generalized time
			var s = String.fromArrayBuffer(b.getChunk(len));
			if (tag == 0x18) {
				var prefix = s.substring(0, 2);
				s = s.substring(2);
			}
			var ymd = /(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d*)[\.]*(\d*)([Z\+\-].*)/.exec(s);
			if (tag == 0x18)
				ymd[1] = prefix + ymd[1];
			else
				ymd[1] = (ymd[1] >= 90) ? "19": "20" + ymd[1];
			var date = Date.UTC(ymd[1], ymd[2] - 1, ymd[3], ymd[4], ymd[5], ymd[6], ymd[7]);
			if (ymd[8] && ymd[8] != "Z") {
				var ms = /(\d\d)(\d\d)/.exec(ymd[8]);
				var dif = Date.setUTCHours(ms[1], ms[2]);
				date -= dif;
			}
			res = date;
			break;
		case 0x19:	// graphics string
		case 0x1a:	// ISO64 string
		case 0x1b:	// general string
		case 0x1c:	// universal string
		case 0x1e:	// BMP string
			res = b.getChunk(len);
			break;
		case 0x1f:	// extended tag
			// just return the content
			res = b.getChunk(len);
			break;
		}
		return res;
	};
};
