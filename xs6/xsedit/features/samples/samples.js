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

import { 
	TemplatesModel,
} from "templates";

export default class extends Feature {
	constructor(model) {
		super(model, "Samples");
		this.Template = SamplePane;
		this.iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60, states:60 });
		
		this.reload = false;
		this.samples = {
			sha:"",
			tagItems:[{ count:0, name:"", title:"All" }],
			tagSelection:0,
			items:[],
		};
		this.tree = null;
		
		let url = model.cacheDirectory;
		Files.ensureDirectory(url);
		url = mergeURI(model.cacheDirectory, "./tags.json");
		let formers = Files.exists(url) ? Files.readJSON(url) : [];
		url = mergeURI(shell.url, "../templates/tags.json");	
		let currents = Files.readJSON(url);
		currents.forEach(current => {
			let former = formers.find(former => former.name == current.name);
			if (!former) {
				this.reload = true;
				current.mask = 1 << formers.length;
				formers.push(current);
			}
		});
		this.tags = formers;
		
		url = mergeURI(shell.url, "../templates/");	
		let iterator = new Files.Iterator(url);
		let info = iterator.getNext();
		let templates = [];
		while (info) {
			if (info.type == Files.directoryType) {
				try {
					let directoryURL = mergeURI(url, info.path + "/");
					let fileURL = directoryURL + "/project.json";
					if (Files.exists(fileURL)) {
						let template = Files.readJSON(fileURL);
						template.url = directoryURL;
						let mask = 1;
						if ("Create" in template)
							mask |= 2;
						if ("Element" in template)
							mask |= 4;
						template.tags.forEach(name => {
							let tag = this.tags.find(tag => tag.name == name);
							mask |= tag.mask;
						})
						template.mask = mask;
						fileURL = directoryURL + "/thumbnail.jpg";
						template.thumbnail = Files.exists(fileURL) ? fileURL : null;
						templates.push(template);
					}
				}
				catch(e) {
				}
			}
			info = iterator.getNext();
		}
		this.templates = templates;
		
	}
	authorize(message) {
		message.setRequestHeader("User-Agent", "k4.kinoma.com/1.0");
	}
	close() {
		let url = mergeURI(model.cacheDirectory, "./tags.json");
		Files.writeText(url, JSON.stringify(this.tags, null, 4));
	}
	delete(sample) {
		let url = mergeURI(model.samplesDirectory, sample.name + "/");
		sample.close();
		sample.descriptionVisible = true;
		sample.title = sample.name;
		sample.url = null;
		shell.distribute("onSampleDeleted", sample);
		model.deleteDirectory(url);
	}
	download(sample) {
		let path = "/samples/" + sample.name;
		let handler = sample.handler = new Handler(path);
		handler.behavior = new SampleDownloadBehavior(this, sample);
		Handler.put(handler);
		shell.invoke(new Message(path));
	}
	idle(now) {
		// ping
	}
	install(data) {
		let path = "/templates/" + data.template.name;
		let handler = new Handler(path);
		if (data.template instanceof Sample)
			handler.behavior = new TemplateDownloadBehavior(this, data);
		else
			handler.behavior = new TemplateCopyBehavior(this, data);
		Handler.put(handler);
		shell.invoke(new Message(path));
	}
	notify() {
	}
	open() {
		let path = "/samples/README.md";
		let handler = this.handler = new Handler(path);
		handler.behavior = new SampleCatalogBehavior(this);
		Handler.put(handler);
		shell.invoke(new Message(path));
	}
	parseCatalog(catalog) {
		var document = DOM.parse("<catalog>" + catalog + "</catalog>" );
		var children = document.element.children;
		var elementProto = DOM.element;
		var samples = {};
		var items = samples.items = [];
		var state = 0;
		let tagItems, title, name, thumbnail, id, mask, description;
		children.forEach(child => {
			let flag = elementProto.isPrototypeOf(child);
			switch (state) {
			case 0:
				if (flag && (child.name == "x-tag-info")) {
					let tags = this.tags;
					let tagNames = child.getAttribute("tags").split(',');
					let tagTitles = child.getAttribute("titles").split(',');
					tagItems = tagNames.map((name, i) => {
						let tag = tags.find(tag => tag.name == name);
						if (tag)
							return { count:0, mask:tag.mask, name, title:tagTitles[i] };
						let mask = 1 << tags.length;
						tags.push({ mask, name, title:tagTitles[i] });
						return { count:0, mask, name, title:tagTitles[i] };
					});
					tagItems.sort((a, b) => a.title.compare(b.title));
					samples.tagItems = [{ count:0, mask:1, name:"", title:"All" }].concat(tagItems);
					samples.tagSelection = 0;
					state = 1;
				}
				break;
			case 1:
				if (flag && (child.name == "a")) {
					let parts = parseURI(child.getAttribute("href"));
					name = parts.name;
					title = child.children[0].value;
					state = 2;
				}
				break;
			case 2:
				if (flag && (child.name == "a")) {
					thumbnail = child.children[0].getAttribute("src");
					state = 3;
				}
				break;
			case 3:
				if (flag && (child.name == "x-app-info")) {
					id = child.getAttribute("id");
					let value = child.children[0].getAttribute("class");
					let items = this.tagItems;
					let c = tagItems.length;
					mask = 1;
					tagItems[0].count++;
					for (let i = 0; i < c; i++) {
						let tagItem = tagItems[i];
						if (value.indexOf(tagItem.name) >= 0) {
							mask |= tagItem.mask;
							tagItem.count++;
						} 
					}
					state = 4;
				}
				break;
			case 4:
				if (!flag) {
					description = child.value.trim();
					state = 5;
				}
				break;
			case 5:
				if (flag && (child.name == "div")) {
					items.push(new Sample(title, name, id, description, thumbnail, mask));
					state = 1;
				}
				break;
			}
		});
		return samples;
	}
	
	canNewProject() {
		let feature = model.devicesFeature;
		return feature.Configs.some(Config => feature.discoveryFlags[Config.id] || feature.simulatorFlags[Config.id]);
	}
	doNewProject() {
		let data = new TemplatesModel();
		data.run();
	}
};

import {
	Project
} from "features/files/files"

class Sample extends Project {
	constructor(title, name, id, description, thumbnail, mask) {
		super(null);
		this.title = title;
		this.name = name;
		this.id = id;
		this.description = description;
		this.descriptionExpanded = true;
		this.descriptionVisible = true;
		this.thumbnail = thumbnail;
		this.mask = mask;
		this.expanded = true;
		this.handler = null;
	}
	downloaded(url, items) {
		this.descriptionVisible = false;
		this.expanded = true;
		this.items = items;
		this.url = url;
		this.initialize();
	}
	toJSON() {
		return {
			url: this.url,
			title: this.title,
			name: this.name,
			expanded: this.expanded,
			depth: this.depth,
			items: this.items,
			id: this.id,
			description: this.description,
			descriptionExpanded: this.descriptionExpanded,
			descriptionVisible: this.descriptionVisible,
			mask: this.mask,
			thumbnail: this.thumbnail,
		};
	}
};

class SampleCatalogBehavior extends Behavior {
	constructor(feature) {
		super();
		this.feature = feature;
		this.index = -1;
		this.url = mergeURI(model.cacheDirectory, "./samples.json");
	}
	onComplete(handler, message, result) {
		if (this.index < 0) {
			if (!result || !result.tree) {
				Handler.remove(handler);
				delete this.feature.handler;
				shell.distribute("onSamplesChanged");
				return;
			}
			this.feature.tree = result.tree;
			this.index++;
			if (Files.exists(this.url)) {
				message = new Message(this.url);
				handler.invoke(message, Message.JSON);
				return;
			}
			result = undefined;
		}
		if (this.index == 0) {
			let title = "README.md";
			let node = this.feature.tree.find(node => node.path == title);
			this.index++;
			if (!result || (result.sha != node.sha) || this.feature.reload) {
				this.sha = node.sha;
				message = new Message(node.url);
				message.setRequestHeader("Accept", "application/vnd.github.v3.raw");
				this.feature.authorize(message);
				handler.invoke(message, Message.TEXT);
				return;
			}
			result.items.forEach(item => Object.setPrototypeOf(item, Sample.prototype));
			this.feature.samples = result;
			this.index++;
		}
		if (this.index == 1) {
			result = this.feature.parseCatalog(result);
			result.sha = this.sha;
			Files.deleteFile(this.url);
			Files.writeText(this.url, JSON.stringify(result, null, 4));
			this.feature.samples = result;
			this.index++;
		}
		if (this.index == 2) {
			this.index++;
			message = new Message(model.samplesDirectory);
			handler.invoke(message, Message.JSON);
			return;
		}
		if (this.index == 3) {
			let items = this.feature.samples.items;
			if (result) {
				for (let node of result) {
					let path = node.path;
					let item = items.find(item => item.name == path);
					if (item)
						item.downloaded(node.url, []);
				}
			}
			Handler.remove(handler);
			delete this.feature.handler;
			shell.distribute("onSamplesChanged");
		}
	}
	onInvoke(handler, message) {
		message = new Message("https://api.github.com/repos/Kinoma/KPR-examples/git/trees/master");
		this.feature.authorize(message);
		handler.invoke(message, Message.JSON);
	}
};

class DownloadBehavior extends Behavior {
	constructor(feature, sample, base) {
		super();
		this.feature = feature;
		this.sample = sample;
		this.index = -1;
	}
	onComplete(handler, message, json) {
		if (this.index < 0) {
			let blobs = [];
			let items = [];
			let size = 0;
			for (let node of json.tree) {
				let split = node.path.split("/");
				let depth = split.length;
				let name = split[0];
				if (name[0] != ".") {
					if (node.type != "tree") {
						let url = mergeURI(this.base, node.path);
						blobs.push({
							src: node.url,
							dst: url,
							size: node.size,
						});
						size += node.size;
						if (depth == 1) {
							items.push({ depth, kind:"file", name, url });
						}
						else if (depth == 2) {
							let item = items.find(item => item.name == name);
							name = split[1];
							item.items.push({ depth, kind:"file", name, url });
						}
					}
					else {
						let url = mergeURI(this.base, node.path + "/");
						Files.ensureDirectory(url);
						if (depth == 1) {
							items.push({ depth, kind:"folder", name, url, expanded:true, items:[] });
						}
						else if (depth == 2) {
							let item = items.find(item => item.name == name);
							name = split[1];
							item.items.push({ depth, kind:"folder", name, url, expanded:false, items:[] });
						}
					}
				}
			}
			this.blobs = blobs;
			this.items = items;
			this.offset = 0;
			this.size = size;
		}
		else
			this.offset += this.blobs[this.index].size;
		this.index++;
		if (this.index < this.blobs.length) {
			let blob = this.blobs[this.index];
			message = new Message(blob.src);
			message.setRequestHeader("Accept", "application/vnd.github.v3.raw");
			this.feature.authorize(message);
			handler.download(message, blob.dst);
		}
		else {
			Handler.remove(handler);
			this.onCompleteAll(handler, message, json);
		}
	}
	onInvoke(handler, message) {
		let path = this.sample.name;
		let node = this.feature.tree.find(node => node.path == path);
		var message = new Message(node.url + "?recursive=1");
		this.feature.authorize(message);
		handler.invoke(message, Message.JSON);
	}
	onProgress(handler, message, offset, size) {
		this.onProgressAll(handler, message, this.offset + offset, this.size);
	}
};

class SampleDownloadBehavior extends DownloadBehavior {
	constructor(feature, sample) {
		super(feature, sample);
		this.base = mergeURI(model.samplesDirectory, sample.name + "/");
		Files.ensureDirectory(this.base);
	}
	onCompleteAll(handler, message, json) {
		this.sample.downloaded(this.base, this.items);
		shell.distribute("onSampleDownloaded", this.sample);
	}
	onProgressAll(handler, message, offset, size) {
		shell.distribute("onSampleDownloading", this.sample, offset, size);
	}
};

import {
	g,
} from "features/files/files"

class TemplateDownloadBehavior extends DownloadBehavior {
	constructor(feature, data) {
		super(feature, data.template);
		this.data = data;
		this.base = mergeURI(model.projectsDirectory, data.id + "/");
		Files.ensureDirectory(this.base);
		let projects = model.filesFeature.projects.items;
		let project = this.project = new Project(null);
		project.title = data.title;
		project.name = data.id;
		projects.push(project);
		projects.sort((a, b) => a.title.compare(b.title));
		model.doSelectFeature(shell, model.filesFeature);
		shell.distribute("onProjectsChanged", project);
	}
	onCompleteAll(handler, message, json) {
		let data = this.data;
		let url = mergeURI(this.base, "project.json");
		if (Files.exists(url)) {
			let json = Files.readJSON(url);
			json.id = data.id + "." + data.domain;
			json.title = data.title;
			Files.writeText(url, JSON.stringify(json, null, 4));
		}
		else {
			let url = mergeURI(this.base, "application.xml");
			if (Files.exists(url)) {
				let buffer = Files.readText(url);
				let application = g.parse(buffer);
				buffer = `<?xml version="1.0" encoding="utf-8"?><application xmlns="http://www.kinoma.com/kpr/application/1" id="${ data.id + "." + data.domain }" program="${ application.program }" title="${ data.title }"></application>`
				Files.writeText(url, buffer);
			}
			else
				debugger
		}
		this.project.downloaded(this.base, this.items);
		shell.distribute("onProjectsChanged", this.project);
	}
	onProgressAll(handler, message, offset, size) {
		shell.distribute("onProjectDownloading", this.project, offset, size);
	}
};

class TemplateCopyBehavior extends TemplateDownloadBehavior {
	constructor(feature, data) {
		super(feature, data);
	}
	onComplete(handler, message) {
		if (message)
			this.offset += this.blobs[this.index].size;
		this.index++;
		if (this.index < this.blobs.length) {
			let blob = this.blobs[this.index];
			message = new Message(blob.src);
			handler.download(message, blob.dst);
		}
		else {
			Handler.remove(handler);
			this.onCompleteAll(handler, message);
		}
	}
	onInvoke(handler, message) {
		this.blobs = [];
		this.items = [];
		this.offset = 0;
		this.size = 0;
		this.onInvokeDirectory(handler, this.sample.url, this.base, 1);
		this.index = -1;
		this.onComplete(handler, null);
	}
	onInvokeDirectory(handler, src, dst, depth) {
		Files.ensureDirectory(dst);
		let iterator = new Files.Iterator(src), info;
		while (info = iterator.getNext()) {
			let name = info.path;
			let url = mergeURI(dst, name);
			if (info.type == Files.directoryType) {
				url += "/";
				if (depth == 1)
					this.items.push({ depth, kind:"folder", name, url, expanded:true, items:[] });
				else if (depth == 2)
					this.items.push({ depth, kind:"folder", name, url, expanded:false, items:[] });
				this.onInvokeDirectory(handler, mergeURI(src, info.path + "/"), url, depth + 1);
			}
			else if (name != "thumbnail.jpg") {
				if (depth <= 2)
					this.items.push({ depth, kind:"file", name, url });
				this.blobs.push({ src:mergeURI(src, info.path), dst:mergeURI(dst, info.path) });
				this.size += info.size;
			}
		}
	}
};

// ASSETS

import { 
	menuLineSkin,
	popupCheckSkin,
	popupItemStyle,
} from "common/menu";

import {
	BLACK,
	PASTEL_GREEN,
	LIGHT_GREEN,
	PASTEL_GRAY,
	LIGHT_GRAY,
	DARKER_GRAY,
	GRAY,
	WHITE,
	ORANGE,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	fileGlyphsSkin,
	grayBodySkin,
	grayBorderSkin,
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

const sampleHeaderStyle = tableHeaderStyle;
const sampleDescriptionShortStyle = new Style({ font:NORMAL_FONT, size:12, color:"#404040", horizontal:"left", leading:16, lines:3 });
const sampleDescriptionLongStyle = new Style({ font:NORMAL_FONT, size:12, color:"#404040", horizontal:"left", leading:16 });

const sampleTagTitleStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"right", });;
const sampleTagItemStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:BLACK, horizontal:"left", right:5 });;
const samplesLoadingStyle = new Style({ font:NORMAL_FONT, size:14, color:GRAY, horizontal:"left", left:10 });

// BEHAVIORS

import { 
	ButtonBehavior, 
} from "common/control";

import { 
	PopupDialog, 
	MenuItemBehavior, 
} from "common/menu";

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

import {
	FolderTableBehavior,
} from "features/files/files";

class SamplePaneBehavior extends FeaturePaneBehavior {
	onDisplaying(container) {
		container.distribute("onDeviceSelected", model.devicesFeature.currentDevice);
		container.distribute("onMachinesChanged", model.debugFeature.machines, model.debugFeature.debuggees);
		container.distribute("onMachineSelected", model.debugFeature.currentMachine);
		container.distribute("onSamplesChanged");
	}
};

class SampleTagButtonBehavior extends LineBehavior {
	onTap(line) {
		let samples = this.data.samples;
		let data = {
			ItemTemplate:SampleTagItemLine,
			button: line.first.next,
			items:samples.tagItems,
			value:samples.tagItems[samples.tagSelection],
			context: shell,
		};
		shell.add(new PopupDialog(data));
	}
	onMenuSelected(button, tagItem) {
		if (!tagItem) return;
		let data = this.data;
		data.samples.tagSelection = data.samples.tagItems.indexOf(tagItem);
		data.TAG.string = tagItem.title;
		shell.distribute("onSamplesChanged");
	}
};

class SampleColumnBehavior extends HolderColumnBehavior {
	onCreate(column, data) {
		this.data = data;
	}
	onDisplaying(column) {
		let feature = this.data;
		if 	(feature.tree)
			this.onSamplesChanged(column);
	}
	onSamplesChanged(column) {
		column.empty();
		let samples = this.data.samples;
		if (samples.items.length) {
			let mask = samples.tagItems[samples.tagSelection].mask;
			samples.items.forEach(item => {
				if (item.mask & mask)
					column.add(new SampleTable(item));
			});
		}
		else if (this.data.handler)
			column.add(new SampleSpinner(this.data));
		else
			column.add(new NoSamplesLine(this.data));
	}
}

class SampleSpinnerBehavior extends Behavior {
	onLoaded(picture) {
		picture.origin = {x:picture.width>>1, y:picture.height>>1};
		picture.scale = {x:1.0, y:1.0};
		picture.rotation = 0;
		picture.start();
	}	
	onTimeChanged(picture) {
		var rotation = picture.rotation;
		rotation -= 2;
		if (rotation < 0) rotation = 360;
		picture.rotation = rotation;
	}
};

class SampleTableBehavior extends FolderTableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			if (data.descriptionVisible) {
				column.add(new SampleDescription(data));
				this.emptyIndex = 2;
			}
			else
				this.emptyIndex = 1;
			if (data.url)
				this.onDirectoryChanged(column);
		}
		else {
			header.behavior.expand(header, false);
		}
		column.add(new SampleFooter(data));
	}
	hold(column) {
		return SampleHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		this.expand(column, data.expanded);
	}
	onSampleDeleted(column, data) {
		if (this.data == data)
			this.expand(column, data.expanded);
	}
	onSampleDownloaded(column, data) {
		if (this.data == data)
			this.expand(column, data.expanded);
	}
}

class SampleHeaderBehavior extends HeaderBehavior {
	changeArrowState(line, state) {
		line.last.first.next.state = state;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
		if (data.url) {
			line.skin = greenHeaderSkin;
		}
	}
	onDeleteSample(line) {
		let table = this.held ? line.container.previous.first.behavior.table : line.container;
		table.container.behavior.data.delete(this.data);
	}
	onDownloadSample(line) {
		let table = this.held ? line.container.previous.first.behavior.table : line.container;
		table.container.behavior.data.download(this.data);
	}
	onProjectChanged(line, project) {
		if (this.data == project)
			line.last.TITLE.string = project.title;
	}
	onSampleDeleted(line, data) {
		if (this.data == data) {
			line.skin = grayHeaderSkin;
			line.first.width = 0;
			line = line.last;
			line.TITLE.string = data.title;
			line.empty(3);
			line.add(new DownloadSampleButton(data));
		}
	}
	onSampleDownloaded(line, data) {
		if (this.data == data) {
			line.skin = greenHeaderSkin;
			line.first.width = 0;
			line = line.last;
			line.TITLE.string = data.title;
			line.empty(3);
			line.add(new DeleteSampleButton(data));
			line.add(new ToggleSampleDescriptionButton(data));
			line.add(new RunSampleButton(data));
		}
	}
	onSampleDownloading(line, data, offset, size) {
		if (this.data == data) {
			var bar = line.first;
			bar.width = Math.round(line.width * offset / size);
		}
	}
	onToggleSampleDescription(line) {
		var data = this.data;
		data.descriptionVisible = !data.descriptionVisible;
		let table = this.held ? line.container.previous.first.behavior.table : line.container;
		table.behavior.expand(table, data.expanded);
	}
	reveal(line, revealIt) {
		line = line.last;
		if (this.data.url) {
			let button = line.last.previous;
			button.visible = revealIt;
			button.previous.visible = revealIt;
		}
		else {
			line.last.visible = revealIt;
		}
	}
}

// TEMPLATES

import {
	DeviceHeader,
} from "features/devices/devices"

import {
	WaitContent,
} from "features/devices/behaviors"

var SamplePane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	Behavior: SamplePaneBehavior,
	contents: [
		DeviceHeader(model.devicesFeature, { }),
		Container($, {
			left:10, right:10, top:100, height:30, skin:orangeHeaderSkin, active:true,
			Behavior:SampleTagButtonBehavior,
			contents: [
				Label($, { left:10, right:10, height:30, style:tableHeaderStyle, string:"SAMPLES" }),
				Container($, {
					width:180, height:30,
					contents: [
						Label($, { anchor:"TAG", left:0, right:0, state:1, style:whiteButtonStyle, string:$.samples.tagItems[$.samples.tagSelection].title }),
					]
				}),
				Content($, { width:30, right:0, height:30, state:1, skin:whiteButtonsSkin, variant:8 }),
			],
		}),
		Scroller($, {
			left:0, right:0, top:135, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					anchor:"LIST", left:10, right:10, top:0, Behavior:SampleColumnBehavior, 
				}),
				VerticalScrollbar($, {}),
			]
		}),
		Container($, {
			left:10, right:10, top:135, height:40, clip:true, Behavior:HolderContainerBehavior, 
		}),
	],
}));

var SampleTagItemLine = Line.template($ => ({
	left:0, right:0, height:30, skin:menuLineSkin, active:true, Behavior:MenuItemBehavior,
	contents: [
		Content($, { width:30, height:30, skin:popupCheckSkin, visible:false }),
		Label($, { left:0, height:30, style:popupItemStyle, string:$.title }),
	],
}));

var SampleSpinner = Line.template($ => ({
	left:0, right:0, height:30, 
	contents: [
		WaitContent($, {}),
		Label($, { left:0, right:0, height:30, style:samplesLoadingStyle, string:"Loading..." }),
	],
}));

var NoSamplesLine = Line.template($ => ({ 
	left:0, right:0, height:30, 
	contents: [
		WaitContent($, { 
			Behavior: class extends Behavior {
				onDisplaying(content) {
					content.variant = 24
				}
			}
		}),
		Label($, { left:0, right:0, height:30, style:samplesLoadingStyle, string:"No samples!" }),
	],
}));


var SampleTable = Column.template($ => ({
	left:0, right:0, active:true,
	Behavior: SampleTableBehavior,
	contents: [
		SampleHeader($, {}),
	],
}));

var SampleHeader = Container.template($ => ({
	left:0, right:0, height:30, skin:grayHeaderSkin, active:true,
	Behavior: SampleHeaderBehavior,
	contents: [
		Content($, { left:0, width:0, height:30, skin:greenHeaderSkin }),
		Line($, {
			left:0, right:0, height:30,
			contents: [
				Content($, { width:0 }),
				Content($, { width:30, height:30, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:1 }),
				Label($, { name:"TITLE", left:0, right:0, style:sampleHeaderStyle, string:$.title }),
				$.url ? [
					DeleteSampleButton($, {}),
					ToggleSampleDescriptionButton($, {}),
					RunSampleButton($, {}),
				] : [
					DownloadSampleButton($, {}),
				],
			],
		}),
	]
}));

var DeleteSampleButton = Content.template($ => ({
	name:"onDeleteSample", width:30, height:30, skin:whiteButtonsSkin, variant:11, active:true, visible:false, 
	Behavior:ButtonBehavior
}));

var DownloadSampleButton = Content.template($ => ({
	name:"onDownloadSample", width:30, height:30, skin:whiteButtonsSkin, variant:9, active:true, visible:false, 
	Behavior:ButtonBehavior
}));

var RunSampleButton = Content.template($ => ({
	width:30, height:30, skin:whiteButtonsSkin, variant:2, active:true, visible:false, 
	Behavior:class extends ButtonBehavior {
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
	},
}));

var ToggleSampleDescriptionButton = Content.template($ => ({
	name:"onToggleSampleDescription", width:30, height:30, skin:whiteButtonsSkin, variant:10, active:true, visible:false, 
	Behavior:ButtonBehavior
}));

var SampleDescription = Column.template($ => ({
	left:0, right:0, skin:greenLineSkin,
	contents: [
		Text($, { 
			 left:5, right:5, top:5, style:sampleDescriptionLongStyle, 
			 blocks:[
				{ spans: [
					{ content: new SampleThumbnail($), align:"left"  },
					{ string:$.description },
				] }
			] 
		}),
		Content($, { height:5 }),
	],
}));

var SampleThumbnail = Container.template($ => ({
	width:69, height:48,
	contents:[
		Content($, { left:1, width:64, top:0, height:48, skin:grayBodySkin }),
		Thumbnail($, { left:1, width:64, top:0, height:48, url:$.thumbnail }),
	],
}));

var SampleFooter = Line.template($ => ({
	left:0, right:0, height:5, skin:greenFooterSkin,
}));

