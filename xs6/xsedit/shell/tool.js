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
import {
	model,
} from "shell/main";

var PATH = {
	basename(path, extension) {
		let from = path.lastIndexOf("/") + 1;
		let to = (extension && path.endsWith(extension)) ? path.length - extension.length : path.length;
		return path.slice(from, to);
	},
	dirname(path) {
		let slash = path.lastIndexOf("/");
		if (slash >= 0)
			return path.slice(0, slash);
		return path;
	},
	extname(path) {
		let dot = path.lastIndexOf(".");
		if (dot >= 0)
			return path.slice(dot);
		return "";
	},
	fromURI(url) {
		return Files.toPath(url);
	},
	toURI(path) {
		return Files.toURI(path);
	},
}

var console = {
	log(text) {
		shell.behavior.doLog(shell, text + "\n");
	},
}

class Task {
	constructor(context) {
		this.context = context;
	}
}

class Queue extends Task {
	constructor(context) {
		super(context);
		this.queue = null;
		this.tasks = [];
	}
	execute(queue) {
		this.queue = queue;
		this.step();
	}
	exit(result) {
		if (result)
			this.queue.exit(result);
		else
			this.queue.run();
	}
	push(task) {
		this.tasks.push(task);
	}
	run() {
		Promise.resolve(this).then(queue => {
			queue.step();
		});
	}
	step() {
		var tasks = this.tasks;
		if (tasks.length == 0) {
			this.exit(0);
		}
		else {
			var task = tasks.shift();
			var result;
			try {
				task.execute(this);
			}
			catch(e) {
				this.exit(1);
			}
		}
	}
}

class ChecksumTask extends Task {
	constructor(context, path) {
		super(context);
		this.path = path;
	}
	execute(queue) {
		var context = this.context;
		var path = this.path;
		var url = PATH.toURI(path);
		var buffer = Files.readChunk(url);
		var current = KPR.MD5(buffer);
		var checksumPath = path.slice(context.output.length + 1);
		var uploadPath = "applications/" + context.di + "/" + checksumPath;
		var former = (checksumPath in context.manifest.checksums) ? context.manifest.checksums[checksumPath] : "";
		if (former != current) {
			context.manifest.checksums[checksumPath] = current;
			context.manifestChanged = true;
			queue.push(new UploadTask(context, path, uploadPath));
		}
		queue.step();
	}
}

class CommandTask extends Task {
	constructor(context, command, parameters) {
		super(context);
		this.command = command;
		this.parameters = parameters;
	}
	execute(queue) {
		var context = this.context;
		var command = this.command;
		if (this.parameters)
			command += " " + this.parameters.map(parameter => parameter.replace(/ /g, "\\ ")).join(" ");
		console.log(command);
		this.exec = shell.execute(command, {
			callback: status => {
				if (status != 0)
					queue.exit(status);
				else
					queue.step();
			},
			directory:context.input,
			environment: {
				F_HOME:context.home,
				PATH:context.paths,
			},
			stderr: text => console.log(text),
			stdout: text => shell.behavior.doLog(shell, text),
		});
	}
}

class CreateApplicationTask extends Task {
	constructor(context, src, dst) {
		super(context);
		this.src = src;
		this.dst = dst;
	}
	execute(queue) {
		//console.log("# create application " + this.dst + " from " + this.src);
		let json = Files.readJSON(this.src);
		let buffer = `<?xml version="1.0" encoding="utf-8"?><application xmlns="http://www.kinoma.com/kpr/application/1" id="${ json.id }" program="${ json.Create.main }" title="${ json.title }"></application>`
		Files.writeText(this.dst, buffer);
		queue.run();
	}
}

Handler.Bind("/delay", class extends Behavior {
	onComplete(handler) {
		var message = handler.message;
		message.status = 200;
	}
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		handler.wait(query.duration);
	}
});

class DelayTask extends Task {
	constructor(context, duration) {
		super(context);
		this.duration = duration;
	}
	execute(queue) {
		//console.log("# delay " + this.duration);
		var message = new Message("/delay?duration=" + this.duration);
		message.invoke().then(message => {
			queue.step();
		});
	}
}

class Directory extends Task {
	constructor(context, path, files) {
		super(context);
		this.name = PATH.basename(path);
		this.path = path;
		this.date = 0;
		this.files = files;
		this.type = "Directory";
	}
	clone(queue, path) {
		var context = this.context;
		queue.push(new CommandTask(context, "mkdir", [ path ]));
		return new Directory(context, path, []);
	}
	compare(from) {
		return this.name.localeCompare(from.name);
	}
	remove(queue) {
		this.files.forEach(function(file) {
			file.remove(queue);
		});
		queue.push(new CommandTask(this.context, "rmdir", [ this.path ]));
	}
	sync(queue, from) {
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
		for (; toIndex < toLength; toIndex++) {
			toFile = toFiles[toIndex];
			toFile.remove(queue);
		}
	}
	trace(depth) {
		console.log(depth + this.name);
		depth += " ";
		this.files.forEach(function(file) {
			file.trace(depth);
		});
	}
}

class File extends Task {
	constructor(context, path, date) {
		super(context);
		this.extension = PATH.extname(path);
		this.name = PATH.basename(path);
		this.path = path;
		this.date = date;
		this.type = "File";
	}
	clone(queue, path) {
		return new File(this.context, path, 0);
	}
	compare(from) {
		return this.name.localeCompare(from.name);
	}
	remove(queue) {
		var context = this.context;
		queue.push(new CommandTask(context, "rm", [ this.path ]));
		var checksumPath = this.path.slice(context.output.length + 1);
		var uploadPath = "applications/" + context.di + "/" + checksumPath;
		if (checksumPath in context.manifest.checksums) {
			delete context.manifest.checksums[checksumPath];
			context.manifestChanged = true;
			queue.push(new UnloadTask(context, uploadPath));
		}
	}
	sync(queue, from) {
		var context = this.context;
		if (this.date < from.date) {
			if (this.extension == ".jsb") {
				if (from.extension == ".xml") {
					var temporary = PATH.dirname(this.path) + "/" + PATH.basename(this.path, ".jsb") + ".js";
					queue.push(new CommandTask(context, "xsr6", [ "-a", context.archive, "kpr2js", from.path, "-o", PATH.dirname(temporary) ]));
					queue.push(new CommandTask(context, "xsc6", [ "-b", "-d", temporary, "-o", PATH.dirname(temporary) ]));
					queue.push(new CommandTask(context, "rm", [ temporary ]));
				}
				else 
					queue.push(new CommandTask(context, "xsc6", [ "-b", "-d", from.path, "-o", PATH.dirname(this.path) ]));
			}
			else
				queue.push(new CommandTask(context, "cp", [ from.path, this.path ]));
		}
		queue.push(new ChecksumTask(context, this.path));
	}
	trace(depth) {
		console.log(depth + this.name);
	}
}

class FSTask extends Task {
	constructor(context, path) {
		super(context);
		this.path = path;
	}
	recurseFile(path, info) {
		return new File(this.context, path, info.date);
	}
	recurseFiles(path) {
		var context = this.context;
		var url = PATH.toURI(path);
		var info = Files.getInfo(url);
		if (info && (info.type == Files.directoryType)) {
			var files = [];
			var iterator = new Files.Iterator(url + "/");
			info = iterator.getNext();
			while (info) {
				files.push(this.recurseFiles(path + "/" + info.path));
				info = iterator.getNext();
			}
			return new Directory(context, path, files);
		}
		return this.recurseFile(path, info);
	}
}
/*
class CleanTask extends FSTask {
	constructor(context, path) {
		super(context, path);
	}
	execute(queue) {
		var path = this.path;
		var url = PATH.toURI(path);
		if (Files.exists(url)) {
			var directory = this.recurseFiles(path);
			directory.remove(queue);
		}
		queue.run();
	}
}
*/
class ScanTask extends FSTask {
	constructor(context, path) {
		super(context, path);
	}
	recurseFile(path, info) {
		var context = this.context;
		var result = new File(context, path, info.date);
		if (!context.source) {
			if (result.extension == ".js") {
				result.name = PATH.basename(path, result.extension);
				result.name += ".jsb";
			}
			else if (result.extension == ".xml") {
				var url = PATH.toURI(path);
				var buffer = Files.readText(url);
				if ((buffer.indexOf("xmlns=\"http://www.kinoma.com/kpr/1\"") > 0)
						&& ((buffer.indexOf("<program") >= 0) || (buffer.indexOf("<module") >= 0) || (buffer.indexOf("<shell") >= 0))) {
					result.name = PATH.basename(path, result.extension);
					result.name += ".jsb";
				}
			}
		}
		return result;
	}
	execute(queue) {
		var context = this.context;
		context.root = this.recurseFiles(this.path);
		queue.push(new SyncTask(context, context.output));
		queue.run();
	}
}

class SyncTask extends FSTask {
	constructor(context, path) {
		super(context, path);
	}
	execute(queue) {
		var root = this.recurseFiles(this.path);
		this.skip(queue, this.context.root, root);
		root.sync(queue, this.context.root);
		queue.run();
	}
	skip(queue, from, to) {
		var context = this.context;
		var fromFiles = from.files;
		var fromIndex = 0;
		var toFiles = to.files;
		var toIndex = 0;
		var fromApplicationFile = null;
		var fromProjectFile = null;
		var toApplicationFile = null;
		while (fromIndex < fromFiles.length) {
			var fromFile = fromFiles[fromIndex];
			if (fromFile.name == "application.xml") {
				fromApplicationFile = fromFile;
				fromFiles.splice(fromIndex, 1);
			}
			else if (fromFile.name == "project.json") {
				fromProjectFile = fromFile;
				fromFiles.splice(fromIndex, 1);
			}
			else
				fromIndex++;
		}
		while (toIndex < toFiles.length) {
			var toFile = toFiles[toIndex];
			if (toFile.name == "application.xml") {
				toApplicationFile = toFile;
				toFiles.splice(toIndex, 1);
			}
			else
				toIndex++;
		}
		if (context.createApplication) {
			var path = toApplicationFile ? toApplicationFile.path : this.path + "/application.xml";
			if (fromApplicationFile) {
				if (!toApplicationFile || (toApplicationFile.date < fromApplicationFile.date)) {
					queue.push(new CommandTask(context, "cp", [ fromApplicationFile.path, path ]));
					queue.push(new ChecksumTask(context, path));
				}
			}
			else if (fromProjectFile) {
				if (!toApplicationFile || (toApplicationFile.date < fromProjectFile.date)) {
					queue.push(new CreateApplicationTask(context, fromProjectFile.path, path));
					queue.push(new ChecksumTask(context, path));
				}
			}
			queue.push(new ChecksumTask(context, path));
		}
	}
}

Handler.Bind("/tool", class extends Behavior {
	onComplete(handler, message, result) {
	trace(message.url + " " + message.error + " " + message.status + "\n");
		var response = handler.message;
		response.error = message.error;
		response.responseText = message.responseText;
		response.status = message.status;
	}
	onInvoke(handler, request) {
		var options = parseQuery(request.query);
		var url = mergeURI(options.url, options.path);
		console.log(url);
		var message = new Message(url);
		if ("method" in options)
			message.method = options.method;
		if ("auth" in options)
			message.setRequestHeader("Authorization", "Basic " + options.auth);
		if ("body" in options)
			message.requestText = options.body;
		if (request.requestObject) {
			let headers = request.requestObject;
			for (var name in headers) {
				message.setRequestHeader(name, headers[name]);
// 				console.log("### " + name + " " + headers[name]);
			}
		}
		if ("file" in options)
			handler.upload(message, options.file);
		else
			handler.invoke(message, Message.TEXT);
	}
	onProgress(handler, message, offset, size) {
	}
});

class HTTPTask extends Task {
	constructor(context) {
		super(context);
	}
	onData(queue, data) {
	}
	onError(queue, message) {
		if (message.error)
			console.log("### " + message.error);
		else
			console.log("### " + message.status);
		queue.exit(1);
	}
	request(queue, options, headers) {
		var message = new Message("/tool?" + serializeQuery(options));
		message.requestObject = headers;
		message.invoke().then(message => {
			if ((200 <= message.status) && (message.status < 300)) {
				this.onData(queue, message.responseText);
				queue.step();
			}
			else
				this.onError(queue, message);
		}, message => {
			this.onError(queue, message);
		});
	}
}

class CheckTask extends HTTPTask {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./app/check?id=" + encodeURIComponent(context.id),
		});
	}
	onData(queue, data) {
		var context = this.context;
		var result = JSON.parse(data);
		if (result.success) {
			var path = "applications/" + context.di;
			if (result.url.indexOf(path) < 0)
				queue.push(new UninstallTask(context, context.id));
			else {
				var temporary = result.url.indexOf("kdt/cache") >= 0;
				if (context.temporary) 
					context.temporary = temporary;
				else if (temporary)
					queue.push(new UninstallTask(context, context.id));
			}	
		}
	}
}

class CloseTask extends HTTPTask {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./close?id=" + encodeURIComponent(context.id),
		});
	}
}

class ConnectTask extends HTTPTask {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./connect?host=" + context.debugHost + "&port=" + context.debugPort + "&title=" + encodeURIComponent(context.query.title),
		});
	}
}

class DisconnectTask extends HTTPTask {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./disconnect",
		});
	}
}

class GetManifestTask extends HTTPTask {
	constructor(context, di) {
		super(context);
		this.di = di;
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./manifest?path=" + encodeURIComponent("applications/" + this.di) + "&temporary=" + context.temporary,
		});
	}
	onData(queue, data) {
		var context = this.context;
		context.manifest = JSON.parse(data);
		if (!context.manifest)
			debugger
		if (!("checksums" in context.manifest))
			context.manifest.checksums = {};
	}
}

class LaunchTask extends HTTPTask {
	constructor(context, query) {
		super(context);
		this.query = query;
	}
	execute(queue) {
		var context = this.context;
		if (context.createApplication) {
			this.request(queue, {
				auth: context.authorization,
				url: context.url,
				path: "./launch",
				method:"POST",
				body:JSON.stringify({
					application: {
						id:context.id,
					},
					breakOnExceptions:true,
					temporary:context.temporary,
				})
			}, 
			{
				"Connection": "close"
			});
		}
		else {
			var query = serializeQuery(this.query);
			this.request(queue, {
				auth: context.authorization,
				url: context.url,
				path: "./launch?" + query,
			}, 
			{
				"Connection": "close"
			});
		}
	}
}

class PingTask extends HTTPTask {
	constructor(context, id) {
		super(context);
		this.id = id;
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./ping",
		});
	}
}

class PutManifestTask extends HTTPTask {
	constructor(context, di) {
		super(context);
		this.di = di;
	}
	execute(queue) {
		var context = this.context;
		if (context.manifestChanged) {
			var buffer = JSON.stringify(context.manifest);
			var request = this.request(queue, {
				auth: context.authorization,
				url: context.url,
				path: "./manifest?path=" + encodeURIComponent("applications/" + this.di) + "&put=true&temporary=" + context.temporary,
				method : 'PUT',
				body: buffer,
			}, 
			{
				'Content-Length' : buffer.length
			});
		}
		else
			queue.run();
	}
}

class RegisterTask extends HTTPTask {
	constructor(context, di) {
		super(context);
		this.di = di;
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./install?app=" + encodeURIComponent("applications/" + this.di + "/application.xml") + "&temporary=" + context.temporary,
		});
	}
}

class UninstallTask extends HTTPTask {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./uninstall?id=" + encodeURIComponent(context.id),
		});
	}
}

class UnloadTask extends HTTPTask {
	constructor(context, path) {
		super(context);
		this.path = path;
	}
	execute(queue) {
		var context = this.context;
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./unload?path=" + encodeURIComponent(this.path) + "&temporary=" + context.temporary,
		});
	}
}

class UploadTask extends HTTPTask {
	constructor(context, path, uploadPath) {
		super(context);
		this.path = path;
		this.uploadPath = uploadPath;
	}
	execute(queue) {
		var context = this.context;
		var path = this.path;
		var url = PATH.toURI(path);
		var info = Files.getInfo(url);
		this.request(queue, {
			auth: context.authorization,
			url: context.url,
			path: "./upload?path=" + encodeURIComponent(this.uploadPath) + "&temporary=" + context.temporary,
			method : 'PUT',
			file: url
		}, 
		{
			'Content-Length' : info.size,
		});
	}
}

// ACTIONS

class CallbackTask extends Task {
	constructor(context, callback) {
		super(context);
		this.callback = callback;
	}
	execute(queue) {
		this.callback();
		queue.run();
	}
}

class CleanTask extends Task {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		context.manifest = { checksums: {} };
		var root = this.recurseFiles(context.output);
		root.remove(queue);
		queue.run();
	}
}

class DeleteTask extends Task {
	constructor(context, callback) {
		super(context);
		this.callback = callback;
	}
	execute(queue) {
		var context = this.context;
		queue.push(new PingTask(context));
		queue.push(new CloseTask(context));
		queue.push(new DelayTask(context, 1000));
		queue.push(new UninstallTask(context));
		if (this.callback)
			queue.push(new CallbackTask(context, this.callback));
		queue.run();
	}
}

class InstallTask extends Task {
	constructor(context, callback) {
		super(context);
		this.callback = callback;
	}
	execute(queue) {
		var context = this.context;
		context.temporary = false;
		queue.push(new PingTask(context));
		queue.push(new CloseTask(context));
		queue.push(new DelayTask(1000));
		var task = new Queue(context);
		queue.push(task)
		task.push(new CheckTask(context));
		queue.push(new GetManifestTask(context, context.di));
		var task = new Queue(context);
		queue.push(task)
		task.push(new ScanTask(context, context.input, context.source));
		queue.push(new PutManifestTask(context, context.di));
		queue.push(new RegisterTask(context, context.di));
		if (this.callback)
			queue.push(new CallbackTask(context, this.callback));
		queue.run();
	}
}

class RunTask extends Task {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		var empty = queue.tasks.length == 0;
		//context.temporary = true;
		queue.push(new PingTask(context));
		queue.push(new DisconnectTask(context));
		var task = new Queue(context);
		queue.push(task)
		task.push(new CheckTask(context));
		queue.push(new GetManifestTask(context, context.di));
		var task = new Queue(context);
		queue.push(task)
		task.push(new ScanTask(context, context.input, context.source));
		queue.push(new PutManifestTask(context, context.di));
		queue.push(new RegisterTask(context, context.di));
		queue.push(new ConnectTask(context, context.id));
		queue.push(new LaunchTask(context, context.query));
		if (empty)
			queue.run();
	}
}

class RunContext {
	constructor(device, project, debug) {
		let url;
		this.createApplication = device.constructor.templateTag == "createSample";
		this.debugHost = device.debugHost;
		this.debugPort = debug.port;
		this.helperID = device.helperID;
		this.url = device.toolURL;
		this.authorization = device.authorization;
		this.id = project.id;
		this.di = this.id.split(".").reverse().join(".");
		this.source = "source" in project ? project.source : false;
		if ("query" in project)
			this.query = project.query;
		else
			this.query = {
				id: project.id,
				title: project.title,
			};
		this.input = PATH.fromURI(project.url).slice(0, -1);	
		url = mergeURI(model.cacheDirectory, this.di);
		Files.ensureDirectory(url + "/");
		this.output = PATH.fromURI(url);
		this.inputRoot = null;
		this.outputRoot = null;
		this.manifest = null;
		this.manifestChanged = false;
		this.temporary = true;
		
		this.archive = PATH.fromURI(mergeURI(shell.url, "../../tools/tools.xsa"))
		this.paths = PATH.fromURI(mergeURI(shell.url, "../../tools")) + ":/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin";
		
		HTTP.Cache.clear();
	}
}

class BuildContext {
	constructor(project) {
		this.title = project.title;
		this.id = project.id;
		this.di = this.id.split(".").reverse().join(".");
		this.home = PATH.fromURI(shell.behavior.home).slice(0, -1);	
		this.input = PATH.fromURI(project.url).slice(0, -1);	
		let url = mergeURI(model.cacheDirectory, this.di);
		Files.ensureDirectory(url + "/");
		this.output = PATH.fromURI(url);
		this.archive = PATH.fromURI(mergeURI(shell.url, "../../tools/tools.xsa"))
		this.paths = PATH.fromURI(mergeURI(shell.url, "../../tools")) + ":/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin";
	}
}

class MakeTask extends CommandTask {
	constructor(context, command, parameters) {
		super(context, command, parameters);
		this.output = "";
	}
	execute(queue) {
		var context = this.context;
		this.parameters = this.parameters.map(parameter => parameter.replace(/ /g, "\\ "));
		var command = this.command + " " + this.parameters.join(" ");
		console.log(command);
		this.exec = shell.execute(command, {
			callback: status => {
				if (status != 0)
					queue.exit(status);
				else {
					queue.tasks = this.split();
					queue.step();
				}
			},
			directory:context.input,
			environment: {
				F_HOME:context.home,
				PATH:context.paths,
			},
			stderr: text => console.log(text),
			stdout: text => this.log(text),
		});
	}
	log(text) {
		this.output += text;
	}
	split() {
		let queue = new Queue(this.context);
		let lines = this.output.split("\n")
		if (lines.length == 0)
			return lines;
		return lines.slice(0, -1).map(line => new CommandTask(this.context, line));
	}
}

class BuildTask extends Task {
	constructor(context) {
		super(context);
	}
	execute(queue) {
		var context = this.context;
		var empty = queue.tasks.length == 0;
		queue.push(new CommandTask(context, "osascript -e 'quit app \"" + context.title + "\"'"));
		queue.push(new CommandTask(context, "xsr6", [ "-a", context.archive, "kprconfig", "-d", "-i" ]));
		var task = new Queue(context);
		queue.push(task)
		task.push(new MakeTask(context, "make", [ "-s", "-n", "-f", context.home + "/tmp/mac/debug/" + context.title + "/makefile" ]));
		queue.push(new CommandTask(context, "open " + context.home + "/bin/mac/debug/" + context.title + ".app"));
		if (empty)
			queue.run();
	}
}

class Tool extends Queue {
	constructor(context) {
		super(context);
	}
	abort(device, debug) {
		let message = device.newStudioMessage("./disconnect");
		message.invoke();
	}
	build(project) {
		let home = shell.behavior.home;
		if (home) 
			this.buildCallback(project); 
		else {
			var dictionary = { message:"Locate kinomajs", prompt:"Open", url:Files.documentsDirectory };
			system.openDirectory(dictionary, url => { 
				if (url) {
					shell.behavior.home = url;
					this.buildCallback(project); 
				}
			});
			return;
		}
	}
	buildCallback(project) {
		let context = new BuildContext(project);
		let task = new BuildTask(context);
		task.execute(this);
	}
	evaluate(project, url, debug) {
		let XS = project.XS;
		let command = XS.command.replace("$", "\"" + Files.toPath(url)+ "\"");
		let directory = Files.toPath(mergeURI(project.url, XS.directory));
		let paths = XS.paths.map(url => Files.toPath(mergeURI(project.url, url)));
		let environment = {
			PATH: paths.join(":"),
			XSBUG_HOST: debug.host + ":" + debug.port,
		};
		console.log("> " + command),
		shell.execute(command, {
			callback: status => console.log("< " + status),
			directory,
			environment,
			stderr: text => shell.behavior.doLog(shell, text),
			stdout: text => shell.behavior.doLog(shell, text),
		});
	}
	execute(name, device, project, debug) {
		if (device.checkProject(project)) {
			let context = new RunContext(device, project, debug);
			let task = new RunTask(context);
			task.execute(this);
		}
	}
	exit(result) {
		this.tasks = [];
		//console.log("< " + result);
	}
	delete(device, project, debug, callback) {
		let context = new RunContext(device, project, debug);
		let task = new DeleteTask(context, callback);
		task.execute(this);
	}
	install(device, project, debug, callback) {
		let context = new RunContext(device, project, debug);
		let task = new InstallTask(context, callback);
		task.execute(this);
	}
	
	
}

var tool = new Tool(null);
export default tool;

