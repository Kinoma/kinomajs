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
function BlobReader() {
	this.blobs = [];
	this.remains = 0;
	this.pos = 0;
}

BlobReader.prototype = {
	feed: function(blob) {
		this.blobs.push(new DataView(blob));
		this.remains += blob.byteLength;
	},
	read: function(n) {
		if (this.blobs.length == 0 || this.remains < n) return undefined;

		var result = new Array(n);
		this.remains -= n;
		var p = 0;
		while (p < n && this.blobs.length > 0) {
			var blob = this.blobs[0];
			var pos = this.pos;
			for (var c = blob.byteLength; pos < c && p < n; pos++, p++) {
				result[p] = blob.getUint8(pos);
			}

			if (pos < blob.byteLength) {
				this.pos = pos;
			} else {
				this.blobs.shift();
				this.pos = 0;
			}
		}

		return result;
	},
};

function Encoder() {
	this.bytes = [];
};

Encoder.prototype = {
	pokeInto(blob, offset) {
		var bytes = this.bytes;
		var view = new DataView(blob, offset);
		for (var i = 0, c = bytes.length; i < c; i++) {
			view.setUint8(i, bytes[i]);
		}
	},

	pushByte(b) {
		this.bytes.push(b);
	},

	pushBytes(a) {
		var len = a.length;
		for (var i = 0; i < len; i++) {
			this.pushByte(a[i]);
		}
	},

	pushUInt16(val) {
		this.bytes.push((val >> 8) & 0xff);
		this.bytes.push(val & 0xff);
	},

	pushString(str) {
		if (str.length > 0)
			this.pushBlob(ArrayBuffer.fromString(str));
	},

	pushBlob(blob) {
		if (blob.byteLength > 0)
			this.pushBytes(new Uint8Array(blob));
	},

	get length() {
		return this.bytes.length;
	},

	getBlob() {
		var blob = new ArrayBuffer(this.length);
		this.pokeInto(blob);
		return blob;
	}
};

export default {
	ptr(str) {
		return function(a,b) { return b === undefined ? str.charAt(a) : str.substring(a,b)}
	},
	attr(obj) {
		return function(name, fallback) { return (name in obj) ? obj[name] : fallback; };
	},
	rnd(n, m) {
		var start, size;
		if (m === undefined) {
			start = 0;
			size = n;
		} else {
			start = n;
			size = m - n;
		}
		return start + ((Math.random() * (size + 1)) | 0);
	},
	pow(n, m) {
		var val = 1;
		while (m-- > 0) val *= n;
		return val;
	},
	ref() {
		return new ReferenceKeeper();
	},
	now() {
		return (new Date()).getTime();
	},
	remove(a, e) {
		var pos = a.indexOf(e);
		if (pos >= 0) return a.splice(pos, 1)[0];
	},
	arrayToBlob(bytes) {
		var length = bytes.length;
		var blob = new ArrayBuffer(length);
		var view = new DataView(blob);
		for (var i = 0; i < length; i++) {
			view.setUint8(i, bytes[i]);
		}
		return blob;
	},
	BlobReader,
	Encoder,
};
