<package><program><![CDATA[

class Socket @ "KPR_Socket_destructor" {
	_constructor(options) @ "KPR_Socket_constructor";
	constructor(options={}) {
		let proto = options.proto ? options.proto : Socket.TCP;
		let host = options.host ? '' + options.host : undefined;
		let port = options.port ? parseInt(options.port) : 0;

		if (proto != Socket.TCP && proto != Socket.UDP) throw "invalid proto";
		proto = proto == Socket.TCP ? 1 : 2; // interval enum value

		let secure = options.secure;
		if (secure) {
			if (typeof secure != 'object') {
				secure = {};
			}

			if (secure.protocolVersion) {
				let version;
				switch (secure.protocolVersion.toLowerCase()) {
					case 'tls1.0':
					case 'tls1':
						version = 0x0301;
						break;
					case 'tls1.1':
						version = 0x0302;
						break;
					case 'tls1.2':
						version = 0x0303;
						break;
					default:
						throw "invalid TLS version";
				}
				secure.protocolVersion = version;
			}

			if (secure.applicationLayerProtocolNegotiation) {
				// prepare memory structure for C use. wired but easier for me.
				let names = secure.applicationLayerProtocolNegotiation;
				if (typeof names == 'string') names = [names];
				if (!Array.isArray(names)) throw "Invalid appplication layer protocol list";

				secure.applicationLayerProtocolNegotiation = names.join(':');
			}
		}

		this._constructor(proto, host, port, secure);
	}
	close() @ "KPR_Socket_close";
	send(data, host, port) @ "KPR_Socket_send";
	recv(size, buffer) @ "KPR_Socket_recv";

	get connected() @ "KPR_Socket_get_connected";

	get addr() @ "KPR_Socket_get_addr";
	get port() @ "KPR_Socket_get_port";
	get peerAddr() @ "KPR_Socket_get_peerAddr";
	get peerPort() @ "KPR_Socket_get_peerPort";
	get peer() @ "KPR_Socket_get_peer";

	get bytesWritable() @ "KPR_Socket_get_bytesWritable";
	get bytesAvailable() @ "KPR_Socket_get_bytesAvailable";

	static resolv(host, callback) @ "KPR_Socket_resolv";

	read(cons, n, buf) {
		if ((buf = this.recv(n, buf)) === null)
			return buf;
		if (cons === undefined)
			;
		else if (cons == String) {
			// too bad we have to examine the constructor type just for String
			buf = String.fromArrayBuffer(buf);
		}
		else
			buf = new cons(buf);
		return buf;
	}

	write(...items) {
		for (var item of items) {
			this.send(item);
		}
	}

	// onConnect
	// onClose
	//
	// onMessage, onData

	onMessage(n) {
		const data = this.recv(n);
		this.onData(data);
	}

	onData(data) {

	}

	onError(err) {

	}
};

Socket.TCP = "tcp";
Socket.UDP = "udp";

class ListeningSocket extends Socket {
	constructor(options) @ "KPR_ListeningSocket_constructor";
	accept() @ "KPR_ListeningSocket_accept";

	/*
	// called when server is ready
	onReady() {
	}

	// called when server got a connection request (TCP only)
	onConnect() {
		var sock = this.accept();
		this.clients.push(sock);
	}

	// called when server got a data (UDP only)
	onMessage() {

	}

	// called when server got a data and
	// no onMessage was defined (UDP only)
	onData(data) {

	}

	// called when server got a data and
	// neither of onMessage and onData were defined (UDP only)
	onDataReady(reader) {

	}

	*/
};

KPR.Socket = Socket;
KPR.ListeningSocket = ListeningSocket;

]]></program></package>
