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

export const Type = {
	Con: 0,
	Non: 1,
	Ack: 2,
	Rst: 3,
};

export const Method = {
	GET: 1,
	POST: 2,
	PUT: 3,
	DELETE: 4
};

export const Option = {
	// RFC 7252 core
	IfMatch: 1,
	UriHost: 3,
	ETag: 4,
	IfNoneMatch: 5,
	UriPort: 7,
	LocationPath: 8,
	UriPath: 11,
	ContentFormat: 12,
	MaxAge: 14,
	UriQuery: 15,
	Accept: 17,
	LocationQuery: 20,
	ProxyUri: 35,
	ProxyScheme: 39,
	Size1: 60,

	// draft-ietf-core-block-15
	Block2: 23,
	Block1: 27,
	Size2: 28,

	// draft-ietf-core-observe-14
	Observe: 6,
};

export const OptionFormat = {
	Empty: 0,
	Opaque: 1,
	Uint: 2,
	String: 3,
};

export const ContentFormat = {
	PlainText: 0,
	LinkFormat: 40,
	XML: 41,
	OctetStream: 42,
	EXI: 47,
	JSON: 50,
};

export const ContentType = {
	"text/plain; charset=utf-8": ContentFormat.PlainText,
	"text/plain": ContentFormat.PlainText,
	"text": ContentFormat.PlainText,
	"plaintext": ContentFormat.PlainText,

	"application/link-format": ContentFormat.LinkFormat,
	"link-format": ContentFormat.LinkFormat,
	"linkformat": ContentFormat.LinkFormat,

	"application/xml": ContentFormat.XML,
	"xml": ContentFormat.XML,

	"application/octet-stream": ContentFormat.OctetStream,
	"octet-stream": ContentFormat.OctetStream,
	"octetstream": ContentFormat.OctetStream,

	"application/exi": ContentFormat.EXI,
	"exi": ContentFormat.EXI,

	"application/json": ContentFormat.JSON,
	"json": ContentFormat.JSON,
};

export const Observe = {
	Register: 0,
	Deregister: 1
};

function findKey(obj, value) {
	for (let key in obj) {
		if (obj[key] === value) return key;
	}
}

export function parseContentFormat(format) {
	if (typeof format == 'string') {
		if (format in ContentFormat) {
			format = ContentFormat[format];
		} else if (format.toLowerCase() in ContentType) {
			format = ContentType[format.toLowerCase()];
		} else {
			throw new Error('invalid content format: ' + format);
		}
	} else if (typeof format != 'number') {
		throw new Error('invalid content format: ' + format);
	}

	if (findKey(ContentFormat, format)) return format;

	throw new Error('invalid content format: ' + format);
}

export const Message = {
	getOptions(option) {
		if (!this.options || this.options.length == 0) return [];

		return this.options
			.filter(([opt, value]) => opt == option)
			.map(([opt, value]) => value);
	},
	getOption(option) {
		return this.getOptions(option).shift();
	},
	addOption(option, value) {
		if (!this.options) this.options = [];
		this.options.push([option, value]);
	},
	setOption(option, value) {
		this.clearOptions(option);
		this.addOption(option, value);
	},
	clearOptions(option) {
		if (this.options) {
			this.options = this.options.filter(([opt, value]) => opt != option);
		}
	},
	setCode(cls, detail) {
		this.code = [cls, detail];
	},
	setPayload(payload, contentFormat) {
		if (typeof payload == 'string') payload = ArrayBuffer.fromString(payload);
		this.payload = payload;
		this.contentFormat = contentFormat;
	},
	get observe() {
		return this.observeId === Observe.Register;
	},
	set observe(val) {
		if (val) {
			this.observeId = Observe.Register;
		} else {
			this.clearOptions(Option.Observe);
		}
	},
	get observeDeregister() {
		return this.observeId === Observe.Deregister;
	},
	set observeDeregister(val) {
		if (val) {
			this.observeId = Observe.Deregister;
		} else {
			this.clearOptions(Option.Observe);
		}
	},
	get observeId() {
		return this.getOption(Option.Observe);
	},
	set observeId(val) {
		this.clearOptions(Option.Observe);
		this.addOption(Option.Observe, val);
	},
	get confirmable() {
		return this.type == Type.Con;
	},
	set confirmable(flag) {
		this.type = flag ? Type.Con : Type.Non;
	},
	get url() {
		let url = 'coap://' + this.host;
		const port = this.port;
		if (port != Port) url += ':' + port;
		url += '/' + this.getOptions(Option.UriPath).map(encodeURIComponent).join('/');
		const queries = this.queries;
		if (queries.length > 0) url += '?' + queries.map(encodeURIComponent).join('&');
		return url;
	},
	get uri() {
		return this.url;
	},
	get host() {
		return this.getOption(Option.UriHost);
	},
	get port() {
		const port = this.getOption(Option.UriPort);
		return port ? port : Port;
	},
	get path() {
		return '/' + this.getOptions(Option.UriPath).join('/');
	},
	get query() {
		return this.queries.join('&');
	},
	get queries() {
		return this.getOptions(Option.UriQuery);
	},
	get contentFormat() {
		let format = this.getOption(Option.ContentFormat);
		if (format !== undefined) {
			return findKey(ContentFormat, format);
		}
	},
	set contentFormat(val) {
		this.clearOptions(Option.ContentFormat);
		if (val != undefined && val != null) this.addOption(Option.ContentFormat, parseContentFormat(val));
	},
	get contentType() {
		let format = this.getOption(Option.ContentFormat);
		if (format !== undefined) {
			return findKey(ContentType, format);
		}
	},
	set contentType(val) {
		this.contentFormat = val;
	},
};

export const Port = 5683;

export const ACK_TIMEOUT = 2;  /* sec */
export const ACK_RANDOM_FACTOR = 1.5;
export const MAX_RETRANSMIT = 4;
export const NSTART = 1;
export const DEFAULT_LEISURE = 5; /* sec */
export const PROBING_RATE = 1; /* byte/second */

/* MAX_LATENCY is the maximum time a datagram is expected to take
 * from the start of its transmission to the completion of its
 * reception.  This constant is related to the MSL (Maximum Segment
 * Lifetime) of [RFC0793], which is "arbitrarily defined to be 2
 * minutes" ([RFC0793] glossary, page 81).  Note that this is not
 * necessarily smaller than MAX_TRANSMIT_WAIT, as MAX_LATENCY is not
 * intended to describe a situation when the protocol works well, but
 * the worst-case situation against which the protocol has to guard.
 * We, also arbitrarily, define MAX_LATENCY to be 100 seconds.  Apart
 * from being reasonably realistic for the bulk of configurations as
 * well as close to the historic choice for TCP, this value also
 * allows Message ID lifetime timers to be represented in 8 bits
 * (when measured in seconds).  In these calculations, there is no
 * assumption that the direction of the transmission is irrelevant
 * (i.e., that the network is symmetric); there is just the
 * assumption that the same value can reasonably be used as a maximum
 * value for both directions.  If that is not the case, the following
 * calculations become only slightly more complex.
 */
export const MAX_LATENCY = 100; /* sec */

/*
 * PROCESSING_DELAY is the time a node takes to turn around a
 * Confirmable message into an acknowledgement.  We assume the node
 * will attempt to send an ACK before having the sender time out, so
 * as a conservative assumption we set it equal to ACK_TIMEOUT.
 */
export const PROCESSING_DELAY = ACK_TIMEOUT;

/* MAX_TRANSMIT_SPAN is the maximum time from the first transmission
 * of a Confirmable message to its last retransmission.  For the
 * default transmission parameters, the value is (2+4+8+16)*1.5 = 45
 * seconds, or more generally:
 *
 *   ACK_TIMEOUT * ((2 ** MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR
 */
export const MAX_TRANSMIT_SPAN =  ACK_TIMEOUT * (Math.pow(2, MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR;

/*
 * EXCHANGE_LIFETIME is the time from starting to send a Confirmable
 * message to the time when an acknowledgement is no longer expected,
 * i.e., message-layer information about the message exchange can be
 * purged.  EXCHANGE_LIFETIME includes a MAX_TRANSMIT_SPAN, a
 * MAX_LATENCY forward, PROCESSING_DELAY, and a MAX_LATENCY for the
 * way back.  Note that there is no need to consider
 * MAX_TRANSMIT_WAIT if the configuration is chosen such that the
 * last waiting period (ACK_TIMEOUT * (2 ** MAX_RETRANSMIT) or the
 * difference between MAX_TRANSMIT_SPAN and MAX_TRANSMIT_WAIT) is
 * less than MAX_LATENCY -- which is a likely choice, as MAX_LATENCY
 * is a worst-case value unlikely to be met in the real world.  In
 * this case, EXCHANGE_LIFETIME simplifies to:
 *
 *   MAX_TRANSMIT_SPAN + (2 * MAX_LATENCY) + PROCESSING_DELAY
 *
 * or 247 seconds with the default transmission parameters.
 */
export const EXCHANGE_LIFETIME = MAX_TRANSMIT_SPAN + (2 * MAX_LATENCY) + PROCESSING_DELAY;

export function optionFormat(opt) {
	switch (opt) {
		case Option.IfNoneMatch:
			return OptionFormat.Empty;

		case Option.IfMatch:
		case Option.ETag:
			return OptionFormat.Opaque;

		case Option.UriPort:
		case Option.ContentFormat:
		case Option.MaxAge:
		case Option.Accept:
		case Option.Size1:
		case Option.Block2:
		case Option.Block1:
		case Option.Size2:
		case Option.Observe:
			return OptionFormat.Uint;

		case Option.UriHost:
		case Option.LocationPath:
		case Option.UriPath:
		case Option.UriQuery:
		case Option.LocationQuery:
		case Option.ProxyUri:
		case Option.ProxyScheme:
			return OptionFormat.String;
	}

	throw "Invalid option";
}

export default {
	Type,
	Method,
	Option,
	Port,
	OptionFormat,
	ContentFormat,
	Observe,
	optionFormat,

	Message,

	ACK_TIMEOUT,
	ACK_RANDOM_FACTOR,
	MAX_RETRANSMIT,
	NSTART,
	DEFAULT_LEISURE,
	PROBING_RATE,
	MAX_LATENCY,
	PROCESSING_DELAY,
	MAX_TRANSMIT_SPAN,
	EXCHANGE_LIFETIME
};

