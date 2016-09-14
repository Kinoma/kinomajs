// RFC7540 4. HTTP Frame

export const Type = {
	DATA: 0x00,
	HEADERS: 0x01,
	PRIORITY: 0x02,
	RST_STREAM: 0x03,
	SETTINGS: 0x04,
	PUSH_PROMISE: 0x05,
	PING: 0x06,
	GOAWAY: 0x07,
	WINDOW_UPDATE: 0x08,
	CONTINUATION: 0x09,
};

export const Flag = {
	END_STREAM: 0x1, // DATA, HEADERS
	PADDED: 0x8, // DATA, HEADERS, PUSH_PROMISE

	END_HEADERS: 0x4, // HEADERS, PUSH_PROMISE, CONTINUATION
	PRIORITY: 0x20, // HEADERS

	ACK: 0x1, // SETTINGS, PING
};

export const Settings = {
	SETTINGS_HEADER_TABLE_SIZE: 0x1,
	SETTINGS_ENABLE_PUSH: 0x2,
	SETTINGS_MAX_CONCURRENT_STREAMS: 0x3,
	SETTINGS_INITIAL_WINDOW_SIZE: 0x4,
	SETTINGS_MAX_FRAME_SIZE: 0x5,
	SETTINGS_MAX_HEADER_LIST_SIZE: 0x6,
};

export function pack({type, flags, r=false, streamId=0, payload}) {
	let length = (payload ? payload.byteLength : 0);
	let bytes = new ArrayBuffer(9);
	let view = new DataView(bytes);

	view.setUint16(0, length >> 8);
	view.setUint8(2, length & 0xff);
	view.setUint8(3, type);
	view.setUint8(4, flags);
	view.setUint32(5, streamId);

	if (payload) bytes = bytes.concat(payload);

	return bytes;
}

export function unpack(bytes) {
	const frame = peek(bytes);
	if (frame) {
		const length = 9 + frame.length;
		delete frame.length;

		if (bytes.byteLength >= length) {
			frame.payload = bytes.slice(9, length);
			return [frame, bytes.slice(length)];
		}
	}

	return [undefined, bytes];
}

export function peek(bytes) {
	if (bytes.byteLength < 9) return;

	const view = new DataView(bytes);
	const length = (view.getUint16(0) << 8) + view.getUint8(2);

	const type = view.getUint8(3);
	const flags = view.getUint8(4);

	let streamId = view.getUint32(5);
	let r = !!(streamId >> 31);
	streamId = streamId & 0x7fffffff;

	return {type, flags, r, streamId, length};
}

export default {
	Type, Flag,
	pack, unpack, peek
};

