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

import {CoAPSocket as Socket} from 'coap_socket';
import {
	Type,
	Method,
	Option,
	OptionFormat,
	optionFormat,
	ContentFormat,
	Port
} from 'common';

function str(val) {
	if (val instanceof ArrayBuffer) val = String.fromArrayBuffer(val);
	else val = val.toString();
	return '"' + val.replace(/"/g, '\\"') + '"';
}

function hex(blob) {
	return '<' + Array.from(new Uint8Array(blob))
		.map(val => ('0' + val.toString(16)).slice(-2))
		.join(' ') + '>';
}

function timestamp() {
	let now = new Date();
	let ms = '00' + now.getMilliseconds();
	ms = ms.substring(ms.length - 3);

	return now.toTimeString().split(' ')[0] + '.' + ms;
}

function findKey(obj, value) {
	for (let key in obj) {
		if (obj[key] === value) return key;
	}
}

function codeToString(code) {
	if (!code || (code[0] == 0 && code[1] == 0)) return 'Empty';
	if (code[0] == 0) return findKey(Method, code[1]);
	return code[0] + '.' + ('0' + code[1].toString()).slice(-2);
}

function devideOptions(options, target) {
	let match = [], rest = [];
	options.forEach(option => {
		if (target.indexOf(option[0]) >= 0)
			match.push(option);
		else
			rest.push(option);
	});
	options.splice(0, options.length, ...rest);
	return match;
}

function buildUrl(options, peer) {
	let url = 'coap://';

	// host
	let host = devideOptions(options, [Option.UriHost]).shift();
	url += (host ? host[1] : peer.split(':')[0]);

	// port
	let port = devideOptions(options, [Option.UriPort]).shift();
	port = (port ? port[1] : peer.split(':')[1]);
	if (port == Port) url += ':' + port;

	// path
	let path = devideOptions(options, [Option.UriPath]).map(([_, val]) => '/' + val).join('');
	url += (path ? path : '/');

	let query = devideOptions(options, [Option.UriQuery]).map(([_, val]) => val).join('&');
	if (query) url += '?' + query;

	return url;
}

function opts2str(options) {
	return options.map(([option, value]) => {
		let key = findKey(Option, option);
		if (key) {
			switch (optionFormat(option)) {
				case OptionFormat.String:
					value = str(value);
					break;
				case OptionFormat.Opaque:
					value = hex(value);
					break;
			}
		} else {
			key = 'Unknown(' + option + ')';
		}
		return key + ':' + value;
	});
}

export function messageDigest({type, code, messageId, token, options=[], payload}, peer) {
	options = options.slice();
	let urls = devideOptions(options, [Option.UriHost, Option.UriPort, Option.UriPath, Option.UriQuery]);
	let format = devideOptions(options, [Option.ContentFormat]).shift();

	return [
		peer,
		findKey(Type, type),
		codeToString(code),
		'M:0x' + messageId.toString(16),
		token ? 'T:' + hex(token) : undefined,
		urls.length > 0 ? buildUrl(urls, peer) : undefined,
		format ? 'ContentFormat:' + findKey(ContentFormat, format[1]) : undefined,
		...opts2str(options),
		payload ? 'Payload:' + str(payload) : undefined,
	].filter(v => v).join(' ');
}

export class CoAPSocket extends Socket {
	constructor(receive, error) {
		super((message, peer) => {
			console.log(timestamp() + " | << " + messageDigest(message, peer));
			receive(message, peer);
		}, err => {
			console.log(timestamp() + " | error: " + err);
			error(err);
		});
	}

	send(message, peer) {
		console.log(timestamp() + " | >> " + messageDigest(message, peer));
		super.send(message, peer);
	}
}

export default CoAPSocket;
