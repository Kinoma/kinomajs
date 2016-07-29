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
	Client,
	Method
} from 'coap';

function getResult(response) {
	const payload = response.payload;
	if (!payload) return undefined;
	return response.contentFormat == 'JSON' ? JSON.parse(String.fromArrayBuffer(payload)) : payload;
}

export default class PinsCoAPClientHandler {
	constructor(url) {
		this.coap = new Client();
		this.url = url;

	}

	close() {
		delete this.coap;
		delete this.url;
	}

	invoke(path, opt, callback) {
		if (typeof opt == 'function') {
			callback = opt;
			opt = undefined;
		}

		const request = this.coap.createRequest(this.url + 'invoke?path=' + path);
		request.confirmable = true;

		if (opt) {
			request.method = Method.POST;
			request.setPayload(JSON.stringify(opt), 'JSON');
		}

		if (callback) {
			request.onResponse = response => {
				callback.call(null, getResult(response));
			};
		}

		this.coap.send(request);
	}

	repeat(path, opt, ti, callback) {
		if (typeof ti == 'function') {
			callback = ti;
			ti = opt;
			opt = undefined;
		}

		const condition = (typeof ti == 'number' ? 'interval' : 'timer') + '=' + ti;
		const request = this.coap.createRequest(this.url + 'repeat?path=' + path + '&' + condition);
		request.observe = true;
		request.confirmable = true;
		if (callback) {
			request.onResponse = response => {
				if (response.observeId > 2) {
					callback.call(null, getResult(response));
				}
			};
		}

		this.coap.send(request);
		return {
			close() {

			}
		}
	}
};
