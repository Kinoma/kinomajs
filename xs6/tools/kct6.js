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
var CRYPTO = require('crypto');
var FS = require('fs'); 
var HTTP = require('http'); 
var PATH = require('path'); 
var OS = require('os')
var spawn = require('child_process').spawn;

var temporaryDirectory = "/tmp/";

var Queue = function() {
	this.queue = null;
	this.tasks = [];
}
Queue.prototype = Object.create(Object.prototype, {
	end: { value:function() {
		this.queue.run();
	}},
	execute: { value: function(queue) {
		this.queue = queue;
		this.run();
	}},
	push: { value: function(task) {
		this.tasks.push(task);
	}},
	run: { value: function() {
		var tasks = this.tasks;
		if (tasks.length == 0) {
			this.end();
		}
		else {
			var task = tasks.shift();
			task.execute(this);
		}
	}},
});

var ChecksumTask = function(path) {
	this.path = path;
}
ChecksumTask.prototype = Object.create(Object.prototype, {
	execute: { value: function(queue) {
		var hash = CRYPTO.createHash('md5');
		var path = this.path;
		var stream = FS.createReadStream(path);
		stream.on('data', function (data) {
			hash.update(data, 'utf8')
		})
		stream.on('end', function () {
			var checksumPath = path.slice(tool.output.length + 1);
			var uploadPath = "applications/" + path.slice(temporaryDirectory.length);
			var former = (checksumPath in tool.manifest.checksums) ? tool.manifest.checksums[checksumPath] : "";
			var current = hash.digest('hex');
			if (former != current) {
				tool.manifest.checksums[checksumPath] = current;
				tool.manifestChanged = true;
				queue.push(new UploadTask(path, uploadPath));
			}
  			queue.run();
		})
	}},
});

var CommandTask = function(command, parameters) {
	this.command = command;
	this.parameters = parameters;
}
CommandTask.prototype = Object.create(Object.prototype, {
	execute: { value: function(queue) {
		console.log("# " + this.command + " " + this.parameters.join(" "));
		var child = spawn(this.command, this.parameters, { cwd: ".", detached: true, env: process.env, stdio: 'inherit'  });
		child.on('exit', function (code) {
			if (code != 0)
  				process.exit(code);
  			else
  				queue.run();
		});
	}},
});

var DelayTask = function(duration) {
	this.duration = duration;
}
DelayTask.prototype = Object.create(Object.prototype, {
	execute: { value: function(queue) {
		console.log("# delay " + this.duration);
		setTimeout(function (code) { queue.run(); }, this.duration);
	}},
});

var Directory = function(path, files) {
	this.name = PATH.basename(path);
	this.path = path;
	this.date = 0;
	this.files = files;
}
Directory.prototype = Object.create(Object.prototype, {
	type: { value: "Directory" },
	clone: { value: function(queue, path) {
		queue.push(new CommandTask("mkdir", [ path ]));
		return new Directory(path, []);
	}},
	compare: { value: function(from) {
		return this.name.localeCompare(from.name);
	}},
	remove: { value: function(queue) {
		this.files.forEach(function(file) {
			file.remove(queue);
		});
		queue.push(new CommandTask("rmdir", [ this.path ]));
	}},
	sync: { value: function(queue, from) {
		var path = this.path + "/";
		var fromFiles = from.files;
		var fromLength = fromFiles.length;
		var toFiles = this.files;
		var toLength = toFiles.length;
		var toIndex = 0;
		for (var fromIndex = 0; fromIndex < fromLength; fromIndex++) {
			var fromFile = fromFiles[fromIndex];
			var toFile = null;
			for (; toIndex < toLength; toIndex++) {
				toFile = toFiles[toIndex];
				var delta = toFile.compare(fromFile);
				if (delta == 0)
					break; 
				if (delta > 0) {
					toFile = null;
					break; 
				}
				toFile.remove(queue);
			}
			if (toFile) {
				if (toFile.type != fromFile.type) {
					toFile.remove(queue);
					toFile = fromFile.clone(queue, path + fromFile.name);
					toFiles[toIndex] = toFile;
				}
			}
			else {
				toFile = fromFile.clone(queue, path + fromFile.name);
				toFiles.splice(toIndex, 0, toFile);
				toLength++;
			}
			toIndex++;
			toFile.sync(queue, fromFile);
		}
	}},
	trace: { value: function(depth) {
		console.log(depth + this.name);
		depth += " ";
		this.files.forEach(function(file) {
			file.trace(depth);
		});
	}},
});

var File = function(path, date) {
	this.extension = PATH.extname(path);
	this.name = PATH.basename(path);
	this.path = path;
	this.date = date;
}
File.prototype = Object.create(Object.prototype, {
	type: { value: "File" },
	clone: { value: function(queue, path) {
		return new File(path, 0);
	}},
	compare: { value: function(from) {
		return this.name.localeCompare(from.name);
	}},
	remove: { value: function(queue) {
		queue.push(new CommandTask("rm", [ this.path ]));
		var checksumPath = this.path.slice(tool.output.length + 1);
		var uploadPath = "applications/" + this.path.slice(temporaryDirectory.length);
		if (checksumPath in tool.manifest.checksums) {
			delete tool.manifest.checksums[checksumPath];
			tool.manifestChanged = true;
			queue.push(new UnloadTask(uploadPath));
		}
	}},
	sync: { value: function(queue, from) {
		if (this.date < from.date) {
			if (this.extension == ".jsb") {
				if (from.extension == ".xml") {
					var temporary = PATH.dirname(this.path) + "/" + PATH.basename(this.path, ".jsb") + ".js";
					queue.push(new CommandTask("kpr2js6", [ from.path, "-o", PATH.dirname(temporary) ]));
					queue.push(new CommandTask("xsc6", [ "-b", "-d", temporary, "-o", PATH.dirname(temporary) ]));
					queue.push(new CommandTask("rm", [ temporary ]));
				}
				else 
					queue.push(new CommandTask("xsc6", [ "-b", "-d", from.path, "-o", PATH.dirname(this.path) ]));
			}
			else
				queue.push(new CommandTask("cp", [ from.path, this.path ]));
		}
		queue.push(new ChecksumTask(this.path));
	}},
	trace: { value: function(depth) {
		console.log(depth + this.name);
	}},
});

var FSTask = function(path) {
	this.path = path;
}
FSTask.prototype = Object.create(Object.prototype, {
	recurseFile: { value: function(path, stat) {
		return new File(path, stat.mtime);
	}},
	recurseFiles: { value: function(path) {
		var result;
		var stat = FS.statSync(path);
		if (stat.isDirectory()) {
			var names = FS.readdirSync(path);
			var files = [];
			names.forEach(function(name) {
				if (name[0] != ".")
					files.push(this.recurseFiles(path + "/" + name));
			}, this);
			return new Directory(path, files);
		}
		return this.recurseFile(path, stat);
	}},
});

var CleanTask = function(path) {
	this.path = path;
}
CleanTask.prototype = Object.create(FSTask.prototype, {
	execute: { value: function(queue) {
		if (FS.existsSync(this.path)) {
			var directory = this.recurseFiles(this.path);
			directory.remove(queue);
		}
		queue.run();
	}},
});

var ScanTask = function(path) {
	this.path = path;
}
ScanTask.prototype = Object.create(FSTask.prototype, {
	recurseFile: { value: function(path, stat) {
		result = new File(path, stat.mtime);
		if (path != tool.skip) {
			if (result.extension == ".js") {
				result.name = PATH.basename(path, result.extension);
				result.name += ".jsb";
			}
			else if (result.extension == ".xml") {
				var buffer = FS.readFileSync(path).toString();
				if ((buffer.indexOf("xmlns=\"http://www.kinoma.com/kpr/1\"") > 0)
						&& ((buffer.indexOf("<program") >= 0) || (buffer.indexOf("<module") >= 0) || (buffer.indexOf("<shell") >= 0))) {
					result.name = PATH.basename(path, result.extension);
					result.name += ".jsb";
				}
			}
		}
		return result;
	}},
	execute: { value: function(queue) {
		tool.root = this.recurseFiles(this.path);
		queue.push(new SyncTask(tool.output));
		queue.run();
	}},
});

var SyncTask = function(path) {
	this.path = path;
}
SyncTask.prototype = Object.create(FSTask.prototype, {
	execute: { value: function(queue) {
		var root = this.recurseFiles(this.path);
		root.sync(queue, tool.root);
		queue.run();
	}},
});


var HTTPTask = function() {
}
HTTPTask.prototype = Object.create(Object.prototype, {
	onData: { value: function(queue, data) {
	}},
	request: { value: function(queue, options) {
		console.log("# http://" + options.auth + "@" + options.hostname + ":" + options.port + options.path);
		var self = this;
		var request = HTTP.request(options, function(response) {
			response.on('data', function(chunk) {
				self.onData(queue, chunk);
			});
			response.on('end', function () {
				if ((200 <= response.statusCode) && (response.statusCode < 300))
					queue.run();
				else {
					console.log("### " + response.statusCode);
					process.exit(1);
				}
			});
		});
		request.on('error', function(error) {
			console.log("### " + error.code);
			process.exit(1);
		});
		return request;
		if (data)
			request.write(data);
		request.end();
	}},
});

var CheckTask = function() {
}
CheckTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/app/check?id=" + encodeURIComponent(tool.id)
		}).end();
	}},
	onData: { value: function(queue, data) {
		var result = JSON.parse(data);
		if (result.success) {
			var path = "applications/" + tool.di;
			if (result.url.indexOf(path) < 0)
				queue.push(new UninstallTask(tool.id));
			else {
				var temporary = result.url.indexOf("kdt/cache") >= 0;
				if (tool.temporary) 
					tool.temporary = temporary;
				else if (temporary)
					queue.push(new UninstallTask(tool.id));
			}	
		}
	}},
});

var CloseTask = function(tool) {
}
CloseTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/close?id=" + encodeURIComponent(tool.id)
		}).end();
	}},
});

var ConnectTask = function() {
}
ConnectTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/connect?host=" + tool.address + "&port=5002"
		}).end();
	}},
});

var GetManifestTask = function(di) {
	this.di = di;
}
GetManifestTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/manifest?path=" + encodeURIComponent("applications/" + this.di) + "&temporary=" + tool.temporary
		}).end();
	}},
	onData: { value: function(queue, data) {
		tool.manifest = JSON.parse(data);
		if (!("checksums" in tool.manifest))
			tool.manifest.checksums = {};
	}},
});

var LaunchTask = function(id) {
	this.id = id;
}
LaunchTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/launch?id=" + encodeURIComponent(this.id)
		}).end();
	}},
});

var PingTask = function(id) {
	this.id = id;
}
PingTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/ping"
		}).end();
	}},
});

var PutManifestTask = function(di) {
	this.di = di;
}
PutManifestTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		if (tool.manifestChanged) {
			var buffer = JSON.stringify(tool.manifest);
			var request = this.request(queue, {
				auth: "kinoma:" + tool.password,
				hostname: tool.host,
				port: "10000",
				method : 'PUT',
				headers : {
				  'Content-Length' : buffer.length
				},
				path: "/manifest?path=" + encodeURIComponent("applications/" + this.di) + "&put=true&temporary=" + tool.temporary
			});
			request.write(buffer);
			request.end();
		}
		else
			queue.run();
	}},
});

var RegisterTask = function(di) {
	this.di = di;
}
RegisterTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/install?app=" + encodeURIComponent("applications/" + this.di + "/application.xml") + "&temporary=" + tool.temporary
		}).end();
	}},
});

var UninstallTask = function() {
}
UninstallTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/uninstall?id=" + encodeURIComponent(tool.id)
		}).end();
	}},
});

var UnloadTask = function(path) {
	this.path = path;
}
UnloadTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			path: "/unload?path=" + encodeURIComponent(this.path) + "&temporary=" + tool.temporary
		}).end();
	}},
});

var UploadTask = function(path, uploadPath) {
	this.path = path;
	this.uploadPath = uploadPath;
}
UploadTask.prototype = Object.create(HTTPTask.prototype, {
	execute: { value: function(queue) {
		var path = this.path;
		var stat = FS.statSync(path);
		var request = this.request(queue, {
			auth: "kinoma:" + tool.password,
			hostname: tool.host,
			port: "10000",
			method : 'PUT',
			headers : {
			  'Content-Length' : stat.size
			},
			path: "/upload?path=" + encodeURIComponent(this.uploadPath) + "&temporary=" + tool.temporary
		});
		FS.createReadStream(path, { bufferSize: 4 * 1024 }).pipe(request);
	}},
});

// ACTIONS

var CleanTask = function(tool) {
}
CleanTask.prototype = Object.create(FSTask.prototype, {
	execute: { value: function(queue) {
		tool.manifest = { checksums: {} };
		var root = this.recurseFiles(queue.output);
		root.remove(queue);
		queue.run();
	}},
});

var DeleteTask = function(tool) {
}
DeleteTask.prototype = Object.create(Object.prototype, {
	execute: { value: function(queue) {
		queue.push(new PingTask(tool));
		queue.push(new CloseTask(tool));
		queue.push(new DelayTask(1000));
		queue.push(new UninstallTask());
		queue.run();
	}},
});

var InstallTask = function(tool) {
}
InstallTask.prototype = Object.create(Object.prototype, {
	execute: { value: function(queue) {
		tool.temporary = false;
		queue.push(new PingTask(tool));
		queue.push(new CloseTask(tool));
		queue.push(new DelayTask(1000));
		var task = new Queue();
		queue.push(task)
		task.push(new CheckTask());
		queue.push(new GetManifestTask(tool.di));
		var task = new Queue();
		queue.push(task)
		task.push(new ScanTask(tool.input));
		queue.push(new PutManifestTask(tool.di));
		queue.push(new RegisterTask(tool.di));
		queue.run();
	}},
});

var RunTask = function(tool) {
}
RunTask.prototype = Object.create(Object.prototype, {
	execute: { value: function(queue) {
		tool.temporary = true;
		queue.push(new PingTask(tool));
		queue.push(new CloseTask(tool));
		queue.push(new DelayTask(1000));
		var task = new Queue();
		queue.push(task)
		task.push(new CheckTask());
		queue.push(new GetManifestTask(tool.di));
		var task = new Queue();
		queue.push(task)
		task.push(new ScanTask(tool.input));
		queue.push(new PutManifestTask(tool.di));
		queue.push(new RegisterTask(tool.di));
		queue.push(new ConnectTask(tool.id));
		queue.push(new LaunchTask(tool.id));
		queue.run();
	}},
});

var Tool = function(args) {
	Queue.call(this);
	var clean = false;
	this.host = process.env.KINOMA_CREATE_HOST;
	this.input = ".";
	this.manifestChanged = false;
	this.password = process.env.KINOMA_CREATE_PASSWORD;
	this.temporary = false;
	var c = args.length, i;
	if (c < 3) {
		console.log("### No action");
		process.exit(1);
	}
	var tasks = {
		close: CloseTask,
		clean: CleanTask,
		"delete": DeleteTask,
		install: InstallTask,
		ping: PingTask,
		run: RunTask,
	};
	var task = args[2];
	if (task in tasks)
		task = tasks[task];
	else {
		console.log("### Action not found");
		process.exit(1);
	}
	for (i = 3; i < c; i += 1) {
		switch (args[i]) {
		case '-h':
			i += 1;
			if (i < c)
				this.host = args[i];
			break;
		case '-p':
			i += 1;
			if (i < c)
				this.password = args[i];
			break;
		default:
			this.input = args[i];
			break;
		}
	}
	var interfaces = OS.networkInterfaces();
	var addresses = [];
	for (k in interfaces) {
		for (k2 in interfaces[k]) {
			var address = interfaces[k][k2];
			if (address.family == 'IPv4' && !address.internal) {
				addresses.push(address.address)
			}
		}
	}
	this.address = addresses[0];
	if (!this.host) {
		console.log("# No host, defaults to " + this.address);
		this.host = this.address;
	}
	if (!this.password) {
		console.log("# No password, defaults to kinoma");
		this.password = "kinoma";
	}
	var path = this.input + "/application.xml";
	if (!FS.existsSync(path)) {
		console.log("### File not found: " + this.input + "/application.xml");
		process.exit(1);
	}
	path = FS.realpathSync(path);
	var buffer = FS.readFileSync(path);
	buffer = buffer.toString();
	var offset = buffer.indexOf("<application");
	if (offset < 0) {
		console.log("### Element not found: application");
		process.exit(1);
	}
	offset = buffer.indexOf("id=\"", offset + 12);
	if (offset < 0) {
		console.log("### Attribute not found: id");
		process.exit(1);
	}
	offset += 4;
	this.id = buffer.slice(offset, buffer.indexOf("\"", offset));
	this.di = this.id.split(".").reverse().join(".");
	this.skip = path;
	this.input = PATH.dirname(path);	
	this.output = path = temporaryDirectory + this.di;
	this.manifest = null;
	if (!FS.existsSync(path))
		FS.mkdirSync(path);
				
	this.push(new task(this));
}
Tool.prototype = Object.create(Queue.prototype, {
	end: { value:function() {
	}},
});

var tool = new Tool(process.argv);
tool.run();
