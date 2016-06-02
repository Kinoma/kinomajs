let output = function(obj) {
	console.log(obj);
}

export function out(obj) {
	output(obj);
}

export function stringify(obj) {
	return format(obj).trim();
}

class ObjectRepository {
	constructor() {
		this.repo = [];
	}

	getOid(v) {
		var oid = false;
		for (var i = 0, c = this.repo.length; i < c; i++) {
			if (v === this.repo[i]) {
				oid = i + 1;
				break;
			}
		}
		return oid;
	}

	checkIn(v) {
		this.repo.push(v);
		return this.repo.length;
	}

}

function format(v) {
	var lines = formatValue({v, indent: '', prefix:'', suffix:'', repo: new ObjectRepository()});

	var result = [];
	while (lines.length > 0) {
		var line = lines.shift();
		// console.log(line);
		if (typeof line == 'string') {
			let len = line.length;
			if (len > 80) {
				line = line.substring(0, 80) + `... (${len} bytes})`
			}
			result.push(line);
		} else {
			var indent = line[0];
			var sub = line[1]();
			lines = sub.map(line => (typeof line == 'string') ? indent + line : line)
					.concat(lines);
		}
	}

	return result.join("\n") + "\n";
}

function formatValue(args) {
	var v = args.v;
	var t = typeof v;
	// console.log(`prefix: ${args.prefix} suffix: ${args.suffix} v: ${args.v} \n`);

	if (v === undefined) {
		v = ['undefined'];
	} else if (v === null) {
		v = ['null'];
	} else if (v instanceof ArrayBuffer) {
		v = formatChunk(v, 4);
	} else if (v instanceof Array) {
		var open = '[', close = ']';

		var oid = args.repo.getOid(v);
		if (oid) {
			v = [open + ' (#' + oid + ') ... ' + close];
		} else {
			let c = v.length
			var pairs = new Array(c);
			for (var i = 0; i < c; i++) pairs[i] = [i, v[i]];

			v = formatStructure({pairs, open, close, indent:args.indent, repo:args.repo});
		}
	} else {
		if (t === 'object') {
			v = v.valueOf();
			t = typeof v;
		}

		switch (t) {
			case 'string':
				v = ['"' +
					   v.replace('\\', '\\\\')
						.replace('"', '\\"')
						.replace("'", "\\'")
						.replace('\n', '\\n')
						.replace('\r', '\\r')
						.replace('\t', '\\t')
					+ '"'];
				break;

			case 'number':
				v = ['' + v];
				break;

			case 'boolean':
				v = [v ? 'true' : 'false'];
				break;

			case 'function':
				v = formatFunction(v);
				break;

			case 'object':
				var open = '{', close = '}';
				var oid = args.repo.getOid(v);
				if (oid) {
					v = [open + ' (#' + oid + ') ... ' + close];
				} else {
					var pairs = Object.keys(v).map(key => [key.toString(), v[key]]);
					v = formatStructure({v, pairs, open, close, indent:args.indent, repo:args.repo});
				}
				break;

			default:
				v = [`unknown ${t} (${v})`];
				break;
		}
	}

	v[0] = args.prefix + v[0];
	v[v.length - 1] += args.suffix;
	return v;
}

function formatStructure(args) {
	var repo = args.repo, pairs = args.pairs, indent = args.indent + '  ';
	var oid = repo.checkIn(args.v);

	return [
		args.open + ' (#' + oid + ')',
		[
			indent,
			function() {
				var last = pairs.length - 1;
				var lines = [];
				pairs.forEach((pair, index) => {
					lines = lines.concat(formatValue({
						v: pair[1],
						prefix: pair[0] + ': ',
						suffix: (index < last ? ',' : ''),
						indent,
						repo
					}));
				});
				return lines;
			},
		],
		args.close,
	];
}

function formatFunction(f) {
	return ['function(...) { ... }'];
}

function formatChunk(chunk, maxLines) {
	if (chunk.byteLength == 0) return ['ArrayBuffer (0 bytes)'];

	var lines = [];
	var num = 16;
	var view = new Uint8Array(chunk);
	var len = view.length;

	var sep = '| ', line = sep, cs = sep;

	for (var i = 0; i < len; ++i) {
		if (i > 0 && i % num == 0) {
			if (line) lines.push(line + cs);
			line = cs = sep;
			if (maxLines && lines.length >= maxLines) {
				lines.push("...");
				break;
			}
		}

		var n = view[i];
		var c = Number(n).toString(16).toUpperCase();
		if (c.length == 1) c = '0' + c;

		line += c + " ";

		c = (n >= 0x20 && n < 0x80 ? String.fromCharCode(n) : '.');
		cs += c;
	}

	if (line) {
		if (lines.length > 0) {
			while (line.length < (3 * num)) {
				line += "   ";
			}
		}

		lines.push(line + cs);
	}

	lines.unshift('ArrayBuffer (' + chunk.byteLength + " bytes)");
	return lines;
}

function timestamp() {
	let now = new Date();
	let ms = '00' + now.getMilliseconds();
	ms = ms.substring(ms.length - 3);

	return now.toTimeString().split(' ')[0] + '.' + ms;
}

export class Logger {
	constructor(namespace, colorize = false) {
		this.namespace = namespace;
		this.colorize = colorize;
	}

	log(...args) {
		out(this.header() + this.text(args));
	}

	header() {
		let header = "[";
		header += timestamp();
		if (this.namespace) header += ' | ' + this.namespace;
		header += "] ";

		if (this.colorize) {
			let colorize = require("utils/colorize");
			header = colorize.colored(header, colorize.CYAN);
		}

		return header;
	}

	text(args) {
		let body = "";
		var separator;
		for (var i = 0; i < args.length; i++) {
			if (separator) body += separator;

			var v = arguments[i];
			if (typeof v == 'object') {
				body += stringify(v).trim();
				separator = "\n";
			} else {
				body += ('' + v).trim();
				separator = " ";
			}
		}
		return body;
	}
}

const logger = new Logger();

export function log() {
	logger.log.apply(logger, arguments);
}

export default {
	Logger,
	log,
	out,
	stringify,

	setPrinter(printer) {
		output = printer;
	}
};


