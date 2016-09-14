import {Endpoint, Mode} from 'endpoint';
import {Socket} from 'socket';
import {State, Event} from "stream-state";
import {Settings} from 'frame';

function endpointKey(host, port, scheme) {
	return scheme + '://' + host + ':' + port;
}

export class Client {
	constructor() {
		this.connections = {};
		this.Connection = ClientConnection;
		this.Transport = Socket;

		this.useUpgrade = true;
	}

	get(url, additionalHeaders={}) {
		return this.send('GET', url, additionalHeaders);
	}

	post(url, data, additionalHeaders={}, keepOpen=false) {
		return this.send('POST', url, additionalHeaders, data, keepOpen);
	}

	send(method, url, additionalHeaders={}, data=null, keepOpen=false) {
		const parts = require('utils/net').parseUrl(url);
		const headers = {
			':method': method,
			':path': parts.path ? parts.path : '/',
			':authority': parts.host,
			':scheme': parts.scheme,
		};

		for (let key in additionalHeaders) {
			headers[key] = additionalHeaders[key];
		}

		if (typeof data == 'string') {
			data = ArrayBuffer.fromString(data);
		}

		const stream = this.createStream(parts.host, parts.port, parts.scheme);
		const endpoint = stream.endpoint;
		const request = {stream, headers, data, keepOpen};

		if (stream.ready) {
			endpoint.sendRequest(request);
		} else {
			endpoint.pending.push(request);
		}

		return stream;
	}

	upload(stream, data, endStream=false) {
		if (typeof data == 'string') {
			data = ArrayBuffer.fromString(data);
		}

		if (stream.ready) {
			stream.endpoint.sendData(stream, data, endStream);
		} else {
			throw new Error('HTTP/2: stream is not ready');
		}
	}

	getConnection(url) {
		const parts = require('utils/net').parseUrl(url);
		return this.findConnection(parts.host, parts.port, parts.scheme);
	}

	createStream(host, port, scheme) {
		const endpoint = this.findConnection(host, port, scheme) || this.openConnection(host, port, scheme);
		return endpoint.createStream();
	}

	findConnection(host, port, scheme) {
		return this.connections[endpointKey(host, port, scheme)];
	}

	openConnection(host, port, scheme) {
		const endpoint = new this.Connection(scheme, host, port, this);
		this.connections[endpointKey(host, port, scheme)] = endpoint;

		return endpoint;
	}

	// callbacks

	onHeaders(headers, stream) {
	}

	onData(bytes, stream) {
	}

	onTrailingHeaders(headers, stream) {
	}

	onComplete(stream) {
	}

	onPush(requestHeaders, stream) {
	}

	onGoAway(stream, err, payload) {
	}

	onError(err, stream) {
	}
};

export class ClientConnection extends Endpoint {
	constructor(scheme, host, port, client) {
		super(1);

		this.scheme = scheme;
		this.peer = host + ':' + port;
		this.client = client;

		const secure = scheme == 'http' ? false : {
			protocolVersion: 'tls1.2',
			serverName: host,
			applicationLayerProtocolNegotiation: ['h2', 'http/1.1'],
		};

		const transport = new client.Transport({proto:Socket.TCP, host, port, secure});
		transport.onConnect = () => {
			delete this.connecting;

			this.transport = transport;
			this.negotiate();
		};
		this.connecting = transport;
		this.negotiated = false;
		this.pending = [];
	}

	negotiate() {
		this.maxIncomingFrameSize = 16777215 /* 0xffffff */;

		this.mode = Mode.NegotiationDirect;
		this.transport.send("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");
		this.sendSettings([
			[Settings.SETTINGS_MAX_FRAME_SIZE, this.maxIncomingFrameSize],
		]);
	}

	start() {
		for (let request of this.pending) {
			this.sendRequest(request);
		}
	}

	sendRequest({stream, headers, data, keepOpen}) {
		this.sendHeaders(stream, headers, !data && !keepOpen);
		if (data) this.sendData(stream, data, !keepOpen);
		stream.updateWindow(0x7fffffff - 65535);
	}

	gotHeaders(stream, headers) {
		this.client.onHeaders(headers, stream);
	}

	gotData(stream, payload) {
		this.client.onData(payload, stream);
		if (payload.byteLength > 0) {
			this.updateWindow(payload.byteLength);
			stream.updateWindow(payload.byteLength);
		}
	}

	gotTrailingHeaders(stream, headers) {
		this.client.onTrailingHeaders(headers, stream);
	}

	gotComplete(stream) {
		this.client.onComplete(stream);
	}

	gotPromise(stream, headers) {
		this.client.onPush(headers, stream);
	}

	gotGoAway(stream, err, payload) {
		this.client.onGoAway(stream, err, payload);
	}

	gotError(err, stream=null) {
		this.client.onError(err, stream);
	}

	onTransportData(bytes) {
		this.feed(bytes);
	}

	onTransportClose() {
	}

	onTransportError() {
	}
};

export default Client;
