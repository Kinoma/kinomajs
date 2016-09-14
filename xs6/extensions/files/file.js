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

class FileHost @ "xs_file_destructor" {
	constructor(path, mode) @ "xs_file_constructor";
	read(n, buf) @ "xs_file_read";
	readChar() @ "xs_file_readChar";
	write(args) @ "xs_file_write";
	close() @ "xs_file_close";
	get length() @ "xs_file_getLength";
	set length(n) @ "xs_file_setLength";
	get position() @ "xs_file_getPosition";
	set position(n) @ "xs_file_setPosition";
};

class Map @ "xs_file_map_destructor" {
	constructor(path) @ "xs_file_map_constructor";
};

export default class File extends FileHost {
	read(cons, n, buf) {
		buf = super.read(n, buf);
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
};

File.Map = Map;
