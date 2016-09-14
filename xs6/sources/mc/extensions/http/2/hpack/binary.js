export function encodedStrLength(str) {
	return ArrayBuffer.fromString(str).byteLength;
}

export function encodeInt(v, n=8, bits=0) {
	const prefixMax = Math.pow(2, n) - 1;

	let result;
	bits = (bits <<n) & 0xff;

	if (v <= prefixMax) {
		result = [bits | v];
	} else {
		result = [bits | prefixMax];
		v -= prefixMax;

		while (v >= 128) {
			result.push(0b10000000 | (v % 128));
			v = 0 | (v / 128);
		}
		result.push(v);
	}

	return (new Uint8Array(result)).buffer;
}

export function decodeInt(stream, n=8) {
	const prefixMax = Math.pow(2, n) - 1;

	let c = stream.get();
	if (c === undefined) return;

	const prefix = c & prefixMax;

	if (prefix < prefixMax) {
		return prefix;
	}

	let val = prefix, m = 1;
	while (stream.canRead) {
		c = stream.get();
		val += (c & 0x7f) * m;

		if ((c & 0x80) == 0) {
			return val;
		}
		m *= 128;
	}
}

export function encodeStr(str, huffman=false) {
	const bytes = huffman ? require.weak('huffman').encode(str) : ArrayBuffer.fromString(str);
	const n = bytes.byteLength;
	return encodeInt(n, 7, huffman ? 1 : 0).concat(bytes);
},

export function decodeStr(stream) {
	const huffman = !!(stream.peek() & 0x80);
	const length = decodeInt(stream, 7);

	let source = stream.get(length);
	if (!source) return;

	return huffman ? require.weak('huffman').decode(source) : String.fromArrayBuffer(source);
}

