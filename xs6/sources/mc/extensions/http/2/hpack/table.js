import {encodedStrLength} from 'binary';

export class Table {
	constructor(table=[]) {
		this._table = table;
	}

	get length() {
		return this._table.length;
	}

	dereference(index) {
		if (index >= 1 && index <= this._table.length) {
			return this._table[index - 1];
		}
	}

	reference(name, value) {
		for (let i = 0, c = this._table.length; i < c; i++) {
			let entry = this._table[i];
			if (entry[0] == name) {
				if (value === undefined || entry[1] == value) return i + 1;
			}
		}
	}
}

export class DynamicTable extends Table {
	constructor() {
		super([]);
		this.sizes = [];
	}

	setParent(parent) {
		this.parent = parent;
		return this;
	}

	append(pair) {
		const size = DynamicTable.sizeOfEntry(pair);
		if (this.maxSize === 0 || this.maxSize > 0) {
			this.shurinkTableToSize(this.maxSize - size);
		}

		if (this.maxSize === undefined || this.maxSize >= size) {
			this._table.unshift(pair);
			this.sizes.unshift(size);
		}
	}

	get length() {
		return (this.parent ? this.parent.length : 0) + this._table.length;
	}

	dereference(index) {
		if (this.parent) {
			const result = this.parent.dereference(index);
			if (result) return result;

			index -= this.parent.length;
		}

		return super.dereference(index);
	}

	reference(name, value) {
		let offset = 0, result;

		if (this.parent) {
			result = this.parent.reference(name, value);
			if (result) return result;

			offset = this.parent.length;
		}

		result = super.reference(name, value);
		if (result) return offset + result;
	}

	popTable() {
		this._table.pop();
		this.sizes.pop();
	}

	shurinkTableToSize(size) {
		while (this.sizes.length > 0 && size < this.size) {
			this.popTable();
		}
	}

	setMaxSize(size) {
		this.shurinkTableToSize(size);

		this.maxSize = size;
	}

	get size() {
		return this.sizes.reduce((total, size) => total + size, 0);
	}

	static sizeOfEntry([name, value]) {
		return encodedStrLength(name) + encodedStrLength(value) + 32;
	}
}

