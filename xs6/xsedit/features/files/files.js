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
// MODEL

import { 
	model,
} from "shell/main";

import {
	Feature
} from "shell/feature";

import {
	Viewer
} from "shell/viewer";

import tool from "shell/tool";

const strings = {
	closeInfo: "Your changes will be lost if you don't save them.",
	closePrompt: "Do you want to save the changes to \"$\"?",
	projectCloseInfo: "The reference to the project will be removed.",
	projectClosePrompt: "Do you want to close the \"$\" project?",
	quitPrompt: "Do you want to save the changes to the following documents?\r\r",
};

export default class extends Feature {
	constructor(model) {
		super(model, "Projects");
		this.Template = FilePane;
		this.IconBehavior = FileIconBehavior;
		this.iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60, states:60 });
		this.viewers = [
			new CodeViewer(this),
			new ImageViewer(this),
			new MarkdownViewer(this),
		];
		this.mappings = [];
		this.documents = {
			expanded:true,
			items:[],
		};
		this.projects = {
			expanded:true,
			items:[],
		};
		this.currentProject = null;
		this.search = {
			expanded:false,
			findHint:"SEARCH",
			findMode:1,
			findString:"",
			items:[],
			message:null,
		}
	}
	addDocument(document) {
		this.documents.items.push(document);
		this.documents.items.sort((a, b) => a.name.compare(b.name));
		shell.distribute("onDocumentsChanged");
	}
	deleteProject(url) {
	}
	findProjectByID(id) {
		return this.projects.items.find(project => project.id == id);
	}
	findProjectByURI(url) {
		return this.projects.items.find(project => project.url == url);
	}
	read(json) {
		if ("documents" in json) {
			this.documents.expanded = json.documents.expanded;
			json.documents.items.forEach(item => { 
				if (Files.exists(item.url)) {
					Object.setPrototypeOf(item, Document.prototype);
					item.initialize();
					this.documents.items.push(item);
				}
			})
		}
		if ("projects" in json) {
			json.projects.items.forEach(item => { 
				if (Files.exists(item.url)) {
					Object.setPrototypeOf(item, Project.prototype);
					item.initialize();
					this.projects.items.push(item);
				}
			})
		}
		if ("mappings" in json) {
			this.mappings = json.mappings;
		}
	}
	removeAllDocuments() {
		this.documents.items.forEach(document => document.dispose());
		this.documents.items = [];
		shell.distribute("onDocumentsChanged");
	}
	removeDocument(document) {
		this.removeDocumentAt(this.documents.items.indexOf(document));
	}
	removeDocumentAt(index) {
		this.documents.items.splice(index, 1);
		shell.distribute("onDocumentsChanged");
	}
	selectProject(project) {
		this.currentProject = project;
	}
	write(json) {
		json.documents = this.documents;
		json.projects = this.projects;
		json.mappings = this.mappings;
	}
	
	canNewFile() {
		return true;
	}
	canNewDirectory() {
		return true;
	}
	canOpenFile() {
		return true;
	}
	canOpenDirectory() {
		return true;
	}
	canClose(url) {
		return url ? true : false;
	}
	canCloseAll() {
		return this.documents.items.length > 0;
	}
	canSaveAll() {
		return this.documents.items.length > 0;
	}
	canQuit() {
		return true;
	}
	
	doNewDirectory(url = Files.documentsDirectory) {
		
		system.saveDirectory({ name:"untitled", prompt:"New Project", url }, url => { if (url) this.doNewDirectoryCallback(url); });
	}
	doNewDirectoryCallback(url) {
		let parts = parseURI(url);
		Files.ensureDirectory(url + "/");
		let string = `{ "title":"${ parts.name }"}`;
		this.doNewFileCallback(url + "/project.json", string);
		this.doOpenDirectoryCallback(url + "/");
	}
	doNewFile(url = Files.documentsDirectory) {
		system.saveFile({ name:"untitled.js", prompt:"New File", url }, url => { if (url) this.doNewFileCallback(url); });
	}
	doNewFileCallback(url, string) {
		Files.writeText(url, string ? string : "");
		this.addDocument(new Document(url));
		this.doOpenFileCallback(url);
	}
	doOpenDirectory() {
		system.openDirectory({ prompt:"Open Project", url:Files.documentsDirectory }, url => { if (url) this.doOpenDirectoryCallback(url); });
	}
	doOpenDirectoryCallback(url) {
		let projects = this.projects.items;
		let project = projects.find(item => item.url == url);
		if (!project) {
			project = new Project(url);
			projects.push(project);
			projects.sort((a, b) => a.title.compare(b.title));
			shell.distribute("onProjectsChanged");
		}
		return project;
	}
	doOpenFile() {
		system.openFile({ prompt:"Open File", url:Files.documentsDirectory }, url => { if (url) this.doOpenFileCallback(url); });
	}
	doOpenFileCallback(url) {
		shell.delegate("doOpenURL", url);
	}
	doClose(url) {
		let items = this.documents.items;
		let index = items.findIndex(item => item.url == url);
		let document = (index >= 0) ? this.documents.items[index] : null;
		if (document) {
			if (document.dirty) {
				system.alert({ 
					type:"stop", 
					prompt:strings.closePrompt.replace("$", document.name), 
					info:strings.closeInfo, 
					buttons:["Save", "Cancel", "Don't Save"]
				}, ok => {
					if (ok === undefined)
						return;
					if (ok)
						document.save();
					else
						document.clean();
					document.close();
					shell.delegate("doCloseURL", url);
				});
			}
			else {
				document.close();
				shell.delegate("doCloseURL", url);
			}
		}
		else
			shell.delegate("doCloseURL", url);
	}
	doCloseAll() {
		let count = 0;
		let document = null;
		let prompt = strings.quitPrompt;
		this.documents.items.forEach(item => { 
			if (item.dirty) {
				count++; 
				document = item;
				prompt += "- " + item.name + "\r";
			}
		});
		if (count)
			system.alert({ 
				type:"stop", 
				prompt:(count == 1) ? strings.closePrompt.replace("$", document.name) : prompt, 
				info:strings.closeInfo, 
				buttons:["Save", "Cancel", "Don't Save"]
			}, ok => {
				if (ok === undefined)
					return;
				this.documents.items.forEach(item => {
					if (item.dirty) {
						if (ok)
							item.save();
						else
							item.clean();
					}
				});
				this.removeAllDocuments();
				shell.delegate("doCloseURL");
			});
		else {
			this.removeAllDocuments();	
			shell.delegate("doCloseURL");
		}
	}
	doSaveAll() {
		this.documents.items.forEach(item => { 
			if (item.dirty) {
				item.save();
			}
		});
	}
	doQuit() {
		let count = 0;
		let document = null;
		let prompt = strings.quitPrompt;
		this.documents.items.forEach(item => { 
			if (item.dirty) {
				count++; 
				document = item;
				prompt += "- " + item.name + "\r";
			}
		});
		if (count)
			system.alert({ 
				type:"stop", 
				prompt:(count == 1) ? strings.closePrompt.replace("$", document.name) : prompt, 
				info:strings.closeInfo, 
				buttons:["Save", "Cancel", "Don't Save"]
			}, ok => {
				if (ok === undefined)
					return;
				if (ok)
					this.documents.items.forEach(item => { 
						if (item.dirty)
							item.save(); 
					});
				shell.quit();  
			});
		else
			shell.quit();  
	}
	doSearchFile() {
		let search = this.search;
		if (search.message) {
			search.message.cancel();
			search.message = null;
		}
		search.items = [];
		if (search.findString) {
			search.message = new Message("xkpr://search/" + encodeURIComponent(search.findString) + "/" + search.findMode);
			trace("### SEARCHING " + search.message.url + "\n");
			search.message.requestObject = {
				pattern: findModeToPattern(search.findMode, search.findString),
				caseless: findModeToCaseless(search.findMode),
				items: this.projects.items.map(item => ({ name:item.name, path:Files.toPath(item.url) }))
			};
			search.message.invoke().then(message => {
				trace("### RESULTS " + message.url + "\n");
				if (search.message.url == message.url) {
					let json = message.responseObject;
					search.items = json;
					search.message = null;
					shell.distribute("onSearchChanged");
				}
			}, message => {
				trace("### CANCELLED\n");
				search.items = json;
				search.message = null;
				shell.distribute("onSearchChanged");
			});
			search.expanded = true;
		}
		else {
			search.expanded = false;
		}
		shell.distribute("onSearchChanged");
	}

}

export class Document {
	constructor(url) {
		let parts = parseURI(url);
		this.url = url;
		this.name = parts.name;
		this.initialize();
	}
	clean() {
		this.dirty = false;
		shell.distribute("onDocumentChanged", this);
	}
	close() {
		this.dispose();
		model.filesFeature.removeDocument(this);
	}
	dispose() {
		if (this.folderNotifier)
			this.folderNotifier.close();
		if (this.notifier)
			this.notifier.close();
	}
	initialize() {
		this.dirty = false;
		this.state = null;
		this.CODE = null;
		let url = this.url.slice(0, 0 - this.name.length);
		this.folderNotifier = new Files.DirectoryNotifier(url, url => {
			this.onFolderChanged(url);
		});
		this.fileNotifier = new Files.DirectoryNotifier(this.url, () => {
			this.onFileChanged();
		});
	}
	onCodeBegan(code) {
		this.CODE = code;
		this.state = null;
	}
	onCodeEnded(code) {
		this.CODE = null;
	}
	onFileChanged() {
		this.dirty = false;
		this.state = null;
		if (this.CODE)
			this.CODE.delegate("onFileChanged");
	}
	onFolderChanged() {
		if (!Files.exists(this.url))
			this.close();
	}
	save() {
		let string = (this.CODE) ? this.CODE.string : (this.state) ? this.state.string : "";
		this.fileNotifier.close();
		Files.writeText(this.url, string);
		this.fileNotifier = new Files.DirectoryNotifier(this.url, () => {
			this.onFileChanged();
		});
		this.clean();
	}
	toJSON() {
		return {
			url: this.url,
			name: this.name,
		};
	}
};

var studioApplication = {
	id:"",
	program:"",
	title:"",
	version:"",
}

export var g = new Grammar;
g.namespace("http://www.kinoma.com/kpr/application/1", "kpr");
g.object(studioApplication, "/kpr:application", {
	id: g.string("@id"),
	program: g.string("@program"),
	title: g.string("@title"),
	version: g.string("@version"),
});
g.link();

var projectFileNames = [
	"project.json",
	"manifest.json",
	"application.xml",
];

export class Project {
	constructor(url) {
		this.url = url;
		this.id = null;
		this.depth = 0;
		this.expanded = true;
		this.items = [];
		if (url) {
			let parts = parseURI(url.slice(0, -1));
			this.name = parts.name;
			this.title = decodeURIComponent(this.name);
			this.initialize();
		}
		else {
			this.title = null;
			this.name = null;
		}
	}
	canRun(device, url) {
		if (device && device.authorized && this[device.constructor.tag])
			return true;
		if (this.XS && url && url.startsWith(this.url))
			return true;
		if (this.standalone && (device.constructor.tag in this.standalone.platforms))
			return true;
		return false;
	}
	close() {
		this.dispose();
		let feature = model.filesFeature;
		let items = feature.projects.items;
		items.splice(items.indexOf(this), 1);
		shell.distribute("onProjectsChanged");
		if (feature.currentProject == this)
			feature.selectProject(null);
	}
	dispose() {
		if (this.fileNotifier)
			this.fileNotifier.close();
		if (this.folderNotifier)
			this.folderNotifier.close();
		if (this.notifier)
			this.notifier.close();
	}
	doRun(tool, device, url, debug) {
		if (device && this[device.constructor.tag])
			return tool.execute("run", device, this, debug);
		if (this.XS && url && url.startsWith(this.url))
			return tool.evaluate(this, url, debug);
		if (this.standalone)
			return tool.build(this, device.config);
	}
	downloaded(url, items) {
		this.expanded = true;
		this.items = items;
		this.url = url;
		this.initialize();
	}
	initialize() {
		let url = this.url.slice(0, 0 - this.name.length - 1);
		this.fileNotifier = null;
		this.folderNotifier = new Files.DirectoryNotifier(url, url => {
			this.onFolderChanged(url);
		});
		this.notifier = new Files.DirectoryNotifier(this.url, () => {
			if (this.fileNotifier) {
				this.fileNotifier.close();
				this.fileNotifier = null;
			}
			this.onChanged(url);
			this.onProjectChanged();
		});
		this.onChanged();
	}
	onChanged() {
		var c = projectFileNames.length;
		for (var i = 0; i < c; i++) {
			var url = mergeURI(this.url, projectFileNames[i]);
			if (Files.exists(url)) {
				this.fileNotifier = new Files.DirectoryNotifier(url, () => {
					this.onFileChanged(url, i);
					this.onProjectChanged();
				});
				this.onFileChanged(url, i);
				break;
			}
		}
		if (i == c) {
			this.title = decodeURIComponent(this.name);
			this.color = null;
			for (let Config of model.devicesFeature.Configs)
				this[Config.tag] = null;
			this.XS = null;
		}
		let icon = mergeURI(this.url, "icon.png");
		this.icon = (Files.exists(icon)) ? icon : null;
	}
	onFileChanged(url, flag) {
		try {
			if (flag == 0) {
				let json = Files.readJSON(url);
				this.id = json.id;
				this.title = json.title;
				this.color = ("color" in json) ? json.color : null;
				for (let Config of model.devicesFeature.Configs) {
					let name = Config.tag;
					if (name in json)
						this[name] = json[name];
					else
						this[name] = null;
				}
				if ("XS" in json)
					this.XS = json.XS;
				this.standalone = false;
			}
			else if (flag == 1) {
				let json = Files.readJSON(url);
				for (let Config of model.devicesFeature.Configs)
					this[Config.tag] = null;
				this.XS = null;
				this.color = null;
				if ("environment" in json) {
					let environment = json.environment;
					if ("NAMESPACE" in environment)
						this.id = environment.NAMESPACE.split(".").reverse().join(".");
					if ("NAME" in environment)
						this.title = environment.NAME;
					this.standalone = { platforms: json.platforms };
				}
				else {
					this.id = null;
					this.title = decodeURIComponent(this.name);
					this.standalone = false;
				}
			}
			else {
				let buffer = Files.readText(url);
				let config = g.parse(buffer);
				this.id = config.id;
				this.title = config.title;
				this.color = config.background;
				for (let Config of model.devicesFeature.Configs)
					this[Config.tag] = (Config.product == "Kinoma Element") ? null : { main:config.program };
				this.XS = null;
				this.standalone = false;
			}
		}
		catch(e) {
			for (let Config of model.devicesFeature.Configs)
				this[Config.tag] = null;
			this.XS = null;
			this.color = null;
			this.id = null;
			this.title = decodeURIComponent(this.name);
			this.standalone = false;
		}
	}
	onFolderChanged() {
		if (!Files.exists(this.url))
			this.close();
	}
	onProjectChanged() {
		shell.distribute("onProjectChanged", this);
		let items = model.filesFeature.projects.items;
		items.sort((a, b) => a.title.compare(b.title));
		shell.distribute("onProjectsChanged");
	}
	toJSON() {
		return {
			url: this.url,
			title: this.title,
			name: this.name,
			expanded: this.expanded,
			depth: this.depth,
			items: this.items,
		};
	}
};

class FileViewer extends Viewer {
	constructor(feature) {
		super(feature);
		this.scheme = "file";
	}
	accept(parts) {
		return (this.scheme == parts.scheme) && (this.extensions.findIndex(extension => parts.name.endsWith(extension)) >= 0);
	}
	create(parts, url) {
		if (Files.exists(url))
			return new this.Template(this.feature.model);
		for (let mapping of model.filesFeature.mappings) {
			if (url.startsWith(mapping.remote)) {
				url = mapping.locale.concat(url.slice(mapping.remote.length));
				if (Files.exists(url))
					model.url = url;
					return new this.Template(this.feature.model);
			}
		}
		return new ErrorView({ url, error:"File not found!" });
	}
};

class CodeViewer extends FileViewer {
	constructor(feature) {
		super(feature);
		this.Template = CodeView;
		this.extensions = [
			".c",
			".h",
			".js",
			".json",
			".markdown",
			".txt",
			".xml",
			".xs",
		];
	}
};

class ImageViewer extends FileViewer {
	constructor(feature) {
		super(feature);
		this.Template = ImageView;
		this.extensions = [
			".jpeg",
			".jpg",
			".png",
		];
	}
};

class MarkdownViewer extends FileViewer {
	constructor(feature) {
		super(feature);
		this.Template = MarkdownView;
		this.extensions = [
			".md",
		];
	}
};

// ASSETS

import {
	FIXED_FONT,
	GRAY,
	NORMAL_FONT,
	PASTEL_YELLOW,
	featureEmptyStyle,
	fileGlyphsSkin,
	grayHeaderSkin,
	grayFooterSkin,
	grayLineSkin,
	greenHeaderSkin,
	greenFooterSkin,
	greenLineSkin,
	orangeHeaderSkin,
	orangeFooterSkin,
	orangeLineSkin,
	redBorderSkin,
	redHeaderSkin,
	tableHeaderStyle,
	tableLineStyle,
	whiteButtonSkin,
	whiteButtonStyle,
	whiteButtonsSkin,
} from "shell/assets";

var documentHeaderSkin = orangeHeaderSkin;
var documentFooterSkin = orangeFooterSkin;
var documentLineSkin = orangeLineSkin;

var projectHeaderSkin = greenHeaderSkin;
var projectFooterSkin = greenFooterSkin;
var projectLineSkin = greenLineSkin;

const searchEmptyStyle = new Style({ font:NORMAL_FONT, size:14, color:GRAY });
const searchSpinnerStyle = new Style({ font:NORMAL_FONT, size:14, color:GRAY, horizontal:"left" });
const resultLineSkin = new Skin({ fill:["transparent", "transparent", PASTEL_YELLOW, PASTEL_YELLOW] });
const resultLineStyle = new Style({ font: FIXED_FONT, size:12, color:"#505050", horizontal:"left" });
const resultCountStyle = new Style({ font: NORMAL_FONT, size:12, color:GRAY, horizontal:"right", right:5 });

// BEHAVIORS

import { 
	ButtonBehavior, 
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

import {
	ScrollerBehavior,
	VerticalScrollbar,
} from "common/scrollbar";

import { 
	HolderColumnBehavior,
	HolderContainerBehavior,
	LineBehavior,
	HeaderBehavior,
	TableBehavior,
} from "shell/behaviors";

import {
	FeatureIconBehavior,
	FeaturePaneBehavior,
} from "shell/feature";

class FileIconBehavior extends FeatureIconBehavior {
	onDocumentChanged(icon, document) {
		this.onDocumentsChanged(icon);
	}
	onDocumentsChanged(icon) {
		let feature = this.data;
		let count = 0;
		feature.documents.items.forEach(document => {
			if (document.dirty)
				count++;
		});
		this.setBadge(icon, count);
	}
}

class FilePaneBehavior extends FeaturePaneBehavior {
	onDisplaying(container) {
		container.distribute("onDeviceSelected", model.devicesFeature.currentDevice);
		container.distribute("onMachinesChanged", model.debugFeature.machines, model.debugFeature.debuggees);
		container.distribute("onMachineSelected", model.debugFeature.currentMachine);
		container.distribute("onSearchChanged");
		this.onProjectsChanged(container);
	}
	onProjectsChanged(container, project) {
		let scroller = container.first;
		let column = scroller.first;
		let data = this.data;
		let items = data.projects.items;
		let target = null;
		scroller.empty(2);
		column.empty(1);
		if (items.length) {
			column.add(new SearchTable(data.search));
			items.forEach(item => { 
				let table = item.url ? new ProjectTable(item) : new DownloadTable(item);
				column.add(table);
				if (item == project)
					target = table
			});
			if (target) {
				let bounds = target.bounds;
				bounds.x -= column.x;
				bounds.y -= column.y;
				scroller.reveal(bounds);
			}
		}
		else
			scroller.add(new NoProjectsContainer());
	}
};

class FileButtonBehavior extends Behavior {
	changeState(button, state) {
		button.state = state;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onMouseEntered(container, x, y) {
		this.changeState(container, 1);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, 0);
	}
	onTap(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.changeState(container, 2);
		container.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		if (container.hit(x, y)) {
			this.changeState(container, 1);
			this.onTap(container);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 2 : 1);
	}
};

export class FileLineBehavior extends LineBehavior {
	onCreate(line, data) {
		super.onCreate(line, data);
		this.onURLChanged(line, model.url);
	}
	onDocumentChanged(line, document) {
		if (this.data.url == document.url)
			line.first.next.visible = document.dirty;
	}
	onTap(line) {
		shell.delegate("doOpenURL", this.data.url);
	}
	onURLChanged(line, url) {
		if (this.data.url == url)
			this.flags |= 4;
		else
			this.flags &= ~4;
		this.changeState(line);
	}
};

class DocumentTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			for (let item of data.items)
				column.add(new DocumentLine(item));
		}
		else {
			header.behavior.expand(header, false);
		}
		column.add(new DocumentFooter(data));
	}
	hold(column) {
		return DocumentHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		if (data.expanded) {
			for (let item of data.items)
				column.add(new DocumentLine(item));
		}
		column.add(new DocumentFooter(data));
	}
	onDocumentsChanged(column, data) {
		var data = this.data;
		if (data.expanded) {
			column.empty(1);
			for (let item of data.items) {
				column.add(new DocumentLine(item));
			}
			column.add(new DocumentFooter(data));
		}
	}
};

class DocumentHeaderBehavior extends HeaderBehavior {
	reveal(line, revealIt) {
		let button = line.last;
		button.visible = revealIt;
		button.previous.visible = revealIt;
	}
};

class DocumentLineBehavior extends FileLineBehavior {
	onDocumentChanged(line, document) {
		if (this.data == document)
			line.first.next.visible = document.dirty;
	}
	onMouseEntered(line, x, y) {
		super.onMouseEntered(line, x, y);
		line.CLOSE.visible = true;
	}
	onMouseExited(line, x, y) {
		line.CLOSE.visible = false;
		super.onMouseExited(line, x, y);
	}
};

class DocumentCloseButtonBehavior extends FileButtonBehavior {
	onTap(button) {
		button.bubble("doClose", null, this.data.url);
	}
};

class DownloadHeaderBehavior extends HeaderBehavior {
	onProjectDownloading(line, data, offset, size) {
		if (this.data == data) {
			var bar = line.first;
			bar.width = Math.round(line.width * offset / size);
		}
	}
};

export class FolderTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		if (expandIt) {
			header.behavior.expand(header, true);
			this.notifier = new Files.DirectoryNotifier(data.url, url => {
				this.onDirectoryChanged(column, url);
			});
			this.onDirectoryChanged(column);
		}
		else {
			header.behavior.expand(header, false);
			this.notifier.close();
			this.notifier = null;
			column.empty(this.emptyIndex);
		}
	}
	onCreate(column, data) {
		this.data = data;
		this.emptyIndex = 1;
		if (data.expanded) {
			this.notifier = new Files.DirectoryNotifier(data.url, url => {
				this.onDirectoryChanged(column, url);
			});
			this.onDirectoryChanged(column);
		}
	}
	onDirectoryChanged(column) {
		let data = this.data;
		let depth = data.depth + 1;
		let iterator = new Files.Iterator(data.url);
		let formers = data.items;
		let i = 0;
		let c = formers.length;
		let former = (i < c) ? formers[i] : null;
		let info = iterator.getNext();
		let items = [];
		for (; former || info;) {
			let order = former ? info ? former.name.compare(info.path) : -1 : 1;
			if (order < 0) {
				i++;
				former = (i < c) ? formers[i] : null;
			}
			else if (order > 0) {
				let item;
				if (info.type == Files.fileType)
					item = { depth, kind:"file", name:info.path, url:mergeURI(data.url, info.path) };
				else
					item = { depth, kind:"folder", name:info.path, url:mergeURI(data.url, info.path + "/"), expanded:false, items:[] };
				items.push(item);
				info = iterator.getNext();
			}
			else {
				items.push(former);
				i++;
				former = (i < c) ? formers[i] : null;
				info = iterator.getNext();
			}
		}
		data.items = items;
		column.empty(this.emptyIndex);
		if (data.expanded) {
			for (let item of items)
				column.add(new FileKindTemplates[item.kind](item));
		}
	}
	onUndisplayed(column) {
		if (this.notifier) {
			this.notifier.close();
			this.notifier = null;
		}
	}
};

class FolderLineBehavior extends HeaderBehavior {
};

class ProjectTableBehavior extends FolderTableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		if (expandIt) {
			header.behavior.expand(header, true);
			this.onDirectoryChanged(column);
		}
		else {
			header.behavior.expand(header, false);
			column.empty(1);
		}
		column.add(new ProjectFooter(data));
	}
	hold(column) {
		return ProjectHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		this.emptyIndex = 1;
		if (data.expanded) {
			this.onDirectoryChanged(column);
		}
		column.add(new ProjectFooter(data));
	}
	onDirectoryChanged(column) {
		super.onDirectoryChanged(column);
	}
	onProjectChanged(column, data) {
		if (this.data == data) {
			if (data.expanded) {
				this.onDirectoryChanged(column);
				column.add(new ProjectFooter(data));
			}
			let line = column.first;
			line.TITLE.string = data.title;
		}
	}
};

class ProjectHeaderBehavior extends FolderLineBehavior {
	onProjectChanged(line, project) {
		if (this.data == project)
			line.TITLE.string = project.title;
	}
	reveal(line, revealIt) {
		line.CLOSE.visible = revealIt;
		line.ADD.visible = revealIt;
	}
};

class ProjectCloseButtonBehavior extends ButtonBehavior {
	onTap(button) {
		system.alert({
			type:"stop",
			prompt:strings.projectClosePrompt.replace("$", this.data.title),
			info:strings.projectCloseInfo,
			buttons:["Close", "Cancel"]
		}, ok => {
			if (ok === undefined)
				return;
			if (ok)
				this.data.close();
		});
	}
};

class ProjectAddButtonBehavior extends ButtonBehavior {
	onTap(button) {
		shell.behavior.filesFeature.doNewFile(this.data.url);
	}
};

class ProjectRunButtonBehavior extends ButtonBehavior {
	onDisplaying(button) {
		this.sync(button, shell.behavior.devicesFeature.currentDevice, shell.behavior.url);
	}
	onDeviceSelected(button, device) {
		this.sync(button, device, shell.behavior.url);
	}
	onProjectChanged(button, project) {
		if (this.data == project)
			this.sync(button, shell.behavior.devicesFeature.currentDevice, shell.behavior.url);
	}
	onTap(button) {
		shell.behavior.filesFeature.selectProject(this.data);
		button.bubble("doRun");
	}
	onURLChanged(button, url) {
		this.sync(button, shell.behavior.devicesFeature.currentDevice, url);
	}
	sync(button, device, url) {
		button.active = button.visible = this.data.canRun(device, url);
	}
};

class SearchTableBehavior extends TableBehavior {
	expand(table, expandIt) {
		var data = this.data;
		var header = table.first;
		data.expanded = expandIt;
		table.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			if (data.items) {
				if (data.items.length) {
					for (let item of data.items)
						table.add(new ResultTable(item));
				}
				else
					table.add(new SearchEmpty(data));
			}
			else
				table.add(new SearchSpinner(data));
		}
		else {
			header.behavior.expand(header, false);
		}
		table.add(new SearchFooter(data));
	}
	hold(table) {
		return SearchHeader(this.data, {left:0, right:0, top:0, height:table.first.height});
	}
	onComplete(table, message, json) {
		table.stop();
		let data = this.data;
		data.items = json;
		this.expand(table, data.expanded);
	}
	onCreate(table, data) {
		table.duration = 500;
		this.data = data;
		this.onFindEdited(table);
	}
	onFindEdited(table) {
		table.cancel();
		table.stop();
		let data = this.data;
		if (data.findString) {
			let message = new Message("xkpr://search/all");
			message.requestObject = {
				pattern: findModeToPattern(data.findMode, data.findString),
				caseless: findModeToCaseless(data.findMode),
				items: model.filesFeature.projects.items.map(item => ({ name:item.name, url:item.url }))
			};
			table.invoke(message, Message.JSON);
			if (data.items && data.items.length) {
				table.time = 0;
				table.start();
			}
			else
				data.items = null;
			this.expand(table, true);
		}
		else {
			data.items = [];
			this.expand(table, false);
		}
	}
	onFinished(table) {
		let data = this.data;
		data.items = null;
		this.expand(table, this.data.expanded);
	}
	onFlow(table, holder) {
		let data = this.data;
		let label = data.FIND_FOCUS;
		let focused = label.focused;
		let selectionOffset = label.selectionOffset;
		let selectionLength = label.selectionLength;
		super.onFlow(table, holder);
		table.first.distribute("onRestore");
		label = data.FIND_FOCUS;
		if (focused)
			label.focus();
		label.select(selectionOffset, selectionLength);
	}
	onHold(table, holder) {
		let data = this.data;
		let label = data.FIND_FOCUS;
		let focused = label.focused;
		let selectionOffset = label.selectionOffset;
		let selectionLength = label.selectionLength;
		super.onHold(table, holder);
		label = data.FIND_FOCUS;
		if (focused)
			label.focus();
		label.select(selectionOffset, selectionLength);
	}
};

class SearchHeaderBehavior extends HeaderBehavior {
	onFindEdited(line) {
		let table = this.held ? this.table : line.container;
		table.behavior.onFindEdited(table);
		return true;
	}
};

class ResultTableBehavior extends TableBehavior {
	expand(table, expandIt) {
		let data = this.data;
		let header = table.first;
		data.expanded = expandIt;
		table.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			if (data.items) {
				for (let item of data.items)
					table.add(new ResultLine(item));
			}
			else {
				let message = new Message("xkpr://search/results");
				message.requestObject = { url: data.url };
				table.invoke(message, Message.JSON)
			}
		}
		else {
			header.behavior.expand(header, false);
			table.cancel();
		}
	}
	onComplete(table, message, json) {
		let data = this.data;
		data.items = json;
		this.expand(table, data.expanded);
	}
	onCreate(table, data) {
		this.data = data;
		this.expand(table, data.expanded);
	}
};

class ResultHeaderBehavior extends HeaderBehavior {
};

class ResultLineBehavior extends LineBehavior {
	onCreate(line, data) {
		this.data = data;
		line.last.select(data.delta, data.length);
	}
	onTap(line) {
		let data = this.data;
		shell.delegate("doOpenURL", line.container.behavior.data.url, data.offset + "-" + (data.offset + data.length));
	}
};

// TEMPLATES

import {
	DeviceHeader,
} from "features/devices/devices"

import {
	FindField,
	findModeToCaseless,
	findModeToPattern,
} from "find";

import {
	WaitContent,
} from "features/devices/behaviors"

var FilePane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0,
	Behavior: FilePaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:100, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:10, right:10, top:0, Behavior:HolderColumnBehavior, 
					contents: [
						DocumentTable($.documents, {}),
					]
				}),
				VerticalScrollbar($, {}),
			]
		}),
		Container($, {
			left:10, right:10, top:100, height:40, clip:true, Behavior:HolderContainerBehavior,
		}),
		DeviceHeader(model.devicesFeature, { }),
	]
}});


var DocumentTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: DocumentTableBehavior,
	contents: [
		DocumentHeader($, {}),
	],
}});

var DocumentHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:documentHeaderSkin, active:true,
	Behavior: DocumentHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:1 }),
		Label($, { left:0, right:0, style:tableHeaderStyle, string:"DOCUMENTS" }),
		Container($, {
			width:80, skin:whiteButtonSkin, active:true, visible:false, name:"doCloseAll",
			Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Close All" }),
			],
		}),
		Container($, {
			width:80, skin:whiteButtonSkin, active:true, visible:false, name:"doSaveAll",
			Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Save All" }),
			],
		}),
	],
}});

var DocumentFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:documentFooterSkin,
}});

var DocumentLine = Line.template(function($) { return {
	left:0, right:0, skin:documentLineSkin, active:true,
	Behavior: DocumentLineBehavior,
	contents: [
		Content($, { width:0, height:20, }),
		Content($, { width:20, height:20, skin:fileGlyphsSkin, variant:4, visible:$.dirty }),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
		Content($, { name:"CLOSE", width:20, height:20, skin:fileGlyphsSkin, variant:2, state:0, active:true, visible:false, Behavior:DocumentCloseButtonBehavior }),
	],
}});

var DownloadTable = Column.template(function($) { return {
	left:0, right:0,
	contents: [
		Container($, {
			left:0, right:0, height:30, skin:grayHeaderSkin,
			Behavior: DownloadHeaderBehavior,
			contents: [
				Content($, { left:0, width:0, height:30, skin:greenHeaderSkin }),
				Line($, {
					left:0, right:0, height:30,
					contents: [
						Content($, { width:0 }),
						Content($, { width:30, height:30, skin:fileGlyphsSkin, state:1, variant:1 }),
						Label($, { left:0, right:0, style:tableHeaderStyle, string:$.title }),
					],
				}),
			]
		}),
		Content($, { height:5 }),
	],
}});


var ProjectTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: ProjectTableBehavior,
	contents: [
		ProjectHeader($, {}),
	],
}});

var ProjectHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:projectHeaderSkin, active:true,
	Behavior: ProjectHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:1 }),
		Label($, { name:"TITLE", left:0, right:0, style:tableHeaderStyle, string:$.title }),
		Content($, { name:"CLOSE", width:30, height:30, skin:whiteButtonsSkin, variant:0, state:1, active:true, visible:false, Behavior:ProjectCloseButtonBehavior }),
		Content($, { name:"ADD", width:30, height:30, skin:whiteButtonsSkin, variant:1, state:1, active:true, visible:false, Behavior:ProjectAddButtonBehavior }),
		Content($, { name:"RUN", width:30, height:30, skin:whiteButtonsSkin, variant:2, state:1, active:true, visible:true, Behavior:ProjectRunButtonBehavior }),
	],
}});

var ProjectFooter = Line.template(function($) { return {
	left:0, right:0, height:5, skin:projectFooterSkin,
}});

var FileLine = Line.template(function($) { return {
	left:0, right:0, skin:projectLineSkin, active:true,
	Behavior: FileLineBehavior,
	contents: [
		Content($, { width:($.depth - 1) * 20 }),
		Content($, { width:20, height:20, skin:fileGlyphsSkin, variant:4, visible:false }),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
	],
}});

var FolderTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: FolderTableBehavior,
	contents: [
		FolderLine($, {}),
	],
}});

var FolderLine = Line.template(function($) { return {
	left:0, right:0, skin:projectLineSkin, active:true,
	Behavior: FolderLineBehavior,
	contents: [
		Content($, { width:($.depth - 1) * 20 }),
		Content($, { width:20, height:20, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:0}),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
	],
}});

var FileKindTemplates = {
	file: FileLine,
	folder: FolderTable,
	project: ProjectTable,
};


var SearchTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: SearchTableBehavior,
	contents: [
		SearchHeader($, {}),
	],
}});

var SearchHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:grayHeaderSkin, active:true,
	Behavior: SearchHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:1 }),
		FindField($, { }),
		Content($, { width:4 }),
	],
}});

var SearchFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:grayFooterSkin,
}});

var SearchSpinner = Container.template($ => ({
	left:0, right:0, height:30, skin:grayLineSkin, 
	contents: [
		WaitContent($, { }),
	],
}));

var SearchEmpty = Line.template($ => ({
	left:0, right:0, height:30, skin:grayLineSkin, 
	contents: [
		Label($, { left:0, right:0, height:30, style:searchEmptyStyle, string:"Not Found!" }),
	],
}));

var ResultTable = Column.template(function($) { return {
	left:0, right:0, active:true, clip:true,
	Behavior: ResultTableBehavior,
	contents: [
		ResultHeader($, {}),
	],
}});

var ResultHeader = Line.template(function($) { return {
	left:0, right:0, skin:grayLineSkin, active:true,
	Behavior: ResultHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:20, height:20, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:0}),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
		Label($, { style:resultCountStyle, string:$.count }),
	],
}});

var ResultLine = Line.template(function($) { return {
	left:0, right:0, skin:grayLineSkin, active:true,
	Behavior: ResultLineBehavior,
	contents: [
		Content($, { width:40, height:20 }),
		Label($, { left:0, skin:resultLineSkin, style:resultLineStyle, string:$.string, selectable:true }),
	],
}});


var NoProjectsContainer = Container.template($ => ({ 
	left:0, right:0, top:0, bottom:0,
	Behavior: class extends Behavior {
		onScrolled(container) {
			let scroller = container.container;
			let text = container.last;
			let size = scroller.height;
			let range = scroller.first.height;
			let height = text.height;
			if (height > size - range) {
				text.y = 0;
				text.visible = false;
			}
			else {
				text.y = scroller.y + ((size + range - height) >> 1);
				text.visible = true;
			}
		}
	},
	contents: [
		Content($, {}), // scrollbar
		Text($, { left:0, right:0, top:0, style:featureEmptyStyle, string:"No projects!" }),
	],
}));

import {
	CloseButton,
	errorStyle,
} from "shell/viewer";


import {
	redButtonSkin,
	redButtonStyle
} from "common/assets";

var ErrorView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:redBorderSkin,
	contents: [
		Scroller($, {
			left:1, right:1, top:30, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				Column($, {
					contents: [
						Label($, { style: errorStyle, string:$.error }),
						Container($, {
							anchor:"BUTTON", width:100, skin:redButtonSkin, active:true, name:"onEnter",
							Behavior: class extends ButtonBehavior {
								doOpenFileCallback(locateURL) {
									var s1 = this.data.url.split("/");
									var c1 = s1.length;
									var s2 = locateURL.split("/");
									var c2 = s2.length;
									var c = c1 > c2 ? c2 : c1;
									var m1, m2;
									for (let i = 1; i <= c; i++) {
										if (s1[c1 - i] != s2[c2 - i]) {
											if (i > 1) {
												let line = model.at;
												let remote = s1.slice(0, c1 - i + 1).join("/");
												let locale = s2.slice(0, c2 - i + 1).join("/");
												shell.delegate("doCloseURL", this.data.url);
												shell.delegate("doOpenURL", locateURL, line);
												var mappings = model.filesFeature.mappings;
												mappings.unshift({locale, remote});
												mappings.slice(10);
											}
											break;
										}
									}
								}
								onEnter(button) {
									var url = this.data.url;
									var parts = parseURI(url);
									var name = parts.name;
									var dictionary = { message:"Locate " + url, prompt:"Open", url:Files.documentsDirectory };
									var index = name.lastIndexOf(".");
									if (index >= 0)
										dictionary.fileType = name.slice(index + 1);
									system.openFile(dictionary, url => { if (url) this.doOpenFileCallback(url); });
								}
							},
							contents: [
								Label($, { left:0, right:0, style:redButtonStyle, string:"Locate..." }),
							],
						}),
					]
				}),
			],
		}),
		Line($, {
			left:0, right:0, top:0, height:30, skin:redHeaderSkin,
			contents: [
				Content($, { width:30, height:30 }),
				Label($, { left:0, right:0, height:30, style: tableHeaderStyle, string:$.url }),
				CloseButton($, {}),
			],
		}),
	],
}));


import {
	CodeView
} from "code";

import {
	ImageView
} from "image";

import {
	MarkdownView
} from "markdown";

