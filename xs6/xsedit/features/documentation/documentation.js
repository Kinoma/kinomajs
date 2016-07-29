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

export default class extends Feature {
	constructor(model, url) {
		super(model, "Documentation");
		this.Template = DocumentationPane;
		this.docsURL = mergeURI(url, "./docs.json");
		this.documentation = this.preferences = this.user = undefined;
		this.documentationURL = mergeURI(model.cacheDirectory, "./documentation.json");
		this.iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60, states:60 });
	}
	open() {
		if (this.documentation === undefined) {
			let json = {};
			if (Files.exists(this.documentationURL))
				json = Files.readJSON(this.documentationURL);
			let docs = Files.readJSON(this.docsURL);
			let files = docs.files;
			let force = docs.force;
			let magic = 1;
			let write = false;
			if (!force && ("documentation" in json)) {
				let documentation = this.documentation = json.documentation;
				if (("magic" in documentation) && (documentation.magic == magic)) {
					let items = documentation.items;
					for (let file of files) {
						let path = mergeURI(this.docsURL, file);
						let info = Files.getInfo(path);
						if (file in items) {
							let item = items[file];
							if (info.date == item.date) {
								info = undefined;
							}
						}
						if (info) {
							let item = KPR.parseMarkdown(Files.readText(path), 0);
							if (item) {
								item.date = info.date;
								items[file] = item;
								write = true;
							}
						}
					}
					for (let key in items) {
						if (files.indexOf(key) == -1) {
							delete items[key];
							write = true;
						}
					}
				}
				else {
					force = true;
				}
			}
			else {
				force = true;
			}
			if (force) {
				this.documentation = { items:{}, magic:magic };
				for (let file of files) {
					let path = mergeURI(this.docsURL, file);
					let info = Files.getInfo(path);
					let item = KPR.parseMarkdown(Files.readText(path), 0);
					if (item) {
						item.date = info.date;
						this.documentation.items[file] = item;
					}
				}
				write = true;
			}
			if (write) {
				json = { documentation: this.documentation };
				Files.writeText(this.documentationURL, JSON.stringify(json, null, 4));
			}
			this.docs = [];
			for (let file of docs.files) {
				let path = mergeURI(this.docsURL, file);
				if (Files.exists(path)) {
					let item = this.documentation.items[file];
					if (item) {
						this.docs.push(Object.assign({
							expanded: false,
							url: mergeURI(this.docsURL, file),
							variant: this.docs.length ? 0 : 1,
						}, item));
					}
				}
			}
			if (this.user) {
				let path = this.user.path;
				if (path && path.startsWith("/") && Files.exists(path)) {
					let item = KPR.parseMarkdown(Files.readText(path), 0);
					this.docs.push(Object.assign({
						expanded: false,
						url: Files.toURI(path),
						variant: 2,
					}, item));
				}
			}
		}
	}
	read(json) {
		if ("documentation" in json) {
			this.preferences = json.documentation.preferences;
			this.user = json.documentation.user;
		}
		if (this.preferences === undefined) {
			this.preferences = {};
		}
	}
	write(json) {
		json.documentation = { preferences: this.preferences, user: this.user };
	}
}

// ASSETS

import {
	BLACK,
	NORMAL_FONT,
	grayHeaderSkin,
	grayFooterSkin,
	grayLineSkin,
	greenHeaderSkin,
	greenFooterSkin,
	greenLineSkin,
	orangeHeaderSkin,
	orangeFooterSkin,
	orangeLineSkin,
	tableHeaderStyle,
	tableLineStyle,
} from "shell/assets";

const DOCUMENTATION_HEADER_COLOR = "#dcdcdc";
const DOCUMENTATION_INTRO_TEXT_COLOR = "#505050";

const documentationHeaderSkin = new Skin({ fill:DOCUMENTATION_HEADER_COLOR });
const documentationHeaderStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left" });

const documentationFooterSkins = [ greenFooterSkin, orangeFooterSkin, grayFooterSkin ];
const documentationHeaderSkins = [ greenHeaderSkin, orangeHeaderSkin, grayHeaderSkin ];
const documentationLineSkins = [ greenLineSkin, orangeLineSkin, grayLineSkin ];
const documentationLineStyles = [ tableLineStyle, tableLineStyle, tableLineStyle ];

const tileSelectionTexture = new Texture("assets/tile-sm.png", 2);
const tileSelectionSkin = new Skin({ texture:tileSelectionTexture, x:0, y:0, width:10, height:10, states:10, variants:10 });

// BEHAVIORS

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
	FeaturePaneBehavior,
} from "shell/feature";

class DocumentationLineBehavior extends LineBehavior {
	getPreferences(line) {
		return line.container.container.container.container.behavior.data.preferences;
	}
	onDisplaying(line) {
		let data = this.data;
		let selected = this.getPreferences(line).selected;
		if (selected && (selected.anchor == data.anchor) && (selected.url == data.url)) {
			Promise.resolve(line).then(line => {
				this.onTap(line);
			});
		}
	}
	onDocumentationScrolled(line, map, url, y, scrollY) {
		let data = this.data;
		if (data.flag) {
			data.flag = false;
		}
		else {
			if (scrollY < (map.get(data.anchor) + y)) {
				line.container.distribute("onDocumentationSelect", false);
				this.select(line, true);
				this.reveal(line);
				return true;
			}
		}
	}
	onDocumentationSelect(line, selectIt, flagIt) {
		if ((selectIt !== false) && (selectIt !== true)) {
			selectIt = (line == selectIt);
			this.data.flag = flagIt;
		}
		this.select(line, selectIt);
		if (selectIt && flagIt) {
			this.reveal(line);
		}
	}
	onMarkdownScrolled(line, map, url, y, scrollY) {
		return this.onDocumentationScrolled(line, map, url, y, scrollY);
	}
	onTap(line) {
		let data = this.data;
		line.container.distribute("onDocumentationSelect", line, true);
		shell.delegate("doOpenURL", data.url, data.anchor);
	}
	reveal(line) {
		let table = line.container;
		let held = table.behavior.held;
		let scroller = table.container.container;
		let column = scroller.first;
		let holder = scroller.next;
		let bounds = line.bounds;
		bounds.x -= column.x;
		bounds.y -= column.y;
		if (held && (line.y < (holder.y + table.first.height))) {
			bounds.y -= table.first.height;
		}
		scroller.reveal(bounds);
	}
	select(line, selectIt) {
		super.select(line, selectIt);
		if (selectIt) {
			let data = this.data;
			let preferences = this.getPreferences(line);
			preferences.selected = { anchor:data.anchor, url:data.url };
		}
	}
}

class DocumentationHeaderBehavior extends HeaderBehavior {
	changeArrowState(line, state) {
		// no arrow
	}
	changeCornerState(line, state) {
		line.last.state = state;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
		this.select(line, data.url == model.url);
	}
	onURLChanged(line, url) {
		this.select(line, this.data.url == url);
	}
	select(line, selectIt) {
		this.changeCornerState(line, selectIt ? 0 : 1);
	}
};

class DocumentationTableBehavior extends TableBehavior {
	expand(table, expandIt) {
		let data = this.data;
		let header = table.first;
		table.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			this.onDocumentationLinesChanged(table);
		}
		else {
			header.behavior.expand(header, false);
		}
		table.add(new DocumentationFooter(data));
	}
	hold(table) {
		return DocumentationHeader(this.data, { left:0, right:0, top:0, height:table.first.height });
	}
	onCreate(table, data) {
		this.data = data;
		this.expand(table, (data.url == model.url) && data.expanded);
	}
	onDocumentationLinesChanged(table) {
		let data = this.data;
		if (data.items) {
			let register = 0;
			for (let h1 of data.items) {
				if (h1) {
					if (register++) {
						h1 = Object.assign({
							left: 5,
							url: data.url,
							variant: data.variant,
						}, h1);
						if (h1.title)
							table.add(new DocumentationLine(h1));
					}
					if (h1.items) {
						for (let h2 of h1.items) {
							if (h2) {
								h2 = Object.assign({
									left: 15,
									url: data.url,
									variant: data.variant,
								}, h2);
								if (h2.title)
									table.add(new DocumentationLine(h2));
								if (h2.items) {
									for (let h3 of h2.items) {
										if (h3) {
											h3 = Object.assign({
												left: 25,
												url: data.url,
												variant: data.variant,
											}, h3);
											if (h3.title)
												table.add(new DocumentationLine(h3));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	onURLChanged(table, url) {
		let data = this.data;
		data.expanded = true;
		this.expand(table, data.url == url);
	}
	toggle(table) {
		let data = this.data;
		if (model.url == data.url) {
			data.expanded = !data.expanded;
			this.expand(table, data.expanded);
		}
		else
			shell.delegate("doOpenURL", data.url);
	}
};

class DocumentationPaneBehavior extends FeaturePaneBehavior {
	onDisplaying(container) {
		this.onDocumentationChanged(container);
	}
	onDocumentationChanged(container) {
		let column = container.first.next.first;
		column.empty();
		this.data.docs.forEach(item => column.add(new DocumentationTable(item)));
	}
};

// TEMPLATES

var DocumentationPane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, clip:true,
	Behavior: DocumentationPaneBehavior,
	contents: [
		Line($, {
			left:10, right:10, top:0, height:30, active:true, skin:documentationHeaderSkin,
			contents: [
				Label($, { left:10, right:10, style:documentationHeaderStyle, string:"DOCUMENTATION" }),
			],
		}),
		Scroller($, {
			left:0, right:0, top:40, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:10, right:10, top:0, Behavior:HolderColumnBehavior, 
					contents: [
					]
				}),
				VerticalScrollbar($, {}),
			]
		}),
		Container($, {
			left:10, right:10, top:40, height:40, clip:true, Behavior:HolderContainerBehavior
		}),
	]
}});

var DocumentationTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: DocumentationTableBehavior,
	contents: [
		DocumentationHeader($, {}),
	],
}});

var DocumentationHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:documentationHeaderSkins[$.variant], active:true,
	Behavior: DocumentationHeaderBehavior,
	contents: [
		Label($, { left:10, right:0, style:tableHeaderStyle, string:$.title||"Untitled" }),
		Content($, { right:0, top:0, width:10, height:10, skin:tileSelectionSkin, state:$.expanded ? 0 : 1, variant:$.variant }),
	],
}});

var DocumentationFooter = Line.template(function($) { return {
	left:0, right:0, height:[ 5, 5, 5 ][$.variant], skin:documentationFooterSkins[$.variant]
}});

var DocumentationLine = Line.template(function($) { return {
	left:0, right:0, skin:documentationLineSkins[$.variant], active:true,
	Behavior: DocumentationLineBehavior,
	contents: [
		Label($, { left:$.left, right:0, style:tableLineStyle, string:$.title }),
	],
}});
