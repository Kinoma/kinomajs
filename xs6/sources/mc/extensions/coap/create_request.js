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

import {
	Option,
	Method,
	Type,
	Port,
	Message
} from 'common';

export function createRequest(data) {
	const {parseUrl, isDottedAddress} = require.weak('utils/net');
	var url = data.url;
	if (!url) throw "url is required";

	var parts = parseUrl(url);
	var host = parts.host;
	var port = parts.port || Port;
	if (!parts.scheme || !host || !port) throw "bad url";
	if (parts.scheme != 'coap') throw "bad scheme";

	var options = data.options || [];

	const request = {
		__proto__: Message,
		url: url,
		host: host,
		port: port,

		type: data.type || (('confirmable' in data ? data.confirmable : true) ? Type.Con : Type.Non),
		code: [0, data.method ? data.method : Method.GET],
		options: options,
		payload: data.payload,

		messageId: data.messageId,
		token: data.token,

		created: Date.now(),

		onResponse: data.onResponse,
		onAck: data.onAck,
		onError: data.onError
	};

	request.observe = data.observe;

	if (!isDottedAddress(host)) {
		request.setOption(Option.UriHost, host);
	}

	if (port != Port) {
		request.setOption(Option.UriPort, port);
	}

	var path = parts.path;
	if (path) {
		request.clearOptions(Option.UriPath);
		path.split('/').slice(1).forEach(function(part) {
			request.addOption(Option.UriPath, decodeURIComponent(part));
		});
	}

	var query = parts.query;
	if (query) {
		request.clearOptions(Option.UriQuery);
		query.split('&').forEach(function(part) {
			request.addOption(Option.UriQuery, decodeURIComponent(part));
		});
	}

	return request;
}

export default createRequest;
