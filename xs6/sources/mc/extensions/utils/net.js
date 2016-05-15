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

export function portForScheme(scheme) {
	if (scheme == "https") return 443;
	if (scheme == "http") return 80;
	if (scheme == "coap") return 5683;
}

/* ported from kpr.c */
export function parseUrl(url) {
	const substr = (a,b) => url.substring(a,b);
	const charAt = a => url.charAt(a);

	let parts = {};

	let c, p = 0, q = 0, name;

	while (c = charAt(p)) {
		if ((c == ':') || (c == '/') || (c == '?') || (c == '#'))
			break;
		p++;
	}
	if (c == ':') {
		parts.scheme = substr(q, p);
		p++;
		q = p;
		c = charAt(p);
	}
	if (c == '/') {
		p++;
		c = charAt(p);
		if (c == '/') {
			let semi, host;

			p++;
			q = p;
			while ((c = charAt(p))) {
				if (c == '/')
					break;
				if (c == ':')
					semi = p;
				if (c == '@') {
					let user;
					host = p + 1;
					user = q;
					if (semi !== undefined) {
						parts.user = substr(user, semi);
						parts.password = substr(semi + 1, p);
						semi = undefined;
					} else {
						parts.user = substr(user, p);
					}
				}
				p++;
			}
			parts.authority = substr(q, p);
			if (parts.authority) {
				if (!host)
					host = q;
				if (semi) {
					parts.host = substr(host, semi);
					parts.port = parseInt(substr(semi + 1, p));
				} else {
					if (parts.scheme) {
						const {portForScheme} = require.weak('utils/net');
						parts.port = portForScheme(parts.scheme);
					}
					parts.host = substr(host, p);
				}
			}
			q = p;
		}
		else if (c)
			name = p;
	}
	while ((c = charAt(p))) {
		if (c == '/')
			name = p + 1;
		if ((c == '?') || (c == '#'))
			break;
		p++;
	}
	parts.path = substr(q, p);
	if (name)
		parts.name = substr(name, p);
	if (c == '?') {
		p++;
		q = p;
		while ((c = charAt(p))) {
			if (c == '#')
				break;
			p++;
		}
		parts.query = substr(q, p);
	}
	if (c == '#') {
		p++;
		q = p;
		parts.fragment = substr(p, url.length);
	}

	return parts;
}

export function isDottedAddress(addr) {
	let parts = addr.split('.', 5);
	if (parts.length != 4) return false;
	return parts.every(part => {
		if (part.length > 1 && part.substring(0, 1) === '0') return false;
		let v = parseInt(part);
		if (v != part) return false;
		if (v < 0 || v > 255) return false;
		return true;
	})
}

export default {
	parseUrl,
	isDottedAddress,
	portForScheme,
}

