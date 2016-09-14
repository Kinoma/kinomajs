/*
	header field representation

	- indexed (header, value) pair as an reference to the table (static and dynamic)
	- literal (header, value) paire inserted into top of dynamic table
 */

import {
	Table,
	DynamicTable
} from 'hpack/table';

import {
	encodeInt,
	decodeInt,

	encodeStr,
	decodeStr
} from 'hpack/binary';

const AppendIndexing = 0;
const WithoutIndexing = 1;
const NeverIndexing = 2;

export const IndexingMode = {
	AppendIndexing,
	WithoutIndexing,
	NeverIndexing
};

class Base extends DynamicTable {
	constructor() {
		super();
		this.setParent(require.weak('hpack/static-table'));
	}
}

export class Encoder extends Base {
	encode(headerList) {
		let result = new ArrayBuffer(0);

		for (let [name, value] of headerList) {
			const mode = this.indexingModeForHeader(name, value);

			let index = this.reference(name, value);
			if (index) {
				result = result.concat(encodeInt(index, 7, 0b1));
				continue;
			}

			index = this.reference(name) || 0;
			if (mode === AppendIndexing) {
				result = result.concat(encodeInt(index, 6, 0b01));
				this.append([name, value]);
			} else {
				result = result.concat(encodeInt(index, 4, mode === WithoutIndexing ? 0 : 1));
			}

			if (!index) {
				result = result.concat(encodeStr(name, this.compression));
			}

			result = result.concat(encodeStr(value, this.compression));
			continue;
		}
		return result;
	}

	indexingModeForHeader(name, value) {
		return AppendIndexing;
	}
}

export class Decoder extends Base {
	decode(blob) {
		const stream = new StreamReader(blob);
		const decoded = [];
		let result, index, header, name, value;

		while (stream.canRead) {
			const c = stream.peek();

			if ((c & 0x80) == 0x80) {
				// 6.1 Indexed Header Field Representation
				index = decodeInt(stream, 7);
				if (index === undefined) return;
				header = this.dereference(index);
				if (header === undefined) return;

				decoded.push(header);
			} else if ((c & 0b11000000) == 0b01000000) {
				// 6.2.1 Literal Header Field with Incremental Indexing
				index = decodeInt(stream, 6);
				if (index === undefined) return;

				if (index == 0) {
					name = decodeStr(stream);
					if (name === undefined) return;
				} else {
					header = this.dereference(index);
					if (header === undefined) return;
					name = header[0];
				}
				value = decodeStr(stream);
				if (value === undefined) return;

				decoded.push([name, value]);
				this.append([name, value]);
			} else if ((c & 0b11110000) == 0b00000000 || (c & 0b11110000) == 0b00010000) {
				// 6.2.2 Literal Header Field without indexing
				// 6.2.3 Literal Header Field Never Indexed
				index = decodeInt(stream, 4);
				if (index === undefined) return;

				if (index == 0) {
					name = decodeStr(stream);
					if (name === undefined) return;
				} else {
					header = this.dereference(index);
					if (header === undefined) return;
					name = header[0];
				}
				value = decodeStr(stream);
				if (value === undefined) return;

				decoded.push([name, value]);
			} else if ((c & 0b11100000) == 0b00100000) {
				// 6.3 Dynamic Table Size Update
				value = decodeInt(stream, 5);
				this.setMaxSize(value);
			} else {
				return;
			}
		}

		return decoded;
	}
}

export class StreamReader {
	constructor(blob) {
		this.view = new Uint8Array(blob);
		this.offset = 0;
	}

	get length() {
		return this.view.length;
	}

	get canRead() {
		return this.offset < this.length;
	}

	peek() {
		return this.view[this.offset];
	}

	get(count) {
		if (count === undefined) {
			return this.view[this.offset++];
		} else if ((this.offset + count) <= this.length) {
			let start = this.offset;
			this.offset += count
			return this.view.buffer.slice(start, this.offset);
		}
	}
}

class StreamWriter {
	put(...args) {
		for (let val of args) {
			if (typeof val == 'string') {

			} else if (val instanceof ArrayBuffer) {

			} else {

			}
		}
	}
}
