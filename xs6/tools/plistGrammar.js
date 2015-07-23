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
import Grammar from "grammar";
import infoset from "infoset";

class Buffer {
	constructor(it) {
		this.string = "";
		this.tabs = 0;
	}
	line(...strings) {
		if (this.tabs)
			this.string += "\t".repeat(this.tabs);
		for (var string of strings)
			this.string += string;	
		this.string += "\n";
	}
};

class Data extends String {
	constructor(it) {
		super(it);
	}
};

var _key = {
	name: "",
};
var _value = {
	map() {
		return this.value;
	}
};

var _data = {
	__proto__: _value,
	value: "",
	map() {
		return new Data(this.value);
	}
};
var _date = {
	__proto__: _value,
	value: "",
	map() {
		return new Date(Date.parse(this.value));
	}
};
var _false = {
	__proto__: _value,
	value: false,
};
var _integer = {
	__proto__: _value,
	value: 0,
};
var _real = {
	__proto__: _value,
	value: 0,
};
var _string = {
	__proto__: _value,
	value: "",
};
var _true = {
	__proto__: _value,
	value: true,
};

var _array = {
	__proto__: _value,
	items: [],
	map() {
		return this.items.map(item => item.map());
	}
};
var _dict = {
	__proto__: _value,
	items: [],
	map() {
		var result = {};
		var items = this.items;
		var c = items.length, i = 0;
		while (i < c) {
			var key = items[i++];
			var value = items[i++];
			result[key.name] = value.map();
		}
		return result;
	}
};

var _plist = {
	dict: _dict,
	map() {
		return this.dict.map();
	}
};

class PListGrammar extends Grammar {
	constructor(it) {
		super(it);
	}
	parse(buffer, path) {
		return super.parse(buffer, path).map();
	}
	stringify(dictionary) {
		var buffer = new Buffer;
		buffer.line("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
		buffer.line("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">");
		buffer.line("<plist version=\"1.0\">");
		this.stringifyValue(buffer, dictionary);
		buffer.line("</plist>");
		return buffer.string;
	}
	stringifyValue(buffer, value) {
		if (typeof value == "boolean")
			buffer.line(value ? "<true/>" : "<false/>");
		else if (typeof value == "number") {
			if (Number.isInteger(value))
				buffer.line("<integer>", value, "</integer>");
			else
				buffer.line("<real>", value, "</real>");
		}
		else if (typeof value == "string")
			buffer.line("<string>", value, "</string>");
		else if (value instanceof Array) {
			buffer.line("<array>");
			buffer.tabs++;
			for (var item of value)
				this.stringifyValue(buffer, item);
			buffer.tabs--;
			buffer.line("</array>");
		}
		else if (value instanceof Data) {
			buffer.line("<data>", value, "</data>");
		}
		else if (value instanceof Date) {
			buffer.line("<date>", value.toISOString(), "</date>");
		}
		else if (value instanceof Object) {
			buffer.line("<dict>");
			buffer.tabs++;
			for (var name in value) {
				buffer.line("<key>", name, "</key>");
				this.stringifyValue(buffer, value[name]);
			}
			buffer.tabs--;
			buffer.line("</dict>");
		}
	}
};

var g = new PListGrammar(infoset);

g.object(_key, "key", {
	name: g.string("."),
});
g.object(_false, "false", {
});
g.object(_data, "data", {
	value: g.string("."),
});
g.object(_date, "date", {
	value: g.string("."),
});
g.object(_integer, "integer", {
	value: g.number("."),
});
g.object(_real, "real", {
	value: g.number("."),
});
g.object(_string, "string", {
	value: g.string("."),
});
g.object(_true, "true", {
});
g.object(_array, "array", {
	items: g.array(".", _value),
});
g.object(_dict, "dict", {
	items: g.array(".", _key, _value),
});
g.object(_plist, "/plist", {
	dict: g.reference(".", _dict),
	version: g.string("@version"),
});
g.link();

export default g;
