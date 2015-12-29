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
class _Iterator @ "xs_files_iterator_destructor" {
	constructor(path) @ "xs_files_iterator_constructor";
	getNext() @ "xs_files_iterator_getNext";
	close() @ "xs_files_iterator_close";
};

class _VolumeIterator @ "xs_volume_iterator_destructor" {
	constructor() @ "xs_volume_iterator_constructor";
	getNext() @ "xs_volume_iterator_getNext";
};

function* __Iterator(path) {
	var iter = new _Iterator(path);
	var item;
	while ((item = iter.getNext()) != null) {
		yield item;
	}
	iter.close();
};

function* __VolumeIterator() {
	var iter = new _VolumeIterator();
	var item;
	while ((item = iter.getNext()) != null) {
		yield item;
	}
};

export default class Files @ "xs_files_destructor" {
	constructor(path, mode) @ "xs_files_constructor";
	_read(n, buf) @ "xs_files_read";
	read(cons, n, buf) {
		var buf = this._read(n, buf);
		if (buf === undefined)
			;
		else if (cons === undefined)
			;
		else if (cons == String)
			// too bad we have to examine the constructor type just for String
			buf = String.fromArrayBuffer(buf);
		else
			buf = new cons(buf);
		return buf;
	};
	readChar() @ "xs_files_readChar";
	write(args) @ "xs_files_write";
	close() @ "xs_files_close";
	get length() @ "xs_files_getLength";
	set length(n) @ "xs_files_setLength";
	get position() @ "xs_files_getPosition";
	set position(n) @ "xs_files_setPosition";

	static deleteFile(path) @ "xs_files_delete";
	static deleteDirectory(path) @ "xs_files_deleteDirectory";
	static getInfo(path) @ "xs_files_getFileInfo";
	static getVolumeInfo(path) @ "xs_files_getVolumeInfo";
	static readChunk(path) {
		try {
			var f = new Files(path, 0);
			var blob = f._read(f.length);
			f.close();
			return blob;
		} catch(e) {
			// return undefined
		}
	};
	static writeChunk(path, blob) {
		var f = new Files(path, 1);
		f.write(blob);
		f.close();
	};

	static getSpecialDirectory(name) @ "xs_files_getSpecialDirectory";
	static get applicationDirectory() { return this.getSpecialDirectory("applicationDirectory"); };
	static get preferencesDirectory() { return this.getSpecialDirectory("preferencesDirectory"); };
	static get documentsDirectory() { return this.getSpecialDirectory("documentsDirectory"); };
	static get picturesDirectory() { return this.getSpecialDirectory("picturesDirectory"); };
	static get temporaryDirectory() { return this.getSpecialDirectory("temporaryDirectory"); };

	static get nativeApplicationDirectory() { return this.getSpecialDirectory("nativeApplicationDirectory"); };

	static Iterator(path) {
		return __Iterator(path);
	};
	static VolumeIterator() {
		return __VolumeIterator();
	};

	static checkDirectory(path) @ "xs_files_checkDirectory";

	static _init() @ "xs_files_init";

	static setActive(path) @ "xs_files_setActive";
};

Files._init();	// only for host
