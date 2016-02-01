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
class Grammar {
	static nameToID() @ "xs_nameToID"
	static assignValue() @ "xs_assignValue"
	constructor(it) {
		this.infoset = it;
		this.namespaces = new Map;
		this.objects = new Map;
		this.roots = new Map;
		this.namespaces.set(null, null);
	}	
	link() {
		var iterator = this.objects.values();
		for (;;) {
			var result = iterator.next();
			if (result.done)
				break;
			result.value.inherit(this, null); 
		}
		var iterator = this.roots.values();
		for (;;) {
			var result = iterator.next();
			if (result.done)
				break;
			result.value.linkProperty(Grammar.ruleSet, ["*"]); 
		}
	}
	parse(buffer, path) {
		var document = this.infoset.scan(buffer, path);
		return Grammar.ruleSet.parseElement(document.element, null);
	}
	reportError(path, line, message) {
		console.log(path + ":" + line + ":", message);
	}
	dump() {
		Grammar.ruleSet.dump();
	}
	makeObject(prototype) {
		var object = this.objects.get(prototype);
		if (!object) {
			if (prototype instanceof Object)
				object = new Grammar.Object(prototype);
			else
				object = new Grammar.Data(prototype);
			this.objects.set(prototype, object);
		}
		return object;
	}
	array(pattern, ...prototypes) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ATTRIBUTE_FLAG | Grammar.PI_FLAG | Grammar.ROOT_FLAG, true);
		return new Grammar.Array(path, prototypes.map(prototype => this.makeObject(prototype)));
	}
	boolean(pattern) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ROOT_FLAG, true);
		return new Grammar.Property(path, Grammar.ioBoolean);
	}
	chunk(pattern) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ROOT_FLAG, true);
		return new Grammar.Property(path, Grammar.ioChunk);
	}
	custom(pattern, io) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		if (!io)
			throw new SyntaxError("no io parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ROOT_FLAG, true);
		return new Grammar.Property(path, io);
	}
	data(instance, pattern = "", property) {
		var data = this.makeObject(instance);
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ATTRIBUTE_FLAG | Grammar.PI_FLAG, false);
		data.path = path;
		data.property = property;
	}
	namespace(uri, prefix = null) {
		if (!uri)
			throw new SyntaxError("no uri parameter");
		this.namespaces.set(prefix, uri);
	}
	number(pattern) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ROOT_FLAG, true);
		return new Grammar.Property(path, Grammar.ioNumber);
	}
	object(prototype, pattern = "", properties = {}) {
		if (!prototype)
			throw new SyntaxError("no prototype parameter");
		var object = this.makeObject(prototype);
		if (object.path)
			throw new TypeError("prototype has already a pattern");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ATTRIBUTE_FLAG | Grammar.PI_FLAG, false);
		if (path.flags & Grammar.ROOT_FLAG)
			this.roots.set(prototype, object);
		object.path = path;
		object.properties = properties;	
		return object;
	}
	reference(pattern, ...prototypes) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ATTRIBUTE_FLAG | Grammar.PI_FLAG | Grammar.ROOT_FLAG, true);
		return new Grammar.Reference(path, prototypes.map(prototype => this.makeObject(prototype)));
	}
	string(pattern) {
		if (!pattern)
			throw new SyntaxError("no pattern parameter");
		var path = new Grammar.Path(pattern, this.namespaces, Grammar.ROOT_FLAG, true);
		return new Grammar.Property(path, Grammar.ioString);
	}
}

Grammar.ioBoolean = {
	parse(data) {
		return (data == "true") ? true : false;
	},
	toString() { return "ioBoolean" }
};
Grammar.ioChunk = {
	parse(data) {
		return new Chunk(data);
	},
	toString() { return "ioChunk" }
};
Grammar.ioNumber = {
	parse(data) {
		return parseFloat(data);
	},
	toString() { return "ioNumber" }
};
Grammar.ioString = {
	parse(data) {
		return data;
	},
	toString() { return "ioString" }
};

Grammar.Rule = class extends Array {
	constructor(io, names) {
		super();
		this.ruleSet = null;
		this.io = io;
		if (names) {
			var c = this.length = names.length;
			this.fill();
			for (var i = 0; i < c; i++)
				this[i] = Grammar.nameToID(names[i]);
		}
	}
}

Grammar.RuleSet = class {
	constructor() {
		this.dumped = false;
		this.attributeRules = {};
		this.dataRule = null;
		this.defaultRules = [];
		this.elementRules = {};
		this.piRules = {};
	}
	getRule(rules, step) {
		var namespace = step.namespace;
		var name = step.name;
		if (namespace in rules) {
			rules = rules[namespace];
			if (name in rules)
				return rules[name];
		}
	}
	getAttributeRule(step) {
		return this.getRule(this.attributeRules, step);
	}
	getDataRule() {
		return this.dataRule;
	}
	getElementRule(step) {
		return this.getRule(this.elementRules, step);
	}
	getPIRule(step) {
		return this.getRule(this.piRules, step);
	}
	parseAttribute(attribute, instance) {
		var rule = this.getRule(this.attributeRules, attribute);
		if (!rule)
			return;
		Grammar.assignValue(rule, instance, rule.io.parse(attribute.value));
	}
	parseData(data, instance) {
		var rule = this.dataRule;
		if (!rule)
			return;
		Grammar.assignValue(rule, instance, rule.io.parse(data.value));
	}
	parseDefaults(instance) {
		for (var rule of this.defaultRules)
			Grammar.assignValue(rule, instance, Object.create(rule.io));
	}
	parseElement(element, instance) {
		var rule = this.getRule(this.elementRules, element);
		if (!rule)
			return;
		if (rule.io) {
			var value = Object.create(rule.io);
			value.__xs__path = element.path;
			value.__xs__line = element.line;
			Grammar.assignValue(rule, instance, value);
			instance = value;
		}
		var ruleSet = rule.ruleSet;
		if (ruleSet) {
			ruleSet.parseDefaults(instance);
			for (var attribute of element.attributes)
				ruleSet.parseAttribute(attribute, instance);
			for (var child of element.children) {
				if (child.kind == 1)
					ruleSet.parseElement(child, instance);
				else if (child.kind == 3)
					ruleSet.parseData(child, instance);
				else if (child.kind == 5)
					ruleSet.parsePI(child, instance);
			}
		}
		return instance;
	}
	parsePI(pi, instance) {
		var rule = this.getRule(this.piRules, pi);
		if (!rule)
			return;
		Grammar.assignValue(rule, instance, rule.io.parse(pi.value));
	}
	putAttributeRule(step, rule) {
		return this.putRule(this.attributeRules, step, rule);
	}
	putDataRule(rule) {
		this.dataRule = rule;
		return rule;
	}
	putDefaultRule(rule) {
		this.defaultRules.push(rule);
		return rule;
	}
	putElementRule(step, rule) {
		var rule = this.putRule(this.elementRules, step, rule);
		return rule;
	}
	putPIRule(step, rule) {
		return this.putRule(this.piRules, step, rule);
	}
	putRule(rules, step, rule) {
		var namespace = step.namespace;
		var name = step.name;
		if (namespace in rules)
			rules = rules[namespace];
		else
			rules = rules[namespace] = {};
		rules[name] = rule;
		return rule;
	}
	dump() {
		if (this.dumped) return;
		this.dumped = true
		console.log("ATTRIBUTES");
		this.dumpRules(this.attributeRules);
		if (this.dataRule) {
			console.log("DATA");
			console.log("\t", this.dataRule.io, this.dataRule.names)
		}
		console.log("DEFAULTS");
		this.defaultRules.forEach(rule => { 
			console.log("\t", rule.io, rule)
		});
		console.log("ELEMENTS");
		this.dumpRules(this.elementRules);

		for (var namespace in this.elementRules) {
			var nameRules = this.elementRules[namespace];
			for (var name in nameRules) {
				var rule = nameRules[name];
				if (rule.ruleSet && !rule.ruleSet.dumped) {
					console.log("####", name);
					rule.ruleSet.dump(); 
				}
			}
		}
	}
	dumpRules(namespaceRules) {
		for (var namespace in namespaceRules) {
			var nameRules = namespaceRules[namespace];
			for (var name in nameRules) {
				var rule = nameRules[name];
				console.log("\t", name, rule.io, rule)
			}
		}
	}
}

Grammar.ATTRIBUTE_FLAG = 1;
Grammar.DEFAULT_FLAG = 2;
Grammar.PI_FLAG = 4;
Grammar.ROOT_FLAG = 8;
Grammar.SKIP_FLAG = 16;

Grammar.Step = class {
	constructor(namespace, name, prefix) {
		this.namespace = namespace;
		this.name = name;
		this.prefix = prefix;
	}
}

Grammar.DefaultStep = class  {
	constructor() {
		this.name = ".";
	}
}

Grammar.Path = class {
	constructor(pattern, namespaces, errors, force) {
		var names, c, i, name, split, namespace, prefix;
		var flags = 0;
		var steps = [];

		if (pattern == "")
			flags |= Grammar.SKIP_FLAG;
		else if (pattern == ".") {
			flags |= Grammar.DEFAULT_FLAG;
			steps.push(new Grammar.DefaultStep);
		}
		else {
			if (pattern.charAt(0) == "/") {
				flags |= Grammar.ROOT_FLAG;
				names = pattern.split("/");
				names.shift();
			}
			else
				names = pattern.split("/");
			c = names.length;
			for (i = 0; i < c; i++) {
				name = names[i];
				if (name == ".") {
					if (i < (c - 1))
						throw new SyntaxError("invalid pattern");
					flags |= Grammar.DEFAULT_FLAG;
					steps.push(new Grammar.DefaultStep());
				}
				else if (name == "..") 
					throw new SyntaxError("invalid pattern");
				else if (name.charAt(0) == "@") {
					if (i < (c - 1))
						throw new SyntaxError("invalid pattern");
					name = name.substring(1, name.length);
					flags |= Grammar.ATTRIBUTE_FLAG;
					split = name.split(":");
					if (split.length == 2) {
						prefix = split[0];
						namespace = namespaces.get(prefix);
						if (!namespace)
							throw new SyntaxError("invalid pattern");
						name = split[1];
					}
					else {
						prefix = null;
						namespace = null;
					}
				}
				else if (name.charAt(0) == "?") {
					if (i < (c - 1))
						throw new SyntaxError("invalid pattern");
					name = name.substring(1, name.length);
					flags |= Grammar.PI_FLAG;
					split = name.split(":");
					if (split.length == 2) {
						prefix = split[0];
						namespace = namespaces.get(prefix);
						if (!namespace)
							throw new SyntaxError("invalid pattern");
						name = split[1];
					}
					else {
						prefix = "";
						namespace = namespaces.get(null);
					}
				}
				else {
					split = name.split(":");
					if (split.length == 2) {
						prefix = split[0];
						namespace = namespaces.get(prefix);
						if (!namespace)
							throw new SyntaxError("invalid pattern");
						name = split[1];
					}
					else {
						prefix = "";
						namespace = namespaces.get(null);
					}
				}
				steps.push(new Grammar.Step(namespace, name, prefix));
			}
		}
		if (flags & errors)
			throw new SyntaxError("invalid pattern");
		if (!flags & force) {
			flags |= Grammar.DEFAULT_FLAG;
			steps.push(new Grammar.DefaultStep());
		}
		this.flags = flags;
		this.steps = steps;
	}
}

Grammar.Property = class  {
	constructor(path, io) {
		this.path = path;
		this.io = io;
	}
	linkError(rule) {
		throw new SyntaxError("Rule already exists");
	}
	linkProperty(ruleSet, names) {
		var path = this.path;
		if (!path)
			return;
		if (path.flags & Grammar.SKIP_FLAG)
			return;
		var steps = path.steps;
		var c = steps.length - 1;
		for (var i = 0; i < c; i++) {
			var step = steps[i];
			var rule = ruleSet.getElementRule(step);
			if (!rule) {
				rule = new Grammar.Rule();
				rule.ruleSet = new Grammar.RuleSet;
				ruleSet.putElementRule(step, rule);
			}
			ruleSet = rule.ruleSet;
		}
		this.linkPropertyStep(ruleSet, names, steps[i]);
	}
	linkPropertyStep(ruleSet, names, step) {
		var path = this.path;
		if (path.flags & Grammar.ATTRIBUTE_FLAG) {
			var rule = ruleSet.getAttributeRule(step);
			if (rule)
				this.linkError(rule);
			else
				this.rule = ruleSet.putAttributeRule(step, new Grammar.Rule(this.io, names));
		}
		else if (path.flags & Grammar.DEFAULT_FLAG) {
			var rule = ruleSet.getDataRule(step);
			if (rule)
				this.linkError(rule);
			else
				this.rule = ruleSet.putDataRule(new Grammar.Rule(this.io, names));
		}
		else if (path.flags & Grammar.PI_FLAG) {
			var rule = ruleSet.getPIRule(aPart);
			if (rule)
				this.linkError(rule);
			else
				this.rule = ruleSet.putPIRule(step, new Grammar.Rule(this.io, names));
		}
	}
}

Grammar.Data = class extends Grammar.Property {
	constructor(instance) {
		super(null, null);
		this.instance = instance;
		this.property = null;
		this.references = [];
	}
	inherit(root, instanceObject) {
	}
	linkPropertyStep(ruleSet, names, step) {
		var rule = ruleSet.getElementRule(step);
		if (!rule) {
			rule = new Grammar.Rule();
			rule.ruleSet = new Grammar.RuleSet();
			ruleSet.putElementRule(step, rule);
		}
		this.property.linkProperty(rule.ruleSet, names);
	}
}

Grammar.Reference = class extends Grammar.Property {
	constructor(path, objects) {
		super(path, null);
		this.objects = objects;
		objects.forEach(object => {
			object.references.push(this);
		});
	}
	linkPropertyStep(ruleSet, names, step) {
		var c = this.objects.length;
		for (var i = 0; i < c; i++)
			this.objects[i].linkProperty(ruleSet, names);
	}
	putObject(object) {
		if (this.objects.indexOf(object) < 0)
			this.objects.push(object);
	}
}

Grammar.Array = class extends Grammar.Reference {
	constructor(path, objects) {
		super(path, objects);
	}
	linkPropertyStep(ruleSet, names, step) {
		var rule = ruleSet.putDefaultRule(new Grammar.Rule(Array.prototype, names));
		names = names.concat("@");
		var c = this.objects.length;
		for (var i = 0; i < c; i++)
			this.objects[i].linkProperty(ruleSet, names);
	}
}

Grammar.Object = class extends Grammar.Property {
	constructor(prototype) {
		super(null, null);
		this.inherited = false;
		this.prototype = prototype;
		this.properties = {};
		this.prototypeObject = null;
		this.references = [];
		this.ruleSet = null;
	}
	inherit(root, instanceObject) {
		if (instanceObject) {
			this.references.forEach(reference => {
				reference.putObject(instanceObject);
			});
			if (this.prototypeObject)
				this.prototypeObject.inherit(root, instanceObject);
		}
		if (!this.inherited) {
			this.inherited = true;
			var prototype = Object.getPrototypeOf(this.prototype);
			while (prototype) {
				var object = root.objects.get(prototype);
				if (object)
					break;
				prototype = Object.getPrototypeOf(prototype);
			}
			if (object) {
				object.inherit(root, this);
				this.prototypeObject = object;
				for (var id in object.properties) {
					if (id in this.properties)
						continue;
					this.properties[id] = object.properties[id];
				}
			}
		}
	}
	linkProperties(ruleSet, names) {
		var properties = this.properties;
		for (var name in properties) {
			var property = properties[name];
			property.linkProperty(ruleSet, names.concat(name));

		}
	}
	linkPropertyStep(ruleSet, names, step) {
		var rule;
		if (this.path.flags & Grammar.DEFAULT_FLAG) {
			rule = ruleSet.putDefaultRule(new Grammar.Rule(this.prototype, names));
			this.linkProperties(ruleSet, names);
		}
		else {
			rule = ruleSet.getElementRule(step);
			if (rule) {
				this.linkError(rule);
			}
			else {
				rule = ruleSet.putElementRule(step, new Grammar.Rule(this.prototype, names));
				if (!this.ruleSet) {
					this.ruleSet = new Grammar.RuleSet;
					this.linkProperties(this.ruleSet, []);
				}
				rule.ruleSet = this.ruleSet;
			}
		}
	}
}

Grammar.ruleSet = new Grammar.RuleSet;

export default Grammar;
