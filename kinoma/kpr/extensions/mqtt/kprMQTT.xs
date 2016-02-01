<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package><program><![CDATA[

let MQTT = {
	args: {
		mustBeObject(arg) {
			if (!this.isObject(arg)) {
				throw "`options` must be object if it exists";
			}
		},
		isObject(arg) {
			return (typeof arg == 'object') && !(arg instanceof ArrayBuffer);
		},
		isInteger(arg) {
			var value = parseInt(arg);
			return value === arg;
		},
		mustBeStringOrArrayBuffer(arg) {
			if (!(arg instanceof ArrayBuffer)) {
				arg = '' + arg;
			}
			return arg;
		},
		validateQoS(qos) {
			qos = parseInt(qos);
			if (qos > 2 || qos < 0) throw "Invalid QoS";
			return qos;
		},
		validateTopic(topic) {
			if (!this.isValidTopic(topic, false)) throw "Invalid topic";
			return topic;
		},
		validateTopicFilter(filter) {
			if (!this.isValidTopic(filter, true)) throw "Invalid topic filter";
			return filter;
		},
		parseSubscribeArgs(args) {
			var result = [], topics, qos;

			while (args.length > 0) {
				topics = args.shift();
				if (this.isInteger(topics)) throw "No topics before QoS";
				if (!Array.isArray(topics)) topics = [topics];

				if (args.length > 0 && this.isInteger(args[0])) {
					qos = this.validateQoS(args.shift());
				} else {
					qos = 0;
				}

				for (var i = 0; i < topics.length; i++) {
					result.push(this.validateTopicFilter(topics[i]));
					result.push(qos);
				}
			}

			if (result.length == 0) throw "Empty topics";
			return result;
		},
		parseUnsubscribeArgs(args) {
			var result = [], topics;

			while (args.length > 0) {
				topics = args.shift();
				if (this.isInteger(topics)) throw "Invalid topics";
				if (!Array.isArray(topics)) topics = [topics];

				for (var i = 0; i < topics.length; i++) {
					result.push(this.validateTopicFilter(topics[i]));
				}
			}

			if (result.length == 0) throw "Empty topics";
			return result;
		},
		isValidTopic(topic, isFilter) @ "KPR_mqtt_isValidTopic",
	},
	PromiseRepository: class {
		constructor() {
			this._promises = {};
			this._promiseKeys = [];
		}

		create(key) {
			return new Promise((resolve, reject) => {
				this.reject(key, -1);
				this._promises[key] = {resolve, reject};
				this._promiseKeys.push(key);
			});
		}

		currentKey() {
			if (this._promiseKeys.length == 0) return null;
			return this._promiseKeys[0];
		}

		resolve(key, value) {
			var p = this._promises[key];
			if (!p) return;

			p.resolve(value);
			this._invalidate(key);
		}

		reject(key, reason) {
			var p = this._promises[key];
			if (!p) return;

			p.reject(reason);
			this._invalidate(key);
		}

		_invalidate(key) {
			delete this._promises[key];
			var index = this._promiseKeys.indexOf(key);
			if (index >= 0) this._promiseKeys.splice(index, 1);
		}
	}
};

MQTT.Client = class @ "KPR_mqttclient_destructor" {
	constructor(clientId, cleanSession, options) {
		if (MQTT.args.isObject(clientId)) {
			options = clientId;
			clientId = undefined;
			cleanSession = true;
		} else if (MQTT.args.isObject(cleanSession)) {
			options = cleanSession;
			cleanSession = undefined;
		}

		if (clientId) {
			clientId = '' + clientId;
		} else {
			clientId = '';
		}

		if (cleanSession === undefined) {
			cleanSession = !clientId;
		} else {
			cleanSession = !!cleanSession;
		}

		if (options) {
			MQTT.args.mustBeObject(options);
		}

		this._constructor(clientId, cleanSession);

		if (options) {
			for (var key in options) {
				this[key] = options[key];
			}
		}

		this._promise = new MQTT.PromiseRepository();
		this._callbacksHolding = [];
	}

	connect(host, port, options) {
		var secure = false, keepAlive = 60, username, password;
		var will = { topic:null, qos:0, retain:false, data:null }

		if (host) {
			host = '' + host;
		} else {
			host = 'localhost';
		}

		if (port) {
			port = parseInt(port);
			secure = (port == 1884);
		} else {
			port = 1883;
		}

		if (options) {
			MQTT.args.mustBeObject(options);

			secure = !!(options.secure);

			keepAlive = options.keepAlive;
			if (keepAlive === true) {
				keepAlive = 60;
			} else if (keepAlive === false) {
				keepAlive = 0;
			} else {
				keepAlive = parseInt(keepAlive);
			}

			if (options.username) username = '' + options.username;
			if (options.password) password = '' + options.password;

			if (options.will) {
				will = options.will;

				if (!(typeof will == 'object')) {
					throw "`will` argument must be object if it exists";
				}

				var topic, qos = 0, retain = false, data = null;

				if (will.topic) topic = '' + will.topic;
				qos = MQTT.args.validateQoS(will.qos);
				if (will.retain) retain = true;
				if (will.data) {
					data = MQTT.args.mustBeStringOrArrayBuffer(will.data);
				}

				if (!topic || !MQTT.args.isValidTopic(topic, false)) throw "Invalid will topic";

				will = { topic, qos, retain, data };
			}
		}

		this._connect(host, port, secure, keepAlive, username, password, will.topic, will.qos, will.retain, will.data);
		if (this.onConnect) return;
		return this._promise.create('connect');
	}

	reconnect() {
		this._reconnect();
		if (this.onConnect) return;
		return this._promise.create('connect');
	}

	disconnect() {
		this._disconnect();
		// if (this.onDisconnect) return;
		return this._promise.create('disconnect');
	}

	/**
	 Case 1.
		topic, string_or_binary, qos, retain
	 Case 2
		topic, { data: string_or_binary}, qos, retain
	 Case 3
		topic, { data: string_or_binary, qos: 0, retain: true }
	 */
	publish(topic, payload, qos, retain) {
		if (!topic) throw "topic must be specified";

		topic = '' + topic;
		if (!MQTT.args.isValidTopic(topic, false)) throw "Invalid topic";

		if (MQTT.args.isObject(payload)) {
			if ('qos' in payload) qos = payload.qos;
			if ('retain' in payload) retain = payload.retain;
			payload = payload.data;
		}

		if (payload || payload === 0) {
			payload = MQTT.args.mustBeStringOrArrayBuffer(payload);
		}

		qos = MQTT.args.validateQoS(qos);
		retain = !!retain;

		var pid = this._publish(topic, payload, qos, retain);
		if (this.onPublish) return pid;
		return this._promise.create('pub:' + pid);
	}

	/**
	 Simple case:
	   subscribe("hello", 1)
	 topic array with qos
	   subscribe(["hello/world", "hello/again", ...], 1, ...)
	 topic and qos as arguments
	   subscribe("hello/world", 1, "hello/again", ...)
	*/
	subscribe() {
		var args = MQTT.args.parseSubscribeArgs(Array.prototype.slice.call(arguments, 0));

		var pid = this._subscribe.apply(this, args);
		if (this.onSubscribe) return pid;
		return this._promise.create('sub:' + pid);
	}

	unsubscribe(topic) {
		var args = MQTT.args.parseUnsubscribeArgs(Array.prototype.slice.call(arguments, 0));

		var pid = this._unsubscribe.apply(this, args);
		if (this.onUnsubscribe) return pid;
		return this._promise.create('unsub:' + pid);
	}

	// <null name="onConnect"/>
	// <null name="onDisconnect"/>
	// <null name="onMessage"/>
	// <null name="onSubscribe"/>
	// <null name="onUnsubscribe"/>
	// <null name="onPublish"/>
	// <null name="onError"/>

	get protocolVersion() @ "KPR_mqttclient_get_protocolVersion";

	// private stuff
	_constructor(/* string, boolean */) @ "KPR_MQTTClient";
	_connect(/* string, integer, boolean, integer, string?, string?, string?, integer?, boolean?, (string or ArrayBuffer)? */) @ "KPR_mqttclient_connect";
	_reconnect(/* */) @ "KPR_mqttclient_reconnect";
	_disconnect(/* */) @ "KPR_mqttclient_disconnect";
	_publish(/* string, (string or ArrayBuffer)?, integer, boolean */) @ "KPR_mqttclient_publish";
	_subscribe(/* string, integer, ... */) @ "KPR_mqttclient_subscribe";
	_unsubscribe(/* string, ... */) @ "KPR_mqttclient_unsubscribe";

	_onConnect(returnCode, sessionPresent) {
		if (returnCode == 0) {
			this._promise.resolve('connect', sessionPresent);
		} else {
			this._promise.reject('connect', returnCode);
		}

		this._reserveCallback('onConnect', [returnCode, sessionPresent]);
	}

	_onDisconnect(cleanClose) {
		this._promise.resolve('disconnect', cleanClose);

		if (this.onDisconnect) this.onDisconnect(cleanClose);
	}

	_onPublish(pid) {
		this._promise.resolve('pub:' + pid);

		this._reserveCallback('onPublish', [pid]);
	}

	_onSubscribe(pid, result) {
		this._promise.resolve('sub:' + pid, result);

		this._reserveCallback('onSubscribe', [pid, result]);
	}

	_onUnsubscribe(pid) {
		this._promise.resolve('unsub:' + pid);

		this._reserveCallback('onUnsubscribe', [pid]);
	}

	_onMessage(topic, payload) {
		this._reserveCallback('onMessage', [topic, payload]);
	}

	_onError(err, reason) {
		this._reserveCallback('onError', [err, reason]);
	}

	_reserveCallback(name, args) {
		if (this[name]) {
			this._callbacksHolding.push([name, args]);
		}
	}

	_callbackPending() {
		return this._callbacksHolding.length > 0;
	}

	_releaseCallbacks() {
		this._callbacksHolding.forEach(([name, args]) => {
			try {
				this[name].apply(this, args);
			} catch (e) {
			}
		});
		this._callbacksHolding = [];
	}
};

MQTT.Server = MQTT.Broker = class @ "KPR_mqttbroker_destructor" {
	constructor(port) @ "KPR_MQTTBroker";

	get clients() @ "KPR_mqttbroker_get_clients";
	disconnect(clientIdentifier) @ "KPR_mqttbroker_disconnect";

	get port() @ "KPR_mqttbroker_get_port";

	// <null name="onConnect"/>
	// <null name="onDisconnect"/>
	// <null name="onSubscribe"/>
	// <null name="onUnsubscribe"/>
	// <null name="onPublish"/>
	// <null name="onError"/>
};

MQTT.CLOSED   = 0;
MQTT.OPENING  = 1;
MQTT.OPEN     = 2;
MQTT.CLOSING  = 3;

]]></program></package>

