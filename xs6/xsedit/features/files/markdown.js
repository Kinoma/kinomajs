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
// ASSETS

import {
	bulletSkin,
	menuLineSkin,
} from "common/menu";

import {
	DARK_GRAY,
	DARKER_CYAN,
	DARKER_GRAY,
	DARKER_GREEN,
	LIGHT_GRAY,
	PASTEL_CYAN,
	PASTEL_GRAY,
	PASTEL_YELLOW,
	WHITE,
	grayBorderSkin,
	grayHeaderSkin,
	tableHeaderStyle,
	whiteButtonSkin,
	whiteButtonsSkin,
	whiteButtonStyle,
} from "shell/assets";

const CODE_COLOR = "#202020";
const LINK_COLOR = [ DARKER_GREEN, DARKER_GREEN, DARKER_CYAN, DARKER_CYAN ];
const TABLE_BORDER_COLOR = "#808080";
const TABLE_HEADER_COLOR = "#fafafa";
const TEXT_COLOR = "#505050";

const BOLD_FONT = "Open Sans Bold";
const BOLD_ITALIC_FONT = "Open Sans Bold Italic";
const ITALIC_FONT = "Open Sans Italic";
const NORMAL_FONT = "Open Sans";
const CODE_FONT = "Menlo";

export var markdownOptions = {
	skinHorizontalRule:new Skin({ texture:new Texture("assets/horizontalRule.png", 1), x:0, y:0, width:1000, height:20 }),
//	skinHTMLTableData:new Skin({ fill:"transparent", stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:0, right:1, bottom:0 }}),
//	skinHTMLTableHead:new Skin({ fill:TABLE_HEADER_COLOR, stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:1, right:1, bottom:1 }}),
//	skinHTMLTableRowFirst:new Skin({ fill:"transparent", stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:1, right:0, bottom:1 }}),
//	skinHTMLTableRowNext:new Skin({ fill:"transparent", stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:0, right:0, bottom:1 }}),
	styleBlockquoteFirst:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:50, top:10 }),
	styleBlockquoteNext:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:50, top:0 }),
	styleCodeFirst:new Style({ font:CODE_FONT, size:14, color:CODE_COLOR, horizontal:"left", left:30, top:10 }),
	styleCodeNext:new Style({ font:CODE_FONT, size:14, color:CODE_COLOR, horizontal:"left", left:30, top:0 }),
	styleCodeSpan:new Style({ font:CODE_FONT, color:CODE_COLOR }),
	styleColumn:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:30, top:0 }),
	styleHeader1:new Style({ font:BOLD_FONT, size:32, color:TEXT_COLOR, horizontal:"left", left:10, top:20 }),
	styleHeader2:new Style({ font:BOLD_FONT, size:24, color:TEXT_COLOR, horizontal:"left", left:10, top:20 }),
	styleHeader3:new Style({ font:BOLD_FONT, size:20, color:TEXT_COLOR, horizontal:"left", left:10, top:15 }),
	styleHeader4:new Style({ font:BOLD_FONT, size:18, color:TEXT_COLOR, horizontal:"left", left:10, top:15 }),
	styleHeader5:new Style({ font:BOLD_FONT, size:16, color:TEXT_COLOR, horizontal:"left", left:10, top:15 }),
	styleHeader6:new Style({ font:BOLD_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:10, top:15 }),
	styleHTMLTableData:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:5, top:5, right:5, bottom:5 }),
	styleHTMLTableHead:new Style({ font:BOLD_FONT, size:14, color:TEXT_COLOR, horizontal:"center", left:0, top:5, right:0, bottom:5 }),
	styleLinkSpan:new Style({ font:NORMAL_FONT, color:LINK_COLOR }),
	styleListFirst:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:20, top:10 }),
	styleListNext:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:20, top:0 }),
	styleParagraph:new Style({ font:NORMAL_FONT, size:14, color:TEXT_COLOR, horizontal:"left", left:10, top:10 }),
	styleTextSpan:new Style({ font:NORMAL_FONT, color:TEXT_COLOR }),
	styleX0NormalSpan:new Style({ font:NORMAL_FONT }),
	styleX1ItalicSpan:new Style({ font:ITALIC_FONT }),
	styleX2BoldSpan:new Style({ font:BOLD_FONT }),
	styleX3BoldItalicSpan:new Style({ font:BOLD_ITALIC_FONT }),
	columnWidth:200,
	defaultCodeType:"javascript",
	AnchorContent: Layout.template($ => ({
		width:0, height:0,
		Behavior: class extends Behavior {
			onCreate(layout, data) {
				this.at = data;
			}
			onAdapt(layout) {
				let container = layout.container;
				let map = container.behavior.map;
				if (this.index == 1) {
					map.y = container.container.scroll.y;
					map.by = 0;
				}
				let former = map.get(this.at);
				let current = layout.y - container.y;
				map.set(this.at, current);
				if (map.y >= former)
					map.by = current - former;
				if (this.index == map.size)
					container.container.scrollBy(0, map.by);
			}
			onDisplaying(layout) {
				let container = layout.container;
				let map = container.behavior.map;
				map.set(this.at, layout.y - container.y);
				this.index = map.size;
			}
		}
	})),
	CodeContent: Container.template($ => ({
		contents:[
			Code($, {
				left:50, top:20, skin:textSkin, style:codeBlockStyle, active:true, selectable:true, string:$.string,
				Behavior: class extends Behavior {
					onCreate(code, data, dictionary) {
						super.onCreate(code, data, dictionary);
						code.type = data.type; // set type
					}
				}
			}),
			Container($, {
				left:40, right:0, top:10, bottom:0, skin:codeButtonSkin, active:true,
				Behavior: class extends ButtonBehavior {
					onTap(container) {
						let code = container.container.first;
						shell.behavior.setClipboard(code.string)
					}
				},
				contents: [
					Label($, { right:3, top:-3, style:codeButtonStyle, string:"copy" }),
				],
			}),
		],
	})),
	LinkBehavior: class {
		constructor(url) {
			this.url = url;
		}
		onMouseEntered(content, x, y) {
			content.state = 2;
		}
		onMouseExited(content, x, y) {
			content.state = 1;
		}
		onTouchBegan(content) {
			content.state = 2;
		}
		onTouchEnded(content) {
			content.state = 1;
			let url = this.url; // check absolute URL param first
			if (url.startsWith("http://") || url.startsWith("https://")) {
				launchURI(url);
			}
			else {
				let parts = parseURI(url);
				let at = parts.fragment;
				delete parts.fragment;
				if (!parts.path.endsWith(".md")) {
					if (!parts.path.endsWith("/"))
						parts.path += "/";
					let name, names = parts.path.split("/");
					while (!name && names.length)
						name = names.pop();
					parts.path += name + ".md";
				}
				url = serializeURI(parts);
				//if (at) {
					shell.delegate("doOpenURL", url, at);
				//}
				//else {
				//	shell.distribute("onMarkdownOpenURL", url);
				//}
			}
		}
	},
	SpanStyleHelper: function SpanStyleHelper(value) {
		let matches = value.match(/^color: ?(black|blue|cyan|green|orange|red|yellow);?$/);
		if (matches && matches.length) {
			if (matches[1] == "cyan") return new Style({ color: "aqua" });
			else return new Style({ color: matches[1] });
		}
	},
	TableClassHelper: function TableClassHelper(value) {
		if (value == "normalTable")
			return {
				skinHTMLTableData:new Skin({ fill:"transparent", stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:0, right:1, bottom:0 }}),
				skinHTMLTableHead:new Skin({ fill:TABLE_HEADER_COLOR, stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:1, right:1, bottom:1 }}),
				skinHTMLTableRowFirst:new Skin({ fill:"transparent", stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:1, right:0, bottom:1 }}),
				skinHTMLTableRowNext:new Skin({ fill:"transparent", stroke:TABLE_BORDER_COLOR, borders:{ left:0, top:0, right:0, bottom:1 }}),
				columnWidth:240
			}
	},
}

const SEMIBOLD_FONT = "Open Sans Semibold";

const codeBlockStyle = new Style({ font:CODE_FONT, size:14, horizontal:"left", left:8, right:8, color:["black", "#103ffb", "#b22821", "#008d32"] });
const codeButtonSkin = new Skin({ stroke:["transparent", PASTEL_GRAY, PASTEL_GRAY, LIGHT_GRAY], borders:{ left:1, top:1, right:1, bottom:1 }});
const codeButtonStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:["transparent", "transparent", LIGHT_GRAY, DARK_GRAY] });

const outlineLabelStyle1 = new Style({ font: BOLD_FONT, size:12, color:"black", horizontal:"left", left:5, right:5});
const outlineLabelStyle2 = new Style({ font: NORMAL_FONT, size:12, color:"black", horizontal:"left", left:15, right:5});
const outlineLabelStyle3 = new Style({ font: NORMAL_FONT, size:12, color:"black", horizontal:"left", left:25, right:5});
const outlineLabelStyle4 = new Style({ font: NORMAL_FONT, size:12, color:"black", horizontal:"left", left:35, right:5});

const textSkin = new Skin({ fill:[PASTEL_YELLOW, "transparent", "#e0e0e0", PASTEL_CYAN] });

// BEHAVIORS

import {
	ButtonBehavior,
	FieldLabelBehavior,
	FieldScrollerBehavior,
} from "common/control";

import {
	MenuButtonBehavior,
	MenuItemBehavior,
} from "common/menu";

import {
	CodeBehavior,
} from "shell/behaviors";

class MarkdownViewBehavior extends Behavior {
	canFind(container) {
		return true;
	}
	canFindNext(container) {
		return this.resultCount > 0;
	}
	canFindPrevious(container) {
		return this.resultCount > 0;
	}
	canFindSelection(container) {
		return false;
	}
	canReplace(container) {
		return false;
	}
	canReplaceAll(container) {
		return false;
	}
	canReplaceNext(container) {
		return false;
	}
	canReplacePrevious(container) {
		return false;
	}
	canReplaceSelection(container) {
		return false;
	}
	doFind(container) {
		var data = this.data;
		var findLine = data.FIND;
		var markdown = data.MARKDOWN;
		markdown.behavior.find(markdown, data.findString, data.findMode);
		data.FIND_FOCUS.focus();
		if (!findLine.visible)
			container.run(new FindTransition, container.first, findLine, undefined, 1);
	}
	doFindNext(container) {
		var markdown = this.data.MARKDOWN;
		markdown.focus();
		markdown.behavior.findAgain(markdown, 1);
	}
	doFindPrevious(container) {
		var markdown = this.data.MARKDOWN;
		markdown.focus();
		markdown.behavior.findAgain(markdown, -1);
	}
	onCreate(container, data) {
		this.data = data;
		this.resultCount = 0;
	}
	onFindDone(container) {
		var data = this.data;
		var findLine = data.FIND;
		var markdown = data.MARKDOWN;
		markdown.behavior.find(markdown, "", 0);
		if (findLine.visible)
			container.run(new FindTransition, container.first, findLine, undefined, 0);
	}
	onFindEdited(container) {
		var data = this.data;
		var markdown = data.MARKDOWN;
		markdown.behavior.find(markdown, data.findString, data.findMode);
	}
	onFound(container, resultCount) {
		this.resultCount = resultCount;
	}
	onKeyDown(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if (c == 27)
			return true;
		return false;
	}
	onKeyUp(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if (c == 27) {
			this.onFindDone(container);
			return true;
		}
		return false;
	}
	onMarkdownTitleChanged(container, string) {
		var title = this.data.TITLE;
		title.string = string;
		return true;
	}
};

class MarkdownTextBehavior extends Behavior {
	doSelectLine(markdown, at) {
		if (at) {
			let offset = this.map.get(at);
			if (offset !== undefined)
				markdown.container.scrollTo(0, offset);
		}
	}
	find(markdown, findString, findMode) {
		this.results.forEach(result => result.content.select(0, 0));
		if (findString && /[^\.]+/.test(findString)) {
			this.results = markdown.find(findModeToPattern(findMode, findString), findModeToCaseless(findMode));
			this.resultCount = this.results.length;
			if (0 < this.resultCount)
				this.findResult(markdown, 0);
			else {
				this.resultIndex = -1;
				markdown.container.scrollTo(0, 0);
			}
		}
		else {
			this.results = [];
			this.resultCount = 0;
			this.resultIndex = -1;
		}
		markdown.container.container.container.distribute("onFound", this.resultCount);
	}
	findAgain(markdown, direction) {
		this.results.forEach(result => result.content.select(0, 0));
		let index = this.resultIndex;
		index += direction;
		if (index >= this.resultCount)
			index = 0;
		if (index < 0)
			index = this.resultCount - 1;
		this.findResult(markdown, index);
	}
	findResult(markdown, index) {
		if (this.resultCount > 0) {
			let result = this.results[index];
			let content = result.content;
			content.select(result.offset, result.length);
			let bounds = content.selectionBounds;
			bounds.x += content.x - markdown.x;
			bounds.y += content.y - markdown.y;
			markdown.container.reveal(bounds);
			this.resultIndex = index;
		}
		else
			debugger;
	}
	onCreate(markdown, data, dictionary) {
		this.data = data;
		this.document = null;
		this.map = new Map();
		this.resultCount = 0;
		this.resultIndex = 0;
		this.results = [];
	}
	onDisplayed(markdown) {
		let data = this.data;
		let at = data.at;
		if (at !== undefined) {
			this.doSelectLine(markdown, at);
			data.at = undefined;
		}
	}
	onDisplaying(markdown) {
		let data = this.data;
		if (Files.exists(data.url)) {
			this.notifier = new Files.DirectoryNotifier(data.url, url => {
				this.onFileChanged(markdown);
			});
			this.onFileChanged(markdown);
		}
		else
			debugger
		markdown.focus(); // make searchable
	}
	onFileChanged(markdown) {
		let data = this.data;
		let url = markdownOptions.url = data.url;
		let metaData = markdown.formatMarkdown(Files.readText(url), markdownOptions);
		let applicationPath = getEnvironmentVariable("applicationPath");
		if (Files.toPath(url).indexOf(applicationPath) == 0)
			markdown.bubble("onMarkdownTitleChanged", metaData.title);
		let button = data.MARKERS;
		let markers = [];
		if (metaData.items) {
			for (let h1 of metaData.items) {
				if (h1) {
					if (h1.title)
						markers.push({ anchor:h1.anchor, style:outlineLabelStyle1, title:h1.title });
					if (h1.items) {
						for (let h2 of h1.items) {
							if (h2) {
								if (h2.title)
									markers.push({ anchor:h2.anchor, style:outlineLabelStyle2, title:h2.title });
								if (h2.items) {
									for (let h3 of h2.items) {
										if (h3) {
											if (h3.title)
												markers.push({ anchor:h3.anchor, style:outlineLabelStyle3, title:h3.title });
											if (h3.items) {
												for (let h4 of h3.items) {
													if (h4) {
														if (h4.title)
															markers.push({ anchor:h4.anchor, style:outlineLabelStyle4, title:h4.title });
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
			}
		}
		button.behavior.onMarkersChanged(button, markers);
		Promise.resolve(markdown).then(markdown => {
			this.onScrolled(markdown);
			markdown.focus();
		});
	}
	onScrolled(markdown) {
		let data = this.data;
		let scroller = markdown.container;
		shell.distribute("onMarkdownScrolled", this.map, data.url, scroller.y, scroller.scroll.y);
	}
	onUndisplayed(markdown) {
		let data = this.data;
		let document = this.document;
		let notifier = this.notifier;
		if (document) {
			document.MARKDOWN = null;
			document.state = {
				// TODO
			}
		}
		else if (notifier)
			notifier.close();
	}
};

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	FindField,
	findModeToCaseless,
	findModeToPattern,
} from "find";

export var MarkdownView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	Behavior: MarkdownViewBehavior,
	contents: [
		Container($, {
			left:0, right:10, top:30, bottom:10,
			contents: [
				Scroller($, {
					left:0, right:0, top:0, bottom:0, clip:true, active:true, 
					Behavior: ScrollerBehavior,
					contents: [
						Text($, { anchor:"MARKDOWN", left:1, right:1, top:0, Behavior:MarkdownTextBehavior, active:true, selectable:true, skin:textSkin, state:1 }),
						HorizontalScrollbar($, { bottom:-10 }),
						VerticalScrollbar($, { right:-10 }),
					],
				}),
			],
		}),
		Content($, {
			left:0, right:10, top:30, bottom:10, skin:grayBorderSkin,
		}),
		FindLine($, { anchor:"FIND" }),
		Line($, {
			left:0, right:10, top:0, height:30, skin:grayHeaderSkin, active:true, 
			Behavior: class extends Behavior {
				onCreate(line, data) {
					this.data = data;
				}
				onDocumentChanged(line, document) {
					if (this.data.url == document.url)
						line.first.visible = document.dirty;
				}
			},
			contents: [
				Content($, { width:10 }),
				Label($, { anchor:"TITLE", left:0, right:0, style:tableHeaderStyle, string:Files.toPath($.url) }),
				Content($, {
					anchor:"MARKERS", width:30, height:30, skin:whiteButtonsSkin, variant:7, state:0, active:false,
					Behavior: class extends MenuButtonBehavior {
						onDescribeMenu(button) {
							let data = this.data;
						//	let markdown = this.data.MARKDOWN;
						//	let bounds = markdown.selectionBounds;
						//	let line = Math.floor(bounds.y / markdown.lineHeight) + 1;
							let markers = this.markers;
							return {
								ItemTemplate:MarkerItemLine,
								items:markers,
								horizontal:"right",
						//		selection:markers.findIndex(marker => marker.line == line),
								context: shell,
							};
						}
						onMarkersChanged(button, markers) {
							this.markers = markers;
							button.active = markers.length > 0;
						}
						onMenuSelected(button, selection) {
							let markdown = this.data.MARKDOWN;
							markdown.behavior.doSelectLine(markdown, selection.anchor);
						}
					},
				}),
				Content($, { 
					width:30, height:30, skin:whiteButtonsSkin, variant:0, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("doClose");
						}
					},
				}),
			],
		}),
	],
}));

var MarkerItemLine = Line.template($ => ({
	left:0, right:0, height:20, skin:menuLineSkin, active:true, Behavior:MenuItemBehavior,
	contents: [
		Label($, { left:0, height:20, style:$.style, string:$.title }),
	],
}));

var FindLine = Line.template(function($) { return {
	left:0, right:10, top:30, height:30, skin:grayHeaderSkin, visible:false,
	contents: [
		Content($, { width:10 }),
		FindField($, {}),
		Content($, {
			width:40, skin:whiteButtonsSkin, variant:3, active:false,
			Behavior: class extends ButtonBehavior {
				onFound(button, resultCount) {
					button.active = resultCount > 0;
				}
				onTap(button) {
					button.bubble("doFindPrevious");
				}
			},
		}),
		Content($, {
			width:40, skin:whiteButtonsSkin, variant:4, active:false,
			Behavior: class extends ButtonBehavior {
				onFound(button, resultCount) {
					button.active = resultCount > 0;
				}
				onTap(button) {
					button.bubble("doFindNext");
				}
			},
		}),
		Container($, {
			width:60, skin:whiteButtonSkin, active:true,
			Behavior: class extends ButtonBehavior {
				onTap(button) {
					button.bubble("onFindDone");
				}
			},
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Done" }),
			],
		}),
	]
}});

// TRANSITIONS

class FindTransition extends Transition {
	constructor() {
		super(250);
	}
	onBegin(container, body, findLine, replaceLine, to) {
		var from = 0;
		if (findLine.visible) from++;
		this.container = container;
		this.body = body;
		this.bodyFrom = body.y;
		if (from == 0) {
			findLine.visible = true;
			this.bodyTo = body.y + findLine.height;
			this.findLine = findLine
			this.findFrom = body.y - findLine.height;
			this.findTo = body.y;
		}
		else if (from == 1) {
			this.bodyTo = findLine.y;
			this.findLine = findLine
			this.findFrom = findLine.y;
			this.findTo = findLine.y - findLine.height;
		}
		else {
			this.bodyTo = findLine.y;
			this.findLine = findLine
			this.findFrom = findLine.y;
			this.findTo = findLine.y - findLine.height - replaceLine.height;
		}
		if (this.findLine)
			this.findLine.visible = true;;
		body.coordinates = { left:0, right:10, top:this.bodyFrom, height:container.y + container.height - 10 - this.bodyFrom }
		findLine.first.state = to > 1 ? 3 : 1;
	}
	onEnd(container, body, findLine, replaceLine, to) {
		findLine.visible = to > 0;
		body.coordinates = { left:0, right:10, top:body.y - container.y, bottom:10 };
		if (to == 0)
			body.first.first.focus();
		else
			this.container.behavior.data.FIND_FOCUS.focus();
	}
	onStep(fraction) {
		fraction = Math.quadEaseOut(fraction);
		if (this.findLine)
			this.findLine.y = this.findFrom + ((this.findTo - this.findFrom) * fraction);
		this.body.y = this.bodyFrom + ((this.bodyTo - this.bodyFrom) * fraction);
		this.body.height = this.container.y + this.container.height - 10 - this.body.y;
	}
};
