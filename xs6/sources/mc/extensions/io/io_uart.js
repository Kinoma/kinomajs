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

export default class Serial @ "xs_uart_destructor" {
	constructor(port, baud) @ "xs_uart_constructor";
	_close() @ "xs_uart_close";
	_read(maxbytes) @ "xs_uart_read";
	read(cons, n, timeout) {
		var res;
		if (timeout !== undefined)
			var t = (new Date()).getTime() + timeout;
		else
			var t = -1;
		var data = this.buf;
		this.buf = null;
		while (!data || data.byteLength < n) {
			var cnt = data ? (n-data.byteLength) : n;
			var d = this._read(cnt);
			if (d)
				data = data ? data.concat(d) : d;
			if (t >= 0 && (new Date()).getTime() > t){
				break;
			}
		}
		if (data && data.byteLength > n) {
			res = data.slice(0, n);
			this.buf = data.slice(n);
		}
		else {
			res = data;
			this.buf = null;
		}

		if (cons === undefined)
			;
		else if (cons == String)
			res = String.fromArrayBuffer(res);
		else
			res = new cons(res);
		return res
	};
	_write(val) @ "xs_uart_write";
	write(value){
		var count = 0;
		for (var i = 0; i < arguments.length; i++){
			count += this.__write(arguments[i]);
		}
		return count;
	};
	__write(val) {
		switch (typeof val) {
		case "number":
			val = (new Uint8Array([val])).buffer;
			break;
		case "string":
			val = ArrayBuffer.fromString(val);
			break;
		case "object":
			if (val instanceof Array)
				val = (new Uint8Array(val)).buffer;
			break;
		default:
			throw "unsupported type";
			break;
		}
		return this._write(val);
	};
	repeat(cb) {
		if (this.buf) {
			cb(this.buf);
			this.buf = null;
			return;
		}
		var that = this;
		this.timer = setInterval(function() {
			var data = that._read();
			if (data)
				cb(data);
		}, 50);
	};
	close() {
		if (this.timer) {
			clearInterval(this.timer);
			delete this.timer;
		}
		this._close();
	};
};
