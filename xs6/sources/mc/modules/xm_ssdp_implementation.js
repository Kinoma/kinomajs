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

import {ListeningSocket, Socket} from "socket";

let ssdp = {
	// external interface
	add(description) {
		if (0 == this.description.length)
			this.init();
		this.description.push(description);
		this.announce(description, "ssdp:alive");
	},
	remove(description) {
		let index = this.description.findIndex(item => 
						       (description.DEVICE_TYPE == item.DEVICE_TYPE) &&
						       (description.DEVICE_VERSION == item.DEVICE_VERSION) &&
						       (description.DEVICE_SCHEMA == item.DEVICE_SCHEMA) &&
						       (description.HTTP_PORT == item.HTTP_PORT))
		if (index < 0) return;

		this.announce(description, "ssdp:byebye");
		this.description.splice(index, 1);
		if ((0 == this.description.length) && (undefined === this.packets))
			this.fin();
	},

	// internal
	packet(packet, listener) {
		let header, headers = new Map();

		packet.split("\n").forEach(line => {
			if (undefined === header)
				header = line.trim();
			else {
				let i = line.indexOf(":");
				if (i > 0)
					headers.set(line.slice(0, i).trim().toLowerCase(), line.slice(i + 1).trim());
			}
		});

		if ("M-SEARCH * HTTP/1.1" == header) {
			if (!headers.has("mx") || !headers.has("st"))
				return;
			let delay = parseInt(headers.get("mx"));
			delay = delay > 5 ? 5 : delay;
			if (delay > 1) delay -= 1;

			let type = headers.get("st"), description;
			if ("ssdp:all" == type) {
				let when = Date.now() + Math.random() * delay * 500;
				this.description.forEach(description => {
					let packet = this.makePacket(description, {subtype: undefined, ST: "upnp:rootdevice"}, listener);
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;

					packet = this.makePacket(description, {subtype: undefined, ST: undefined}, listener);
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;

					packet = this.makePacket(description, {subtype: undefined, ST: description.DEVICE_SCHEMA + ":device:" + description.DEVICE_TYPE + ":" + description.DEVICE_VERSION}, listener);
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;
				});
				return;
			}

			this.description.forEach(description => {
				if (("ssdp:rootdevice" == type) || (this.uuid == type) || this.match(description, type)) {
					let when = Date.now() + (Math.random() * 1000);
					let packet = this.makePacket(description, {subtype: undefined, ST: type}, listener);
					this.send(packet, when, listener, listener.peer); when += Math.random() * 100;
					this.send(packet, when, listener, listener.peer);
				}
			});
		}
	},

	init() {
		let Connection = require.weak("wifi");
		let interfaces = Connection.getInterfaces();		//@@ getInterfaces is spinning off symbols. It could return Map instead. Then the loop below could use "of" instead of "in"
		for (let key in interfaces) {
			let i = interfaces[key];

			if (!(i.UP && i.MULTICAST && !i.LOOPBACK && i.addr)) continue;

			let sock = new ListeningSocket({addr: "239.255.255.250", port: 1900, proto: "udp", membership: i.addr, ttl: 2});
			sock.onMessage = this.onMessage.bind(this, sock);

			this.socks.push(sock);
		}

		let TimeInterval = require.weak("timeinterval");
		this.timer = new TimeInterval(this.callback.bind(this), 1);
		this.reschedule(true);

		this.busy(true);
	},
	fin() {
		this.busy(false);

		this.timer.close();
		this.timer = undefined;

//@@ this code can never run... right?
		while (this.description.length)
			this.remove(this.description[0]);

		this.socks.forEach(sock => sock.close());
		this.socks.length = 0;
	},

	reschedule(update) {
		if (update)
			this.update = Date.now() + ((1800 / 2) + (Math.random() * (1800 / 3)) * 1000);

		let delta = this.update - Date.now();
		if (delta < 0) delta = 1;
		this.timer.reschedule(delta);
	},

	announce(description, subtype) {
		(description ? [description] : this.description).forEach(description => {
			let when = Date.now();		// no initial delay needed. alive and byebye are already arbitrarily timed.
			this.socks.forEach(sock => {
				let packet = this.makePacket(description, {subtype: subtype, NT: "upnp:rootdevice"}, sock);
				this.send(packet, when); when += Math.random() * 100;
				this.send(packet, when); when += Math.random() * 100;

				packet = this.makePacket(description, {subtype: subtype, NT: undefined}, sock);
				this.send(packet, when); when += Math.random() * 100;
				this.send(packet, when); when += Math.random() * 100;

				packet = this.makePacket(description, {subtype: subtype, NT: description.DEVICE_SCHEMA + ":device:" + description.DEVICE_TYPE + ":" + description.DEVICE_VERSION}, sock);
				this.send(packet, when); when += Math.random() * 100;
				this.send(packet, when); when += Math.random() * 100;
			});
		});
	},
	makePacket(description, types, sock) {
		let lines = [
			types.subtype ? "NOTIFY * HTTP/1.1" : "HTTP/1.1 200 OK",
			"SERVER: Element/2.3.35, UPnP/1.0, Kinoma/1.0",
			"HOST: " + "239.255.255.250:1900",
		];

		let tag = types.subtype ? "NT" : "ST";
		let uuid = this.uuid;
		lines.push("USN: " + "uuid:" + uuid + (types[tag] ? "::" + types[tag] : ""));
		lines.push(tag + ": " + (types[tag] ? types[tag] : "uuid:" + uuid));

		if (types.subtype)
			 lines.push("NTS: " + types.subtype);

		if (!types.subtype || ("ssdp:alive" == types.subtype)) {
			lines.push("CACHE-CONTROL: max-age=1800");
			lines.push("LOCATION: " + "http://" + sock.addr + ":" + description.HTTP_PORT + description.LOCATION);
		}
		lines.push("");

		return lines.join("\r\n");
	},
	match(description, type) {
		let i = type.lastIndexOf(":")
		return (i > 0) && type.startsWith(description.DEVICE_SCHEMA + ":device:" + description.DEVICE_TYPE) && (parseInt(type.slice(i+1)) <= description.DEVICE_VERSION);
	},
	send(packet, when, listener, peer) {
		let packets = this.packets;
		if (!packets)
			packets = this.packets = [];
		packets.push({packet, when, listener, peer});
		packets.sort((a, b) => {
			if (a.when < b.when) return -1;
			if (a.when > b.when) return +1;
			return 0;
		});
		let delta = packets[0].when - Date.now();
		if (delta <= 0) delta = 1;

		this.timer.reschedule(delta);
	},
	transmit() {
		// allocate response sockets, if needed. if response sockets aren't ready, wait
		if (!this.socks.every(sock => {
			if (sock.response)
				return sock.response.ready;

			sock.response = new Socket({host: "239.255.255.250", port: 1900, proto: "udp", multicast: sock.addr, ttl: 2});
			sock.response.ready = false;
			sock.response.onConnect = function() {
				this.ready = true;
				delete this.onConnect;
			}
		})) {
			this.timer.reschedule(10);
			return true;
		}

		// send next packet out
		let packets = this.packets;
		let item = packets.shift();
		if (item.listener)
			item.listener.response.send(item.packet, item.peer);
		else
			this.socks.forEach(sock => sock.response.send(item.packet, item.peer));

		if (packets.length) {
			// schedule next packet
			let delta = packets[0].when - Date.now();
			if (delta <= 0) delta = 1;
			this.timer.reschedule(delta);
			return true;
		}
		else {
			// no more to send, clean-up
			this.packets = undefined;
			this.socks.forEach(sock => {sock.response.close(); delete sock.response;});
			if (0 === this.description.length)
				this.fin();
		}
	},
	get uuid() {
		let uuid = require.weak("uuid");
		return uuid.get();
	}
};

export default ssdp;
