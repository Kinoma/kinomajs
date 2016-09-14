import {Event, State} from "stream-state";
import {Error} from "error";
import {packUit32} from "endpoint";

export const MaxStreamId = 0x7fff;

export function removePad(payload) {
	const view = new DataView(payload);
	const pad = view.getUint8(0);
	return payload.slice(1, payload.byteLength - pad);
}

export function extractPriority(payload) {
	const view = new DataView(payload);
	let ex, dependentId, weight;

	dependentId = view.getUint32(0);
	ex = (dependentId & 0x8000) >> 31;
	dependentId = dependentId & 0x7fff;
	weight = view.getUint8(4) + 1;

	return [ex, dependentId, weight];
}

export function headersToHeaderList(headers) {
	let list = [];

	for (let name in headers) {
		const value = headers[name];

		if (Array.isArray(value)) {
			list = list.concat(value.map(val => [name, val]));
		} else {
			list.push([name, value]);
		}
	}

	return list;
}

export function headerListToHeaders(list) {
	const headers = {};

	for (let [name, value] of list) {
		if (name in headers) {
			if (!Array.isArray(headers[name])) {
				headers[name] = [headers[name]];
			}

			headers[name].push(value);
		} else {
			headers[name] = value;
		}
	}

	return headers;
}

export function getUint32(payload) {
	const view = new DataView(payload);
	return view.getUint32(0);
}

export class Stream {
	constructor(endpoint, id) {
		this.endpoint = endpoint;
		this.id = id;
		this.state = State.Idle;

		this.handlingHeaders = null;
	}

	get ready() {
		return this.endpoint.ready;
	}

	send(frame) {
		frame.streamId = this.id;
		this.endpoint.send(frame);
	}

	handleFrame({type, flags, payload}) {
		if (this.handlingHeaders && type != 0x09 || !this.handlingHeaders && type == 0x09) {
			this.endpoint.gotError(Error.PROTOCOL_ERROR);
			return;
		}

		switch (type) {
			case 0x00: /* DATA */ {
				if (flags & 0x08) payload = removePad(payload);

				this.endpoint.gotData(this, payload);

				if (flags & 0x01) {
					this.recvES();
				}
				break;
			}

			case 0x01: /* HEADERS */ {
				if (flags & 0x08) payload = removePad(payload);

				if (flags & 0x20) {
					this.setPriority(...extractPriority(payload));
					payload = payload.slice(5, payload.byteLength);
				}

				this.handleHeaders(flags, payload);
				break;
			}

			case 0x02: /* PRIORITY */ {
				if (payload.byteLength != 5) {
					this.endpoint.gotError(Error.FRAME_SIZE_ERROR, this);
				} else {
					this.setPriority(...extractPriority(payload));
				}
				break;
			}

			case 0x03: /* RST_STREAM */ {
				if (payload.byteLength != 4) {
					this.endpoint.gotError(Error.FRAME_SIZE_ERROR, this);
				} else {
					const err = getUint32(payload);

					this.endpoint.gotError(err, this);
					this.changeState(Event.RecvR);
				}
				break;
			}

			case 0x05: /* PUSH_PROMISE */ {
				if (!this.endpoint.serverPushEnabled) {
					this.endpoint.gotError(Error.PROTOCOL_ERROR);
					return;
				}

				if (flags & 0x08) payload = removePad(payload);

				const stream = this.endpoint.getStream(getUint32(payload) & 0x7fff);
				if (!stream || (stream.id % 2) == 1) {
					this.endpoint.gotError(Error.PROTOCOL_ERROR);
				} else {
					payload = payload.slice(4, payload.byteLength);

					this.handleHeaders(flags & 0x04, payload, stream);
				}
				break;
			}

			case 0x08: /* WINDOW_UPDATE */ {
				break;
			}

			case 0x09: /* CONTINUATION */ {
				this.handleHeaders(flags & 0x04, payload);
				break;
			}

			default: {
				this.endpoint.gotError(Error.PROTOCOL_ERROR);
			}
		}
	}

	setPriority(ex, dependentId, weight) {
		// @TODO
		this.weight = weight;
	}

	updateWindow(value) {
		this.send({type: 0x08, payload: packUit32(value)});
	}

	handleHeaders(flags, payload, stream) {
		let handling = this.handlingHeaders || { list: [], flags, stream };

		const decoded = this.endpoint.decoder.decode(payload);
		if (!decoded) return this.endpoint.gotError(Error.COMPRESSION_ERROR, this);

		handling.list.push(...decoded);

		if (flags & 0x04) {
			stream = handling.stream;
			const headers = headerListToHeaders(handling.list);

			if (stream) {
				stream.changeState(Event.RecvPP);
				this.endpoint.gotPromise(stream, headers);
			} else {
				this.endpoint.gotHeaders(this, headers);

				if (handling.flags & 0x01) {
					this.recvES();
				}
			}

			handling = null;
			this.endpoint.priorityStream = null;
		} else {
			this.endpoint.priorityStream = this;
		}

		this.handlingHeaders = handling;
	}

	// state change

	recvES() {
		this.changeState(Event.RecvES);
		this.endpoint.gotComplete(this);
	}

	changeState(event, caller) {
		const newState = this.state.action(event);

		this.endpoint.streamStateWillChange(this, newState, caller);
		this.state = newState;
		this.endpoint.streamStateChanged(this, caller);
		this.onStateChange(caller);
	}

	onStateChange(caller) {

	}


};

export default Stream;
