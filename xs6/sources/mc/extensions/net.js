/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
import Utils from 'utils';

let _busyObjects = [];

function beginBusy(obj) {
	_busyObjects.push(obj);
	if (!require.busy) require.busy = true;
}

function endBusy(obj) {
	var index = _busyObjects.indexOf(obj);
	if (index >= 0) {
		_busyObjects.splice(index, 1);
		if (!_busyObjects.length) require.busy = false;
	}
}

/* ported from kpr.c */
function parseUrl(url) {
	var parts = {};

	var c, p, q;
	var $ = Utils.ptr(url);

	var q = p = 0;
	var semi, host, user, name;

	while (c = $(p)) {
		if ((c == ':') || (c == '/') || (c == '?') || (c == '#'))
			break;
		p++;
	}
	if (c == ':') {
		parts.scheme = $(q, p);
		p++;
		q = p;
		c = $(p);
	}
	if (c == '/') {
		p++;
		c = $(p);
		if (c == '/') {
			p++;
			q = p;
			while ((c = $(p))) {
				if (c == '/')
					break;
				if (c == ':')
					semi = p;
				if (c == '@') {
					host = p + 1;
					user = q;
					if (semi !== undefined) {
						parts.user = $(user, semi);
						parts.password = $(semi + 1, p);
						semi = undefined;
					} else {
						parts.user = $(user, p);
					}
				}
				p++;
			}
			parts.authority = $(q, p);
			if (parts.authority) {
				if (!host)
					host = q;
				if (semi) {
					parts.host = $(host, semi);
					parts.port = parseInt($(semi + 1, p));
				} else {
					if (parts.scheme) {
						if (parts.scheme == "https")
							parts.port = 443;
						else if (parts.scheme == "http")
							parts.port = 80;
						else if (parts.scheme == "coap")
							parts.port = 5683;
					}
					parts.host = $(host, p);
				}
			}
			q = p;
		}
		else if (c)
			name = p;
	}
	while ((c = $(p))) {
		if (c == '/')
			name = p + 1;
		if ((c == '?') || (c == '#'))
			break;
		p++;
	}
	parts.path = $(q, p);
	if (name)
		parts.name = $(name, p);
	if (c == '?') {
		p++;
		q = p;
		while ((c = $(p))) {
			if (c == '#')
				break;
			p++;
		}
		parts.query = $(q, p);
	}
	if (c == '#') {
		p++;
		q = p;
		parts.fragment = $(p, url.length);
	}

	return parts;
}

function isDottedAddress(addr) {
	var parts = addr.split('.', 5);
	if (parts.length != 4) return false;
	return parts.every(function(part) {
		if (part.length > 1 && part.substring(0, 1) === '0') return false;
		var v = parseInt(part);
		if (v != part) return false;
		if (v < 0 || v > 255) return false;
		return true;
	})
}

export default {
	parseUrl,
	isDottedAddress,
}

