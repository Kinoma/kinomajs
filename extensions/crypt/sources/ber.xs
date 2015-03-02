<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<package>
	<patch prototype="Crypt">
		<object name="ber"/>
	</patch>

	<patch prototype="Crypt.ber">
		<object name="error" prototype="Error.prototype">
		</object>
		<function name="Error" params="" prototype="Crypt.ber.error" script="false">
		</function>

		<function name="oideq" params="a1, a2, n">
			if (n == undefined) {
				if (a1.length != a2.length)
					return(false);
				n = a1.length;
			}
			for (var i = 0; i < n; i++) {
				if (a1[i] != a2[i])
					return(false);
			}
			return(true);
		</function>

		<object name="binStream" script="false">
			<chunk name="bc" script="false"/>
			<null name="iter" script="false"/>
			<number name="_offset" value="0" script="false"/>

			<function name="getc" script="false">
				if (this.iter)
					return(this.iter.getc());
				else
					return(this.bc.peek(this._offset++));
			</function>

			<function name="getChunk" params="len" script="false">
				if (this.iter)
					return(this.iter.getChunk(len));
				else {
					var r = this.bc.slice(this._offset, this._offset + len);
					this._offset += len;
					return(r);
				}
			</function>

			<function name="putc" params="c" script="false">
				// this is very inefficient -- to be in C
				var i = this.bc.length++;
				this.bc.poke(i, c & 0xff);
			</function>

			<function name="putChunk" params="b" script="false">
				this.bc.append(b);
			</function>

			<function name="free" script="false">
				this.bc.free();
			</function>

			<function name="get length" script="false">
				return(this.bc.length);
			</function>

			<function name="get offset" script="false">
				if (this.iter)
					return(this.iter.getIdx());
				else
					return(this._offset);
			</function>

			<function name="set offset" params="oft" script="false">
				if (this.iter)
					this.iter.setIdx(oft);
				else
					this._offset = oft;
			</function>
		</object>
		<function name="BinStream" params="bc" prototype="Crypt.ber.binStream">
			if (!bc)
				bc = new Chunk();
			else {
				if ("getIter" in bc)
					this.iter = bc.getIter();
			}
			this.bc = bc;
			this._offset = 0;
		</function>

		<function name="putBerLen" params="b, len">
			if (len < 128)
				b.putc(len);
			else {
				var lenLen = 1;
				var x = len;
				while (x >>>= 8)
					lenLen++;
				b.putc(lenLen | 0x80);
				while (--lenLen >= 0)
					b.putc(len >>> (lenLen * 8));
			}
		</function>

		<function name="encode" params="a, narrow, indefinite">
			var b = new this.BinStream();
			var tag = a[0];
			var val = a.length > 1 ? a[1]: undefined;
			b.putc(tag);
			switch (tag) {
			case 0x01:	// boolean
				this.putBerLen(b, 1);
				b.putc(val ? 1: 0);
				break;
			case 0x02:	// integer
				if (typeof val == "number")
					var c = (new Arith.Integer(val)).toChunk();
				else if (val instanceof Arith.Integer)
					var c = val.toChunk();
				else
					throw new this.Error();
				this.putBerLen(b, c.length);
				b.putChunk(c);
				c.free();
				break;
			case 0x03:	// bit string
				this.putBerLen(b, val.length + 1);
				var pad = val.length * 8 - a[2];
				b.putc(pad);
				if (pad != 0) {
					for (var i = 0; i < val.length; i++)
						b.putc(val.peek(i) << pad);
				}
				else
					b.putChunk(val);
				break;
			case 0x04:	// octet string
				this.putBerLen(b, val.length);
				b.putChunk(val);
				break;
			case 0x05:	// null
				this.putBerLen(b, 0);
				break;
			case 0x06:	// object identifier
				var c = new this.BinStream();
				c.putc(val[0] * 40 + ((val.length < 2) ? 0: val[1]));
				for (var i = 2; i < val.length; i++) {
					var x = val[i];
					var n = 1;
					while (x >>>= 7)
						n++;
					x = val[i];
					while (--n >= 1)
						c.putc((x >>> (n * 7)) | 0x80);
					c.putc(x & 0x7f);
				}
				this.putBerLen(b, c.length);
				b.putChunk(c.bc);
				c.free();
				break;
			case 0x09:	// real (not supported yet)
				c.ungetc();
				break;
			case 0x07:	// object descriptor
			case 0x0c:	// UTF8 string
			case 0x12:	// numeric string
			case 0x13:	// printable string
			case 0x14:	// telex string
			case 0x16:	// IA5 string
				var c = new Crypt.bin.chunk.String(val);
				this.putBerLen(b, c.length);
				b.putChunk(c);
				c.free();
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
				var c = new Crypt.bin.chunk.String(val);
				this.putBerLen(b, c.length);
				b.putChunk(c);
				c.free();
				break;
			case 0x19:	// graphics string
			case 0x1a:	// ISO64 string
			case 0x1b:	// general string
			case 0x1c:	// universal string
			case 0x1e:	// BMP string
				this.putBerLen(b, val.length);
				b.putChunk(val);
				break;
			case 0x30:
			case 0x31:
				var len = 0;
				var seq = [];
				for (var i = 1; i < a.length; i++) {
					if (narrow)
						var e = a[i];
					else
						var e = this.encode(a[i], narrow, indefinite);
					seq.push(e);
					len += e.length;
				}
				if (indefinite)
					b.putc(0x80);
				else
					this.putBerLen(b, len);
				for (var i = 0; i < seq.length; i++)
					b.putChunk(seq[i]);
				if (indefinite) {
					b.putc(0x00); b.putc(0x00);
				}
				break;
			default:
				if ((tag >> 6) == 2) {
					this.putBerLen(b, val.length);
					b.putChunk(val);
				}
				break;
			}
			return(b.bc);
		</function>

		<function name="getBerLen" params="b" script="false">
			var c = b.getc();
			if (c < 0x80)
				return(c);
			else {
				var lenLen = c & ~0x80;
				if (lenLen == 0)
					return(-1);	// indefinite length
				var len = 0;
				for (var i = 0; i < lenLen; i++) {
					c = b.getc();
					len = (len << 8) | c;
				}
				return(len);
			}
		</function>

		<function name="decode" params="bc, narrow">
			if ("getIter" in bc)
				return(this._decodeIter(new this.BinStream(bc), narrow ? 1: -1));
			else
				return(this._decode(new this.BinStream(bc), narrow ? 1: -1));
		</function>

		<function name="decodeTag" params="tag, bc, narrow">
			if ("getIter" in bc)
				return(this._decodeTagIter(tag, new this.BinStream(bc), bc.length, narrow ? 1: -1));
			else
				return(this._decodeTag(tag, new this.BinStream(bc), bc.length, narrow ? 1: -1));
		</function>

		<function name="readUpTo00" params="ibs">
			var tag;
			var obs = new this.BitStream();
			while ((tag = ibs.getc()) != 0) {
				obs.putc(tag);
				if ((tag & 0x1f) == 0x1f) {
					while ((tag = ibs.getc()) >= 0x80)
						obs.putc(tag);
				}
				var len = this.getBerLen(ibs);
				this.putBerLen(obs, len);
				if (len < 0)
					var c = this.readUpTo00(ibs);
				else
					var c = ibs.getChunk(len);
				obs.putChunk(c);
				c.free();
			}
			if (ibs.getc() != 0)
				throw new this.Error();	// must be 00
			return(obs.bc);
		</function>

		<function name="_decode" params="b, level">
			b.startOffset = b.offset;	// save the start position of a BER block
			var tag = b.getc();
			if (tag == 0) {
				// must be 00 terminator
				if (b.getc() != 0)
					throw new this.Error();
				return(null);
			}
			if ((tag & 0x1f) == 0x1f) {
				// extended tag -- just ignore
				while (b.getc() >= 0x80)
					;
			}
			var berLen = b.getc();
			if (berLen >= 0x80) {
				berLen = berLen & ~0x80;
				if (berLen == 0) {
					berLen = -1;	// indefinite length
				} else {
					var c = 0;
					var len = 0;
					for (var i = 0; i < berLen; i++) {
						c = b.getc();
						len = (len << 8) | c;
					}
					berLen = len;
				}
			}
			var res = this._decodeTag(tag, b, berLen, level);
			if (!(res instanceof Array))
				res = [res];
			res.unshift(tag);
			return(res);
		</function>

		<function name="_decodeTag" params="tag, b, berLen, level">
			var res;
			switch (tag) {
			// universal class
			case 0x01:	// boolean
				if (berLen != 1)
					throw new this.Error();
				res = b.getc() != 0;
				break;
			case 0x02:	// integer
				if (berLen <= 0)
					throw new this.Error();
				res = new Arith.Integer(b.getChunk(berLen));
				break;
			case 0x03:	// bit string
				if (berLen < 0)
					throw new this.Error();
				var pad = b.getc();
				if(pad == 0){
					res = [b.getChunk(berLen - 1), (berLen - 1) * 8];
				} else {
					var c = new Chunk(berLen - 1);
					for (var i = 0; i < berLen - 1; i++)
						c.poke(i, b.getc() >>> pad);
					res = [c, (berLen - 1) * 8 - pad];
				}
				break;
			case 0x04:	// octet string
				if (berLen < 0)
					throw new this.Error();
				res = b.getChunk(berLen);
				break;
			case 0x05:	// null
				if (berLen != 0)
					throw new this.Error();
				res = null;
				break;
			case 0x06:	// object identifier
				if (berLen <= 0)
					throw new this.Error();
				var n = berLen;
				var oid = [];
				var i = b.getc();
				var rem = i % 40;
				oid.push((i - rem) / 40);
				oid.push(rem);
				--n;
				while (n > 0) {
					var v = 0;
					while (--n >= 0 && (i = b.getc()) >= 0x80)
						v = (v << 7) | (i & 0x7f);
					oid.push((v << 7) | i);
				}
				res = [oid];
				break;
			case 0x09:	// real (not supported yet)
				throw new this.Error();
				break;
			case 0x07:	// object descriptor
			case 0x0c:	// UTF8 string
			case 0x12:	// numeric string
			case 0x13:	// printable string
			case 0x14:	// telex string
			case 0x16:	// IA5 string
				if (berLen < 0)
					throw new this.Error();
				res = (b.getChunk(berLen)).toRawString();
				break;
			case 0x17:	// ITC time
			case 0x18:	// generalized time
				if (berLen < 0)
					throw new this.Error();
				var s = (b.getChunk(berLen)).toRawString();
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
				if (berLen < 0)
					throw new this.Error();
				res = b.getChunk(berLen);
				break;
			case 0x1f:	// extended tag
				// just return the content
				if (berLen < 0)
					throw new this.Error();
				res = b.getChunk(berLen);
				break;
			default:
				// first, check to see if the class is context specific anyway and return the content as a binary chunk
				if ((tag >> 6) == 2) {	// context specific class
					// just get a content
					if (berLen < 0)
						res = readUpTo00(b);
					else
						res = b.getChunk(berLen);
				}
				else if (tag & 0x20) {	// construct type
					if (level == 0) {
						// return the whole block including tag, berLen
						if (berLen < 0)
							readUpTo00(b);
						else
							b.offset += berLen;
						res = b.bc.slice(b.startOffset, b.offset);
						break;
					}
					var seq = [], r;
					--level;
					if (berLen < 0) {
						while (r = this._decode(b, level))
							seq.push(r);
					}
					else {
						var endOffset = b.offset + berLen;
						while (b.offset < endOffset && (r = this._decode(b, level)))
							seq.push(r);
					}
					res = seq;
				}
				else
					// unknown tag or unsupported class
					throw new this.Error();
				break;
			}
			return(res);
		</function>		
	</patch>
</package>