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

import {Socket} from "socket";

const MDNS_HEADER_SIZE = 12;

const DNS_RRTYPE_A = 1;		// a host address
const DNS_RRTYPE_PTR = 12;
const DNS_RRTYPE_TXT = 16;
const DNS_RRTYPE_SRV = 33;
const DNS_RRTYPE_ANY = 255;
const DNS_RRCLASS_IN = 1;	// the Internet
const DNS_CLASS_MASK = 0x7fff;
const DNS_QU_MASK = 0x8000;
const DNS_CACHE_FLUSH_MASK = 0x8000;
const DNS_DEFAULT_TTL = 255;

class Buffer {
	constructor(a) {
		if (a === undefined)
			a = 128;
		if (typeof a == "number")
			this.data = new DataView(new ArrayBuffer(a));
		else
			this.data = new DataView(a);
		this.i = 0;
	};
	getc() {
		return this.data.getUint8(this.i++);
	};
	getw() {
		let w = this.data.getUint16(this.i);
		this.i += 2;
		return w;
	};
	getl() {
		let l = this.data.getUint32(this.i);
		this.i += 4;
		return l;
	};
	gets(n) {
		let a = this.data.buffer;
		let ret = a.slice(this.i, this.i + n);
		this.i += n;
		return ret;
	};
	putc(c) {
		if (this.i >= this.data.byteLength)
			this.morebuf(1);
		this.data.setUint8(this.i++, c);
	};
	putw(w) {
		if (this.i + 2 > this.data.byteLength)
			this.morebuf(2);
		this.data.setUint16(this.i, w);
		this.i += 2;
	};
	putl(l) {
		if (this.i + 4 > this.data.byteLength)
			this.morebuf(4);
		this.data.setUint32(this.i, l);
		this.i += 4;
	};
	puts(s) {
		if (typeof s == "string")
			s = ArrayBuffer.fromString(s);
		if (this.i + s.byteLength > this.data.byteLength)
			this.morebuf(s.byteLength);
		let a = new Uint8Array(this.data.buffer);
		a.set(new Uint8Array(s), this.i);
		this.i += s.byteLength;
	};
	get buffer() {
		var buf = this.data.buffer;
		return buf.byteLength == this.i ? buf : buf.slice(0, this.i);
	};
	seek(offset) {
		let save = this.i;
		this.i = offset;
		return save;
	};
	morebuf(n) {
		if (n < 64) n = 64;
		var nbuf = new Uint8Array(this.data.byteLength + n);
		nbuf.set(new Uint8Array(this.data.buffer));
		this.data = new DataView(nbuf.buffer);
	};
};

var message = {
	query(id, name, unicast = true, any = false) {
		let buf = new Buffer(MDNS_HEADER_SIZE + name.length + 2 + 4);
		buf.putw(id);
		buf.putc(0x01);	// QR=0, OPCODE=0, AA=0, TC=0, RD=1
		buf.putc(0);	// RA=0, Z=0, RCODE=0
		buf.putw(1);	// QDCOUNT=1
		buf.putw(0);	// ANCOUNT=0
		buf.putw(0);	// NSCOUNT=0
		buf.putw(0);	// ARCOUNT=0
		this.putString(buf, name);	// QNAME
		buf.putw(any ? DNS_RRTYPE_ANY : DNS_RRTYPE_A);		// QTYPE = host
		buf.putw(DNS_RRCLASS_IN | (unicast ? DNS_QU_MASK : 0));	// QCLASS = Internet
		return buf.buffer;
 	},
	probe(services, unicast) {
		let buf = new Buffer();
		buf.putw(0);
		buf.putc(0);	// QR=0, OPCODE=0, AA=0, TC=0, RD=0
		buf.putc(0);	// RA=0, Z=0, RCODE=0
		buf.putw(services.length);	// QDCOUNT
		buf.putw(0);	// ANCOUNT
		buf.putw(services.length);	// NSCOUNT
		buf.putw(0);	// ARCOUNT
		// queries
		for (let svc of services) {
			this.putString(buf, svc.name);
			buf.putw(DNS_RRTYPE_ANY);
			buf.putw(DNS_RRCLASS_IN | (unicast ? DNS_QU_MASK : 0));
		}
		// authoritative NS
		this.writeRR(buf, services);
		return buf.buffer;
	},
	announce(services) {
		let buf = new Buffer();
		buf.putw(0);
		buf.putc(0x84);	// standard query response
		buf.putc(0);
		buf.putw(0);
		buf.putw(services.length);
		buf.putw(0);
		buf.putw(0);
		// answers
		this.writeRR(buf, services);
		return buf.buffer;
	},
	parse(packet) {
		let buf = new Buffer(packet);
		let msg = {};
		msg.id = buf.getw();
		msg.opcode = (buf.getc() >> 3) & 0xf;
		msg.rcode = buf.getc() & 0xf;
		let qdcount = buf.getw();
		let ancount = buf.getw();
		let nscount = buf.getw();
		let arcount = buf.getw();
		msg.qds = [];
		for (let i = 0; i < qdcount; i++) {
			let name = this.getString(buf);
			let qtype = buf.getw();
			let qclass = buf.getw();
			msg.qds.push({name, qtype, qclass: qclass & ~DNS_QU_MASK, unicast: qclass & DNS_QU_MASK});
		}
		msg.ans = this.readRR(buf, ancount);
		msg.nss = this.readRR(buf, nscount);
		msg.ars = this.readRR(buf, arcount);
		return msg;
	},
	readRR(buf, count) {
		var res = [];
		while (--count >= 0) {
			let rr = {};
			rr.name = this.getString(buf);
			rr.type = buf.getw();
			rr.cls = buf.getw();
			rr.ttl = buf.getl();
			let len = buf.getw();
			if ((rr.cls & DNS_CLASS_MASK) != DNS_RRCLASS_IN) {	// only care about the Internet
				buf.gets(len);
				continue;
			}
			switch (rr.type) {
			case DNS_RRTYPE_A:
				rr.addr = Socket.ntoa(buf.gets(len));
				break;
			case DNS_RRTYPE_PTR:
				rr.domain = this.getString(buf);
				break;
			case DNS_RRTYPE_SRV:
				let priority = buf.getw();
				let weight = buf.getw();
				rr.port = buf.getw();
				rr.target = this.getString(buf);
				break;
			case DNS_RRTYPE_TXT:
				let txt = {};
				while (len > 0) {
					let n = buf.getc();
					let s = String.fromArrayBuffer(buf.gets(n));
					let a = s.split('=');
					if (a.length > 1)
						txt[a[0]] = a[1];
					len -= n + 1;
				}
				rr.txt = txt;
				break;
			default:
				buf.gets(len);
				break;
			}
			res.push(rr);
		}
		return res;
	},
	writeRR(buf, services) {
		for (let svc of services) {
			this.putString(buf, svc.name);
			buf.putw(svc.type);
			buf.putw(DNS_RRCLASS_IN | (svc.flush ? DNS_CACHE_FLUSH_MASK : 0));	// only the Internet
			buf.putl(svc.ttl === undefined ? DNS_DEFAULT_TTL : svc.ttl);
			switch (svc.type) {
			case DNS_RRTYPE_A:	// Internet address
				buf.putw(4);
				buf.puts(Socket.aton(svc.addr));
				break;
			case DNS_RRTYPE_PTR:
				buf.putw(svc.domain.length + 2);
				this.putString(buf, svc.domain);
				break;
			case DNS_RRTYPE_SRV:
				// where's the binary format!?
				buf.putw(svc.target.length + 2 + 2*3);
				buf.putw(0);	// priority
				buf.putw(0);	// weight
				buf.putw(svc.port);	// port
				this.putString(buf, svc.target);	// the host name?
				break;
			case DNS_RRTYPE_TXT:
				let len = 0, txt = svc.txt, a = [];
				for (let i in txt) {	// needs the length first
					let s = i + "=" + txt[i];
					len += s.length + 1;
					a.push(s);
				}
				buf.putw(len);
				a.forEach(e => {buf.putc(e.length); buf.puts(e);});
				break;
			default:	// not supported
				buf.putw(0);
				break;
			}
		}
	},
	putString(buf, s) {
		let a = s.split('.');
		for (let i = 0; i < a.length; i++) {
			let ss = a[i];
			buf.putc(ss.length);
			for (let j = 0; j < ss.length; j++)
				buf.putc(ss.charCodeAt(j));
		}
		buf.putc(0);	// terminator
	},
	getString(buf) {
		var c, s = "";
		while ((c = buf.getc()) != 0) {
			if ((c & 0xc0) == 0xc0) {	// compression
				let offset = ((c & ~0xc0) << 8) | buf.getc();
				let saveoffset = buf.seek(offset);
				s += this.getString(buf);
				buf.seek(saveoffset);
				break;
			}
			else {
				while (--c >= 0)
					s += String.fromCharCode(buf.getc());
				s += ".";
			}
		}
		return s;
	},
};

export default message;
