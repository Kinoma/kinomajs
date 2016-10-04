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

const DEFAULT_QUERY_COUNT = 50; // for Kinoma Create

const DEFAULT_RETRY_DELAY = 300000; // 5 minutes

export default class PubNub {
	constructor(params) {
		let pub_key = params.publishKey;
		let sub_key = params.subscribeKey;
		let scheme = params.ssl ? "https" : "http";
		this.delay = params.delay || DEFAULT_RETRY_DELAY;
		Object.assign(REST_API.config, { pub_key, sub_key, scheme });
		this.connected = 0;
		this.listeners = [];
		this.reconnect = false;
		this.remembers = [];
		this.secure = !!params.ssl;
		this.timetoken = null;
	}
	addListener(listener, scope = null) {
		this.listeners.push(listener);
		listener.scope = scope;
	}
	announceMessage(event) {
		this.listeners.forEach(listener => {
			if (listener.message) listener.message.call(listener.scope, event);
		});
	}
	announceStatus(event) {
		this.listeners.forEach(listener => {
			if (listener.status) listener.status.call(listener.scope, event);
		});
	}
	forget(it) {
		let index = this.remembers.indexOf(it);
		if (index !== -1) this.remembers.splice(index, 1);
		//trace("-- remembers " + this.remembers.length + "\n");
		return null;
	}
	remember(it) {
		let index = this.remembers.indexOf(it);
		if (index === -1) this.remembers.unshift(it);
		//trace("-- remembers " + this.remembers.length + "\n");
		return it;
	}
	removeListener(deprecatedListener) {
		this.listeners = this.listeners.filter(listener => {
			return listener !== deprecatedListener;
		});
	}
	// API
	history(params, callback, scope = null) {
		let it = REST_API.fetchHistory(params, (error, data) => {
			if (callback && (typeof callback === "function")) {
				callback.call(scope, error, data);
			}
			else if (("callback" in params) && (typeof params.callback === "function")) {
				params.callback.call(params, error, data);
			}
			else debugger; // callback required!
			this.forget(it);
		}, this);
	}
	publish(params, callback, scope = null) {
		let it = REST_API.publish(params, (error, data) => {
			if (callback && (typeof callback === "function")) {
				callback.call(scope, error, data);
			}
			else if (("callback" in params) && (typeof params.callback === "function")) {
				params.callback.call(params, error, data);
			}
			else debugger; // callback required (at least check for errors)!
			this.forget(it);
		}, this);
	}
	subscribe(params, callback, scope = null) {
		if (callback && (typeof callback === "function")) {
			// later
		}
		else if (("callback" in params) && (typeof params.callback === "function")) {
			// later
		}
		else {
			let message = (("message" in params) && (typeof params.message === "function")) ? params.message : null;
			let status = (("status" in params) && (typeof params.status === "function")) ? params.status : null;
			if (message || status) {
				this.addListener({ message, status }, params);
			}
		}
		let channel = params.channel = params.channel || params.channels[0]; // multiple channels not supported!
		let timetoken = params.timetoken = params.timetoken || this.timetoken;
		let it = REST_API.subscribe(params, (error, data) => {
			if (callback && (typeof callback === "function")) {
				callback.call(scope, error, data);
			}
			else if (("callback" in params) && (typeof params.callback === "function")) {
				params.callback.call(params, error, data);
			}
			else if (this.listeners.length > 0) {
				if (error) {
					this.announceNetworkIssues();
					let delay = this.delay;
					if (delay > 0) {
						let pubnub = this;
						setTimeout(param => {
							pubnub.reconnect = true;
							pubnub.subscribe({ channel });
						}, delay, error);
					}
				}
				else if (Array.isArray(data)) {
					timetoken = data[1].toString();
					if (this.connected) {
						if (this.reconnect) {
							this.connected += 1;
							this.reconnect = false;
							this.announceReconnected();
						}
						data[0].forEach(message => {
							this.announceMessage({ actualChannel:null, message, subscribedChannel:channel, timetoken });
						});
					}
					else {
						this.connected += 1;
						this.reconnect = false;
						this.announceConnected();
					}
					this.timetoken = timetoken;
					this.subscribe({ channel, timetoken });
				}
				else {
					this.announceUnknown();
				}
			}
			else debugger; // listener required!
			this.forget(it);
		}, this);
	}
	stop() {
		let remembers = this.remembers;
		let delay = this.delay;
		this.remembers = [];
		this.delay = 0;
		remembers.forEach(message => {
			message.cancel();
		});
		this.delay = delay;
	}
	time(params, callback, scope = null) {
		let it = REST_API.fetchTime(params, (error, data) => {
			if (callback && (typeof callback === "function")) {
				callback.call(scope, error, data);
			}
			else if (("callback" in params) && (typeof params.callback === "function")) {
				params.callback.call(params, error, data);
			}
			else debugger; // callback required!
			this.forget(it);
		}, this);
	}
	// Events
	announceBadRequest() {
		let category = "PNBadRequestCategory";
		this.announceStatus({ category });
	}
	announceConnected() {
		let category = "PNConnectedCategory";
		this.announceStatus({ category });
	}
	announceNetworkIssues() {
		let category = "PNNetworkIssuesCategory";
		this.announceStatus({ category });
	}
	announceReconnected() {
		let category = "PNReconnectedCategory";
		this.announceStatus({ category });
	}
	announceUnknown() {
		let category = "PNUnknownCategory";
		this.announceStatus({ category });
	}
}

// PubNub REST API Documentation
// https://www.pubnub.com/docs/pubnub-rest-api-documentation
// Publish / Subscribe, History (requires Storage & Playback add-on), and PubNub Time.
// Not Supported: Channel Groups, PubNub Access Manager, Presence, Push Notifications.

function makeRequest(params, callback, scope = null) {
	let data = params.data || params.message;
	let requirement = { uuid:params.uuid, pnsdk:params.pnsdk };
	let url = `${params.scheme}://${params.authority}${params.location}`;
	if (!data && params.payload) {
		// Note: PUBLISH V1 VIA GET payload is 'url-encoded JSON'
		url += "/" + encodeURIComponent(JSON.stringify(params.payload));
	}
	if (params.query && Object.keys(params.query).length) {
		url += "?" + serializeQuery(Object.assign({}, params.query, requirement));
	}
	else {
		url += "?" + serializeQuery(requirement);
	}
	//trace(url + "\n");
	let message = scope
				? scope.remember(new Message(url))
				: new Message(url);
	message.timeout = params.timeout;
	if (data) {
		message.method = "POST";
		message.requestText = JSON.stringify(data);
		message.setRequestHeader("Content-Length", message.requestText.length);
		message.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	}
	let promise = message.invoke();
	promise.then(message => {
		if (message.error || (message.status / 100 !== 2)) {
			callback.call(scope, message.error || message.status || "unknown");
		}
		else {
			try {
				callback.call(scope, null, JSON.parse(message.responseText));
			}
			catch (exception) {
				callback.call(scope, exception.message || "JSON.parse: unknown");
			}
		}
	}).catch(message => {
		callback.call(scope, message.error || message.status || "unknown");
	});
	return message;
}

Handler.bind("/pubnub/wait", {
	onInvoke: (handler, message) => {
		let delay = message.requestObject || 0;
		handler.wait(delay);
	}
});

function setTimeout(callback, delay, param) {
	new MessageWithObject("/pubnub/wait", delay).invoke().then(() => {
		if (callback) callback(param);
	});
}

export function updateConfig(config) {
	return Object.assign(REST_API.config, config);
}

export class REST_API {
	static fetchHistory(params, callback, scope) {
		params = Object.assign({}, this.config, params);
		let query = params.query = { count:DEFAULT_QUERY_COUNT };
		if ("stringtoken" in params) {
			query.stringtoken = params.stringtoken;
		}
		if ("count" in params) {
			query.count = params.count;
		}
		if ("reverse" in params) {
			query.reverse = params.reverse;
		}
		if ("start" in params) {
			query.start = params.start;
		}
		if ("end" in params) {
			query.end = params.end;
		}
		if ("include_token" in params) {
			query.include_token = params.include_token;
		}
		if ("auth" in params) {
			query.auth = params.auth;
			debugger; // auth not supported
		}
		let channel = encodeURIComponent(params.channel) || "0";
		params.location = `/v2/history/sub-key/${params.sub_key}/channel/${channel}`;
		params.data = params.message = null; // do not POST
		return makeRequest(params, callback, scope);
	}
	static fetchTime(params, callback, scope) {
		params = Object.assign({}, this.config, params);
		params.location = "/time/0";
		params.data = params.message = null; // do not POST
		return makeRequest(params, callback, scope);
	}
	static publish(params, callback, scope) {
		params = Object.assign({}, this.config, params);
		let query = params.query = {};
		if ("store" in params) {
			query.store = params.store;
		}
		if ("auth" in params) {
			query.auth = params.auth;
			debugger; // auth not supported
		}
		let channel = encodeURIComponent(params.channel) || "0";
		params.location = `/publish/${params.pub_key}/${params.sub_key}/0/${channel}/0`;
		return makeRequest(params, callback, scope);
	}
	static subscribe(params, callback, scope) {
		params = Object.assign({}, this.config, params);
		let query = params.query = {};
		if ("state" in params) {
			query.state = params.state;
		}
		if ("heartbeat" in params) {
			query.heartbeat = params.heartbeat;
		}
		if ("auth" in params) {
			query.auth = params.auth;
			debugger; // auth not supported
		}
		let channel = encodeURIComponent(params.channel) || "0";
		let timetoken = params.timetoken || "0"; // zero for initial subscribe
		params.location = `/subscribe/${params.sub_key}/${channel}/0/${timetoken}`;
		params.data = params.message = null; // do not POST
		return makeRequest(params, callback, scope);
	}
}

REST_API.config = { authority:"pubsub.pubnub.com", pnsdk:"PubNub-JS-Kinoma/1.0", scheme:"http", timeout:0, uuid:system.deviceID }; // timeout?
