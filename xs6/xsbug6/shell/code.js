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

// ASSETS

import {
	bulletSkin,
	menuLineSkin,
} from "common/menu";

import {
	BLACK,
	BOLD_FIXED_FONT,
	BOLD_FONT,
	GRAYS,
	FIXED_FONT,
	NORMAL_FONT,
	DARKER_GRAY,
	DARKER_RED,
	GRAY,
	LIGHT_CYAN,
	LIGHT_ORANGE,
	ORANGE,
	PASTEL_CYAN,
	PASTEL_ORANGE,
	PASTEL_GREEN,
	PASTEL_YELLOW,
	RED,
	SEMIBOLD_FONT,
	YELLOW,
	WHITE,
	paneBodySkin,
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
	buttonSkin,
	buttonStyle,
	buttonsSkin,
	errorStyle,
	lineNumberSkin,
	lineNumberStyle,
	menuLineStyle,
} from "shell/assets";	

var PASTEL_GRAY = "#f7f7f7";
var LIGHT_GRAY = "#eeeeee";
var LIGHT_BLUE = "#eff3f8";
var PASTEL_BLUE = "#a9d1ff";
var BORDER_GRAY = "#aaaaaa";
var HOVER_GRAY = "#e0e0e0";
var CLICK_GRAY = "#c7c7c7";


var reasonSkin = new Skin({ fill:RED }),
var reasonLabelStyle = new Style({ font:BOLD_FONT, size:12, color:WHITE, left:4, right:4 }),
var reasonNumberStyle = new Style({ 
	font:BOLD_FIXED_FONT,
	size:10, 
	horizontal:"right",
	vertical:"bottom",
	right:6, 
	bottom:1,
	color:WHITE,
}),

var textSkin = new Skin({ fill:[PASTEL_YELLOW, "transparent", "#e0e0e0", PASTEL_CYAN] })
var textStyle = new Style({ 
	font:FIXED_FONT, 
	size:12, 
	horizontal:"left",
	left:8, right:8,
	color: [ "black", "#103ffb", "#b22821", "#008d32" ]
})
var lineNumbersSkin = new Skin({ fill:GRAYS[2], stroke:GRAYS[10], borders: { right:1 } })

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
	CodeScrollerBehavior, 
} from "shell/behaviors";

class CodeViewBehavior extends Behavior {
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
		var code = this.data.CODE;
		return code.selectionLength > 0;
	}
	doFind(container) {
		var data = this.data;
		var code = this.data.CODE;
		var findLine = data.FIND;
		code.behavior.find(code, data.findString, data.findMode);
		data.FIND_FOCUS.focus();
		if (!findLine.visible)
			container.run(new FindTransition, container.first, findLine, null, 1);
	}
	doFindNext(container) {
		var code = this.data.CODE;
		code.focus();
		code.findAgain(1);
		code.behavior.onSelected(code);
		code.behavior.onReveal(code);
	}
	doFindPrevious(container) {
		var code = this.data.CODE;
		code.focus();
		code.findAgain(-1);
		code.behavior.onSelected(code);
		code.behavior.onReveal(code);
	}
	doFindSelection(container) {
		var data = this.data;
		var code = data.CODE;
		var findLine = data.FIND;
		data.findString = code.selectionString;
		data.findMode = 1;
		code.behavior.find(code, data.findString, data.findMode);
		var label = data.FIND_FOCUS;
		label.string = data.findString;
		label.next.visible = data.findString.length == 0;
	}
	onCreate(container, data) {
		this.data = data;
		this.resultCount = 0;
	}
	onCodeSelected(container) {
		var data = this.data;
		var code = this.data.CODE;
		this.resultCount = code.resultCount;
		container.distribute("onFound", this.resultCount);
	}
	onFindDone(container) {
		var data = this.data;
		var code = this.data.CODE;
		var findLine = data.FIND;
		code.behavior.find(code, "", 0);
		if (findLine.visible)
			container.run(new FindTransition, container.first, findLine, null, 0);
	}
	onFindEdited(container) {
		var data = this.data;
		var code = data.CODE;
		code.behavior.find(code, data.findString, data.findMode);
	}
	onKeyDown(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if (c == 9)
			return true;
		if (c == 25)
			return true;
		if (c == 27)
			return true;
		return false;
	}
	onKeyUp(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if (c == 9) {
			var data = this.data;
			if (data.FIND_FOCUS.focused)
				data.CODE.focus();
			return true;
		}
		if (c == 25) {
			var data = this.data;
			if (data.FIND_FOCUS.focused)
				data.CODE.focus();
			return true;
		}
		if (c == 27) {
			this.onFindDone(container);
			return true;
		}
		return false;
	}
	onMouseEntered(container, x, y) {
		if (shell.last == model.WINDOW)
			this.reveal(container, true);
		return true;
	}
	onMouseExited(container, x, y) {
		if (shell.last == model.WINDOW)
			this.reveal(container, false);
		return true;
	}
	reveal(container, flag) {
		let header = container.last;
		let button = header.last;
		button.visible = flag;
		button = button.previous;
		button.visible = flag;
		button = button.previous;
		button.visible = flag;
		button = button.previous;
		button.visible = flag;
	}
};

class CodeEditorBehavior extends CodeBehavior {
	canToggleBreakpoint(code, item) {
		let location = code.locate(code.selectionOffset);
		var lines = this.data.LINES;
		var line = lines.content(Math.floor(location.y / code.lineHeight));
		item.title = item.titles[line.first.state & 1];
		return true;
	}
	find(code, findString, findMode) {
		code.find(findModeToPattern(findMode, findString), findModeToCaseless(findMode));
		this.onSelected(code);
		this.onReveal(code);
	}
	doSelectLine(code, at) {
		var from, to
		if (at.split) {
			var range = at.split("-");
			from = parseInt(range[0]);
			to = parseInt(range[1]);
		}
		else {
			let offset = code.hitOffset(0, code.lineHeight * (at - 1));
			from = code.findLineBreak(offset, false);
			to = code.findLineBreak(offset, true) - 1;
		}
		code.select(from, to - from);
		this.onReveal(code);
	}
	doToggleBreakpoint(code, item) {
		let data = this.data;
		let location = code.locate(code.selectionOffset);
		let at = Math.floor(location.y / code.lineHeight) + 1;
		data.doToggleBreakpoint(data.url, at);
	}
	onCreate(code, data, dictionary) {
		super.onCreate(code, data, dictionary);
		this.document = null;
	}
	onDisplaying(code) {
		let lines = this.data.LINES;
		lines.behavior.onLineHeightChanged(lines, code.lineHeight);
		let data = this.data;
		if (Files.exists(this.data.url)) {
			let fileURL = this.data.url;
			this.notifier = new Files.DirectoryNotifier(fileURL, url => {
				this.onFileChanged(code, url)
			});
			let dirURL = fileURL.substr(0, fileURL.lastIndexOf("/") + 1);
			this.dirNotifier = new Files.DirectoryNotifier(dirURL, url => {
				let info = Files.getInfo(fileURL);
				if (this.info && (this.info.date == info.date))
					return
				this.notifier.close();
				this.notifier = new Files.DirectoryNotifier(fileURL, url => {
					this.onFileChanged(code, url)
				});
				this.onFileChanged(code, url)
			});
			this.onFileChanged(code);
		}
		else
			debugger
		let at = data.at;
		if (at !== undefined) {
			this.doSelectLine(code, at);
			data.at = undefined;
		}
		code.focus();
	}
	onFileChanged(code) {
		code.stop();
		this.onCursorCancel();
		var url = this.data.url;
		this.info = Files.getInfo(url);;
		if (url.endsWith(".js")) {
			code.type = "javascript";
			this.parsing = true;
		}
		else if (url.endsWith(".json"))
			code.type = "json";
		else if (url.endsWith(".xml"))
			code.type = "xml";
		else if (url.endsWith(".markdown"))
			code.type = "markdown";
		else
			code.type = "text";
		trace("### " + code.type + "\n");
		code.string = Files.readText(url);
		this.insertionOffset = -1;
		this.history = [];
		this.historyIndex = 0;
		var lines = this.data.LINES;
		lines.behavior.onEdited(lines, code);
		code.container.scrollTo(0, 0);
	}
	onSelected(code) {
		code.bubble("onCodeSelected");
		this.data.at = undefined;
	}
	onUndisplayed(code) {
		this.dirNotifier.close();
		this.dirNotifier = null;
		this.notifier.close();
		this.notifier = null;
	}
	onUnfocused(code) {
	}
};

class LineNumbersBehavior extends Behavior {
	onBreakpointsChanged(column) {
		let data = this.data;
		let length = column.length;
		let url = data.url;
		let machine = data.currentMachine;
		let path = Files.toPath(url);
		let content = column.first;
		while (content) {
			content.first.variant = 0;
			content.first.state = 0;
			content = content.next;
		}
		data.breakpoints.items.forEach(breakpoint => {
			if (breakpoint.url == url) {
				let at = breakpoint.line - 1;
				if ((0 <= at) && (at < length)) {
					let content = column.content(at);
					content.first.state |= 1;
				}
			}
		});
		if (machine && machine.broken) {
			let view = machine.framesView;
			view.lines.forEach(data => {
				if (data.path == path) {
					let at = data.line - 1;
					if ((0 <= at) && (at < length)) {
						let content = column.content(at);
						content.first.variant |= 1;
					}
				}
			});
		}
	}
	onComplete(column, message, json) {
		this.errors = json.errors;
		this.markers = json.markers;
		this.onBreakpointsChanged(column);
		let button = this.data.MARKERS;
		button.behavior.onMarkersChanged(button, this.markers);
	}
	onCreate(column, data) {
		column.duration = 1000; // no edits for 1 second before parsing
		this.data = data;
		this.errors = [];
		this.markers = [];
		this.hoverAt = -1;
		this.lineHeight = 0;
	}
	onEdited(column, code) {
		var former = column.length;
		var current = code.lineCount;
		if (former < current) {
			let dictionary = { height: Math.round(code.lineHeight) };
			while (former < current) {
				column.add(LineNumber(former, dictionary));
				former++;
			}
			this.onBreakpointsChanged(column);
		}
		else if (former > current) {
			column.empty(current);
		}
		if (code.behavior.parsing) {
			column.cancel();
			column.time = 0;
			column.start();
			
		}
	}
	onErrorsChanged(column) {
		this.onBreakpointsChanged(column);
	}
	onFinished(column, code) {
		let data = this.data;
		var message = new Message("xscode:parse");
		message.setRequestHeader("path", Files.toPath(data.url));
		data.CODE.writeMessage(message);
		column.invoke(message, Message.JSON);
	}
	onLineHeightChanged(column, lineHeight) {
		if (this.lineHeight != lineHeight) {
			this.lineHeight = lineHeight;
			let content = column.first;
			let height = Math.round(lineHeight);
			while (content) {
				content.height = height;
				content = content.next;
			}
		}
	}
	onMachinesChanged(column) {
		this.onBreakpointsChanged(column);
	}
	onMachineSelected(column, machine) {
		this.onBreakpointsChanged(column);
	}
	onMachineViewChanged(column, viewIndex) {
		if (viewIndex == 0)
			this.onBreakpointsChanged(column);
	}
	onMouseEntered(column, x, y) {
		this.onMouseMoved(column, x, y);
	}
	onMouseMoved(column, x, y) {
		if (this.hoverAt != y) {
			let at = Math.floor((y - column.y) / this.lineHeight);
			let data = this.data;
			let error = null;
			let reason = data.REASON;
			let content = column.content(at);
			if (content.first.state & 2) {
				let line = at + 1;
				error = this.errors.find(error => error.line == line);
				if (!error) {
					let url = data.url;
					error = this.data.errors.items.find(error => (error.url == url) && (error.line == line));
				}
			}
			if (error) {
				reason.first.string = error.reason;
				reason.last.string = error.line;
				reason.x = column.x + column.width - reason.width - 1;
				reason.y = column.y + (at * this.lineHeight) - 1;
				reason.height = this.lineHeight + 2;
				reason.visible = true;
			}
			else {
				reason.visible = false;
			}
			this.hoverAt = at;
		}
	}
	onMouseExited(column, x, y) {
		let data = this.data
		let reason = data.REASON;
		reason.visible = false;
		this.hoverAt = -1;
	}
	onTouchBegan(column, id, x, y, ticks) {
		this.data.CODE.focus();
		column.captureTouch(id, x, y, ticks);
		//var code = this.data.CODE;
		//code.behavior.onTouchBeganMode(code, id, x, y, ticks, LINE_MODE);
	}
	onTouchEnded(column, id, x, y, ticks) {
		let at = Math.floor((y - column.y) / this.lineHeight);
		model.doToggleBreakpoint(this.data.url, at + 1);
		//code.behavior.onTouchEnded(code, id, x, y, ticks);
	}
	onTouchMoved(column, id, x, y, ticks) {
		//var code = this.data.CODE;
		//code.behavior.onTouchMoved(code, id, x, y, ticks);
	}
};

class ReasonBehavior extends Behavior {

	onMouseEntered(container, x, y) {
		container.visible = true;
	}
	onMouseExited(container, x, y) {
		container.active = false;
		container.visible = false;
	}
}

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	FindField,
	findHintStyle,
	findLabelStyle,
	findModeToCaseless,
	findModeToPattern,
	PathLayout,
} from "find";

export var CodeView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: CodeViewBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:26, bottom:0,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:paneBorderSkin, }),
				Scroller($, {
					left:50, right:0, top:1, bottom:0, clip:true, active:true, 
					Behavior: class  extends CodeScrollerBehavior {
						onScrolled(scroller) {
							scroller.next.first.next.scrollTo(0, scroller.scroll.y);
						}
					},
					contents: [
						Code($, { anchor:"CODE", left:0, top:0, skin:textSkin, style:textStyle, active:true, selectable:true, Behavior:CodeEditorBehavior }),
						HorizontalScrollbar($, {}),
						VerticalScrollbar($, {}),
					],
				}),
				Container($, {
					left:0, width:60, top:1, bottom:0, clip:true,
					contents: [
						Content($, { left:0, width:50, top:0, bottom:0, skin:lineNumbersSkin, }),
						Scroller($, {
							left:0, width:50, top:0, bottom:0, active:true,
							Behavior: class extends Behavior {
								onTouchScrolled(scroller, touched, dx, dy) {
									scroller.container.previous.scrollBy(-dx, -dy);
									shell.behavior.onHover();
								}
							},
							contents: [
								Column($, {
									anchor:"LINES", left:0, right:0, top:0, active:true, 
									Behavior: LineNumbersBehavior,
								}),
							],
						}),
					],
				}),
			],
		}),
		Container($, {
			left:0, right:0, top:26, bottom:0,
			contents: [
				Line($, {
					anchor:"REASON", left:0, top:0, height:0, visible:false, skin:reasonSkin,
					contents: [
						Label($, { left:1, top:1, bottom:1, style:reasonLabelStyle }),
						Label($, { width:32, height:16, style:reasonNumberStyle }),
					],
				}),
			],
		}),
		FindLine($, { anchor:"FIND" }),
		Line($, {
			left:0, right:0, top:0, height:26, skin:paneHeaderSkin, active:true, 
			Behavior: class extends Behavior {
				onCreate(line, data) {
					this.data = data;
				}
				onTouchBegan(line) {
					this.data.CODE.focus();
				}
			},
			contents: [
				Content($, { width:8 }),
				PathLayout($, {}),
				Content($, {
					skin:buttonsSkin, variant:10, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							launchURI(this.data.url);
						}
					},
				}),
				Content($, { skin:buttonsSkin, variant:11, state:1, active:true, Behavior:ButtonBehavior, name:"doFind" }),
				Content($, { 
					anchor:"MARKERS", width:30, height:30, skin:buttonsSkin, variant:9, state:0, active:false, 
					Behavior: class extends MenuButtonBehavior {
						onDescribeMenu(button) {
							let data = this.data;
							let code = this.data.CODE;
							let bounds = code.selectionBounds;
							let line = Math.floor(bounds.y / code.lineHeight) + 1;
							let markers = this.markers;
							return {
								ItemTemplate:MarkerItemLine,
								items:markers,
								horizontal:"right",
								selection:markers.findIndex(marker => marker.line == line),
								context: shell,
							};
						}
						onMarkersChanged(button, markers) {
							this.markers = markers;
							button.active = markers.length > 0;
						}
						onMenuSelected(button, selection) {
							let code = this.data.CODE;
							code.behavior.doSelectLine(code, selection.line);
						}
					},
				}),
				Content($, { skin:buttonsSkin, variant:6, state:1, active:true, Behavior:ButtonBehavior, name:"doCloseFile" }),
			],
		}),
	],
}));

var MarkerItemLine = Line.template($ => ({
	left:0, right:0, height:20, skin:menuLineSkin, active:true, Behavior:MenuItemBehavior,
	contents: [
		Content($, { width:20, height:20, skin:bulletSkin, visible:false }),
		Label($, { left:0, height:20, style:findLabelStyle, string:$.title }),
	],
}));

var LineNumber = Container.template($ => ({
	left:0, right:0, height:16,
	contents: [
		Label($, { left:0, right:-8, height:16, skin:lineNumberSkin, style:lineNumberStyle, string:++$ }),
	],
}));

var FindLine = Line.template(function($) { return {
	left:0, right:0, top:26, height:27, skin: paneHeaderSkin, visible:false,
	contents: [
		Line($, {
			left:0, right:0, top:-3, height:30,
			contents: [
				Content($, { width:4 }),
				FindField($, {}),
				Content($, {
					width:40, skin:buttonsSkin, variant:7, active:false,
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
					width:40, skin:buttonsSkin, variant:8, active:false,
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
					width:60, skin:buttonSkin, active:true,
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("onFindDone");
						}
					},
					contents: [
						Label($, { left:0, right:0, style:buttonStyle, string:"Done" }),
					],
				}),
			],
		}),
	],
}});

export var ErrorView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, {
			left:0, right:0, top:26, bottom:0, skin:paneBodySkin,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:paneBorderSkin, }),
				Column($, {
					contents: [
						Label($, { state:1, style:errorStyle, string:"File not found!" }),
						Container($, {
							anchor:"BUTTON", width:80, skin:buttonSkin, active:true, name:"onEnter",
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
												model.doCloseURL(this.data.url);
												model.doOpenURL(locateURL, line);
												var mappings = model.mappings;
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
								Label($, { left:0, right:0, style:buttonStyle, string:"Locate..." }),
							],
						}),
					]
				}),
			],
		}),
		Line($, {
			left:0, right:0, top:0, height:26, skin:paneHeaderSkin, 
			contents: [
				Content($, { width:8 }),
				Label($, { left:0, right:0, style: paneHeaderStyle, string:Files.toPath($.url) }),
				Content($, { skin:buttonsSkin, variant:6, state:1, active:true, Behavior:ButtonBehavior, name:"doCloseFile" }),
			],
		}),
	],
}));

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
			if (to == 1) {
				this.bodyTo = body.y + findLine.height;
				this.findLine = findLine
				this.findFrom = body.y - findLine.height;
				this.findTo = body.y;
			}
		}
		else if (from == 1) {
			if (to == 0) {
				this.bodyTo = findLine.y;
				this.findLine = findLine
				this.findFrom = findLine.y;
				this.findTo = findLine.y - findLine.height;
			}
		}
		if (this.findLine)
			this.findLine.visible = true;;
		body.coordinates = { left:0, right:0, top:this.bodyFrom, height:container.y + container.height - this.bodyFrom }
		findLine.first.state = to > 1 ? 3 : 1;	
	}
	onEnd(container, body, findLine, replaceLine, to) {
		findLine.visible = to > 0;
		body.coordinates = { left:0, right:0, top:body.y - container.y, bottom:0 };
		if (to == 0)
			this.container.behavior.data.CODE.focus();
		else
			this.container.behavior.data.FIND_FOCUS.focus();
	}
	onStep(fraction) {
		fraction = Math.quadEaseOut(fraction);
		if (this.findLine)
			this.findLine.y = this.findFrom + ((this.findTo - this.findFrom) * fraction);
		this.body.y = this.bodyFrom + ((this.bodyTo - this.bodyFrom) * fraction);
		this.body.height = this.container.y + this.container.height - this.body.y;
	}
};


