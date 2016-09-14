import {Stream, MaxStreamId, headersToHeaderList} from "stream";
import {State, Event} from "stream-state";
import {pack, unpack, peek, Type, Flag, Settings} from "frame";
import {HTTP2Exception, Error as HTTP2Error} from "error";
import {Encoder, Decoder} from 'hpack';


export const Mode = {
	NegotiationUpgrade: 'Upgrade',
	NegotiationDirect: 'Direct',
	NegotiationTLS: 'TLS',
	HTTP1_1: 'http/1.1',
	HTTP2: 'http/2',
};

export const DefaultMaxHeaderTableSize = 4096;
export const InitialWindowSize = 65535;
export const DefaultMaxFrameSize = 16384;
export const MaxFrameSize = 16777215;

export function packSettings(settings) {
	const payload = new ArrayBuffer(settings.length * 6);
	const view = new DataView(payload);
	for (let i = 0; i < settings.length; i++) {
		const [identifier, value] = settings[i];
		view.setUint16(i * 6, identifier);
		view.setUint32(i * 6 + 2, value);
	}
	return payload;
}

export function packUit32(value) {
	const payload = new ArrayBuffer(4);
	const view = new DataView(payload);
	view.setUint32(0, value);
	return payload;
}

export class Endpoint {
	constructor(initialStreamId=1) {
		this.streams = [];
		this.streamId = initialStreamId;
		this.reader = new StreamFrameReader(this);

		this.priorityStream = null;
		this.mode = undefined;
		this.serverPushEnabled = true; // server
		this.maxConcurrentStreams = undefined;
		this.initialWindowSize = InitialWindowSize;
		this.maxIncomingFrameSize = DefaultMaxFrameSize;
		this.maxOutgoingFrameSize = DefaultMaxFrameSize;
		this.sentSettings = [];

		this.encoder = new Encoder();
		this.encoder.setMaxSize(DefaultMaxHeaderTableSize);

		this.decoder = new Decoder();
		this.decoder.setMaxSize(DefaultMaxHeaderTableSize);
		this.decoderMaxHeaderTableSize = DefaultMaxHeaderTableSize;
	}

	get ready() {
		return this.mode == Mode.HTTP2;
	}

	getStream(id) {
		if (id == 0) return null;

		for (let stream of this.streams) {
			if (stream.id == id) return stream;
		}

		return this.createStream(id);
	}

	createStream(id=undefined) {
		if (!this.canCreateStream) throw "Cannot create new stream";

		if (id === undefined) {
			id = this.streamId;
			this.streamId += 2;
		}

		const stream = new Stream(this, id);
		this.streams.push(stream);

		return stream;
	}

	get canCreateStream() {
		return this.streamId <= 0x7fff;
	}

	get canOpenNewStream() {
		if (this._maxConcurrentStreams === undefined) return true;
		return this.activeStreams.length < this._maxConcurrentStreams;
	}

	get activeStreams() {
		return this.streams.filter(s => s.state.active);
	}

	get maxConcurrentStreams() {
		return this._maxConcurrentStreams;
	}

	set maxConcurrentStreams(val) {
		if (val !== undefined) {
			const streams = this.activeStreams;
			if (streams.length >= val) {
				streams.slice(0, (streams.length - val)).forEach(s => this.closeStream(s));
			}
		}

		this._maxConcurrentStreams = val;
	}

	streamStateWillChange(stream, newState, caller) {
		if (!stream.state.active && newState.active) {
			if (!this.canOpenNewStream) throw new Error("Cannot open new stream: max=" + this._maxConcurrentStreams);
		}
	}

	streamStateChanged(stream, caller) {
		if (caller == this) return;
		this.streams.filter(s => s.state == State.Idle && s.id < stream.id).forEach(s => this.closeStream(s));
	}

	closeStream(s) {
		s.state = State.Closed;
		s.onStateChange(this);
	}

	// data handling

	feed(bytes) {
		this.reader.feed(bytes);
	}

	onFeedFrame(frame) {
		const stream = this.getStream(frame.streamId);

		if (this.priorityStream && this.priorityStream != stream) {
			return this.gotError(HTTP2Error.PROTOCOL_ERROR);
		}

		this.gotFrame(frame, stream);
	}

	onFeedError(err, streamId) {
		this.gotError(err, this.getStream(streamId));
	}

	gotFrame(frame, stream) {
		try {
			if (stream) {
				stream.handleFrame(frame);
			} else {
				this.handleFrame(frame);
			}
		} catch (e) {
			if (e instanceof HTTP2Exception) {
				this.gotError(e.code);
			} else {
				this.gotError(HTTP2Error.INTERNAL_ERROR);
			}
		}
	}

	// safe to throw exception
	handleFrame({type, flags, payload}) {
		const payloadLength = payload ? payload.byteLength : 0;

		switch (type) {
			case 0x04: /* DATA */ {
				// Settings
				const ack = !!(flags & 0x01);

				// 6.5. payload must multiple of 6
				if ((payloadLength % 6) != 0) {
					return this.gotError(HTTP2Error.FRAME_SIZE_ERROR);
				}
				// 6.5. Ack has no payload
				if (payloadLength > 0 && ack) {
					return this.gotError(HTTP2Error.FRAME_SIZE_ERROR);
				}

				if (payloadLength > 0) {
					const settings = [];
					const view = new DataView(payload);

					for (let offset = 0; offset < view.byteLength; offset += 6)
						settings.push([view.getUint16(offset), view.getUint32(offset + 2)]);

					const err = this.settingsRequest(settings);
					if (err) return this.gotError(err);
				}
				// 6.5.3. settings sync
				if (ack) {
					this.settingsAccepted(this.sentSettings.shift());

					if (this.mode != Mode.HTTP2) {
						this.mode = Mode.HTTP2
						this.start();
					}
				} else {
					this.send({type: 0x04, flags: 0x01});
				}
				break;
			}

			case 0x06: /* PING */ {
				if (payloadLength != 8) {
					this.gotError(HTTP2Error.FRAME_SIZE_ERROR);
				} else if (flags & 0x01) {
					this.pingAck(payload);
				} else {
					this.send({type, flags: 0x01, payload});
				}
				break;
			}

			case 0x07: /* GOAWAY */ {
				if (payloadLength < 8) {
					this.gotError(HTTP2Error.FRAME_SIZE_ERROR);
				} else {
					const view = new DataView(payload);
					const stream = this.getStream(view.getUint32(0));
					const err = view.getUint32(4);
					payload = payload.slice(8, payload.byteLength);

					this.gotGoAway(stream, err, payload);
				}
				break;
			}

			case 0x08: /* WINDOW_UPDATE */ {
				break;
			}

			default: {
				this.gotError(HTTP2Error.PROTOCOL_ERROR);
				break;
			}
		}
	}

	settingsRequest(settings) {
		for (const [name, value] of settings) {
			switch (name) {
				case Settings.SETTINGS_HEADER_TABLE_SIZE: {
					this.decoderMaxHeaderTableSize = value;
					if (this.decoder.maxSize >= value) {
						this.decoder.setMaxSize(value);
					}
					break;
				}

				case Settings.SETTINGS_ENABLE_PUSH: {
					if (value != 0 && value != 1) return HTTP2Error.PROTOCOL_ERROR;
					this.serverPushEnabled = !!value;
					break;
				}

				case Settings.SETTINGS_MAX_CONCURRENT_STREAMS: {
					this.maxConcurrentStreams = value;
					break;
				}

				case Settings.SETTINGS_INITIAL_WINDOW_SIZE: {
					if (value > InitialWindowSize) return HTTP2Error.FLOW_CONTROL_ERROR;
					this.initialWindowSize = value;
					break;
				}

				case Settings.SETTINGS_MAX_FRAME_SIZE: {
					if (value > MaxFrameSize || value < DefaultMaxFrameSize) return HTTP2Error.PROTOCOL_ERROR;
					this.maxOutgoingFrameSize = value;
					break;
				}

				case Settings.SETTINGS_MAX_HEADER_LIST_SIZE:
				default: {
					break;
				}
			}
		}
	}

	sendSettings(settings) {
		this.sentSettings.push(settings);
		this.send({
			type: 0x04,
			payload: packSettings(settings)
		});
	}

	settingsAccepted(settings) {

	}

	sendPing(payload) {
		if (payload.byteLength != 8) {
			this.gotError(HTTP2Error.FRAME_SIZE_ERROR);
		} else {
			this.send({type: 0x06, payload});
		}
	}

	pingAck(payload) {

	}

	updateWindow(value) {
		this.send({type: 0x08, payload: packUit32(value)});
	}

	start() {

	}

	gotHeaders(stream, headers) {

	}

	gotData(stream, payload) {

	}

	gotTrailingHeaders(stream, headers) {

	}

	gotComplete(stream) {

	}

	gotPromise(stream, headers) {

	}

	gotError(err, stream=null) {
	}

	sendHeaders(stream, headers, endStream=true) {
		let flags = Flag.END_HEADERS;
		if (endStream) flags |= Flag.END_STREAM;

		let payload = this.encodeHeaders(headers);

		stream.send({type: Type.HEADERS, flags, payload});
		stream.changeState(Event.SendH, this);
		if (endStream) stream.changeState(Event.SendES, this);
	}

	sendData(stream, payload, endStream=true) {
		let flags = 0;

		while (payload.byteLength > this.maxOutgoingFrameSize) {
			let sub = payload.slice(0, this.maxOutgoingFrameSize);
			stream.send({type: Type.DATA, flags, payload: sub});
			payload = payload.slice(this.maxOutgoingFrameSize);
		}

		if (endStream) flags |= Flag.END_STREAM;

		stream.send({type: Type.DATA, flags, payload});
		if (endStream) stream.changeState(Event.SendES, this);
	}

	// header compression

	encodeHeaders(headers) {
		return this.encoder.encode(headersToHeaderList(headers));
	}

	decodeHeaders(bytes) {
		return this.decoder.decode(bytes);
	}

	// transport communication

	get transport() {
		return this._transport;
	}

	set transport(transport) {
		transport.onData = (bytes) => { this.onTransportData(bytes); }
		transport.onClose = () => { this.onTransportClose(); }
		transport.onError = () => { this.onTransportError(); }
		this._transport = transport;
	}

	send(frame) {
		this.transport.send(pack(frame));
	}

	// The callbacks are defined by subclasses

	onTransportData(bytes) {
		this.feed(bytes);
	}

	onTransportClose() {

	}

	onTransportError() {

	}
};

class StreamFrameReader {
	constructor(endpoint) {
		this.endpoint = endpoint;

		this.bytes = new ArrayBuffer(0);
		this.skipBytes = 0;
	}

	feed(bytes) {
		this.bytes = this.bytes.concat(bytes);
		const endpoint = this.endpoint;

		while (true) {
			let frame;

			if (this.skipBytes > 0) {
				if (this.bytes.byteLength <= this.skipBytes) {
					this.skipBytes -= this.bytes.byteLength;
					this.bytes = new ArrayBuffer(0);
					break;
				} else {
					this.bytes = this.bytes.slice(this.skipBytes);
					this.skipBytes = 0;
				}
			}

			frame = peek(this.bytes);
			if (!frame) break;

			if (frame.length > endpoint.maxIncomingFrameSize) {
				endpoint.onFeedError(HTTP2Error.FRAME_SIZE_ERROR, frame.streamId);
				this.skipBytes += frame.length + 9;
				continue;
			}


			[frame, this.bytes] = unpack(this.bytes);
			if (!frame) break;

			endpoint.onFeedFrame(frame);
		}
	}
}

export default Endpoint;
