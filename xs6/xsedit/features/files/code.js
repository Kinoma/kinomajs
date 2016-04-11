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
	Document, 
} from "files";

// ASSETS

import {
	bulletSkin,
	menuLineSkin,
} from "common/menu";

import {
	BLACK,
	BOLD_FONT,
	DARKER_GRAY,
	DARKER_RED,
	GRAY,
	PASTEL_CYAN,
	PASTEL_ORANGE,
	PASTEL_GRAY,
	PASTEL_GREEN,
	PASTEL_YELLOW,
	RED,
	WHITE,
	blackButtonSkin,
	blackButtonStyle,
	blackButtonsSkin,
	grayBodySkin,
	grayBorderSkin,
	grayHeaderSkin,
	tableHeaderStyle,
	whiteButtonSkin,
	whiteButtonStyle,
	whiteButtonsSkin,
	backgroundSkin,
	fileGlyphsSkin,
	fieldLabelSkin,
	fieldScrollerSkin,
} from "shell/assets";	

var reasonSkin = new Skin({ fill:RED }),
var reasonLabelStyle = new Style({ font:BOLD_FONT, size:12, color:WHITE, left:4, right:4 }),
var reasonNumberStyle = new Style({ 
	font:"bold Menlo", 
	size:10, 
	horizontal:"right",
	vertical:"bottom",
	right:6, 
	bottom:1,
	color:WHITE,
}),

var textSkin = new Skin({ fill:[PASTEL_YELLOW, "transparent", "#e0e0e0", PASTEL_CYAN] })
var textStyle = new Style({ 
	font:"Menlo", 
	size:12, 
	horizontal:"left",
	left:8, right:8,
	color: [ "black", "#103ffb", "#b22821", "#008d32" ]
})
var lineNumbersSkin = new Skin({ fill:PASTEL_GRAY })
var lineNumberTexture = new Texture("assets/flags.png", 2);
var lineNumberSkin = new Skin({ 
	texture:lineNumberTexture, 
	x:0, y:0, width:40, height:16, 
	tiles: { left:20, right: 12 }, 
	states:16,
	variants:40,
});
var lineNumberStyle = new Style({ 
	font:"bold Menlo", 
	size:10, 
	horizontal:"right",
	vertical:"bottom",
	right:15,
	bottom:1,
	color: [ DARKER_GRAY, WHITE, WHITE, WHITE ],
})

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
	canReplace(container) {
		return true;
	}
	canReplaceAll(container) {
		return this.resultCount > 0;
	}
	canReplaceNext(container) {
		return this.resultCount > 0;
	}
	canReplacePrevious(container) {
		return this.resultCount > 0;
	}
	canReplaceSelection(container) {
		return this.resultCount > 0;
	}
	doFind(container) {
		var data = this.data;
		var code = this.data.CODE;
		var findLine = data.FIND;
		var replaceLine = data.REPLACE;
		code.behavior.find(code, data.findString, data.findMode);
		data.FIND_FOCUS.focus();
		if (!findLine.visible || replaceLine.visible)
			container.run(new FindTransition, container.first, findLine, replaceLine, 1);
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
	doReplace(container) {
		var data = this.data;
		var code = data.CODE;
		var findLine = data.FIND;
		var replaceLine = data.REPLACE;
		code.behavior.find(code, data.findString, data.findMode);
		data.FIND_FOCUS.focus();
		if (!replaceLine.visible)
			container.run(new FindTransition, container.first, findLine, replaceLine, 2);
	}
	doReplaceAll(container) {
		var data = this.data;
		var code = data.CODE;
		code.behavior.replaceAll(code, data.replaceString);
	}
	doReplaceNext(container) {
		var data = this.data;
		var code = data.CODE;
		code.behavior.replace(code, data.replaceString, 1);
	}
	doReplacePrevious(container) {
		var data = this.data;
		var code = data.CODE;
		code.behavior.replace(code, data.replaceString, -1);
	}
	doReplaceSelection(container) {
		var data = this.data;
		var code = data.CODE;
		code.behavior.replace(code, data.replaceString, 0);
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
		var replaceLine = data.REPLACE;
		code.behavior.find(code, "", 0);
		if (findLine.visible)
			container.run(new FindTransition, container.first, findLine, replaceLine, 0);
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
				data.REPLACE_FOCUS.focus();
			else
				data.CODE.focus();
			return true;
		}
		if (c == 25) {
			var data = this.data;
			if (data.REPLACE_FOCUS.focused)
				data.FIND_FOCUS.focus();
			else
				data.CODE.focus();
			return true;
		}
		if (c == 27) {
			this.onFindDone(container);
			return true;
		}
		return false;
	}
	onReplaceEdited(container) {
	}
};

class CodeEditorBehavior extends CodeBehavior {
	canRevert(code) {
		return this.document && this.document.dirty;
	}
	canSave(code) {
		return this.document && this.document.dirty;
	}
	canToggleBreakpoint(code, item) {
		let location = code.locate(code.selectionOffset);
		var lines = this.data.LINES;
		var line = lines.content(Math.floor(location.y / code.lineHeight));
		item.title = item.titles[line.first.variant & 1];
		return true;
	}
	find(code, findString, findMode) {
		code.find(findModeToPattern(findMode, findString), findModeToCaseless(findMode));
		this.onSelected(code);
		this.onReveal(code);
	}
	replaceAll(code, replacement) {
		code.focus();
		this.insertionOffset = -1;
		this.onChanging(code);
		code.replaceAll(replacement);
		this.onChanged(code);
		this.onReveal(code);
	}
	replace(code, replacement, direction) {
		code.focus();
		this.insertionOffset = -1;
		this.onChanging(code);
		code.replace(replacement);
		code.findAgain(direction);
		this.onChanged(code);
		this.onReveal(code);
	}
	doRevert(code) {
		this.onFileChanged(code);
		this.document.dirty = false;
		shell.distribute("onDocumentChanged", this.document);
	}
	doSave(code) {
		this.document.save();
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
		data.debugFeature.doToggleBreakpoint(data.url, at);
	}
	onCreate(code, data, dictionary) {
		super.onCreate(code, data, dictionary);
		this.document = null;
	}
	onDisplaying(code) {
		let lines = this.data.LINES;
		lines.behavior.onLineHeightChanged(lines, code.lineHeight);
		let data = this.data;
		let document = this.document = data.filesFeature.documents.items.find(item => item.url == data.url);
		if (document) {
			let state = document.state;
			if (state) {
				document.state = null;
				code.stop();
				this.onCursorCancel();
			
				code.type = state.type;
				code.string = state.string;
				code.select(state.selectionOffset, state.selectionLength);
			  
				this.insertionOffset = state.insertionOffset;
				this.history = state.history;
				this.historyIndex = state.historyIndex;
				this.parsing = state.parsing;
			
				lines.behavior.onEdited(lines, code);
			
				code.container.scrollTo(state.scroll.x, state.scroll.y);
			}
			else
				this.onFileChanged(code);
			document.onCodeBegan(code);
		}
		else if (Files.exists(this.data.url)) {
			this.notifier = new Files.DirectoryNotifier(this.data.url, url => {
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
	onEdited(code) {
		super.onEdited(code);
		let data = this.data;
		let document = this.document;
		let notifier = this.notifier;
		if (!document) {
			if (notifier) {
				notifier.close();
				this.notifier = null;
			}
			document = this.document = new Document(data.url);
			document.onCodeBegan(code);
			data.filesFeature.addDocument(document);
		}
		if (!document.dirty) {
			document.dirty = true;
			shell.distribute("onDocumentChanged", document);
		}
		var lines = this.data.LINES;
		lines.behavior.onEdited(lines, code);
	}
	onFileChanged(code) {
		code.stop();
		this.onCursorCancel();
		var url = this.data.url;
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
		let data = this.data;
		let document = this.document;
		let notifier = this.notifier;
		if (document) {
			document.CODE = null;
			document.state = {
				insertionOffset: this.insertionOffset,
				history: this.history,
				historyIndex: this.historyIndex,
				parsing: this.parsing,
				selectionOffset: code.selectionOffset,
				selectionLength: code.selectionLength,
				scroll: code.container.scroll,
				string: code.string,
				type: code.type,
			}
		}
		else if (notifier)
			notifier.close();
	}
	onUnfocused(code) {
	}
};

class LineNumbersBehavior extends Behavior {
	onBreakpointsChanged(column) {
		let data = this.data;
		let length = column.length;
		let url = data.url;
		let machine = data.debugFeature.currentMachine;
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
					content.first.variant |= 1;
				}
			}
		});
		data.errors.items.forEach(error => {
			if (error.url == url) {
				let at = error.line - 1;
				if ((0 <= at) && (at < length)) {
					let content = column.content(at);
					content.first.state |= 2;
				}
			}
		});
		this.errors.forEach(error => {
			let at = error.line - 1;
			if ((0 <= at) && (at < length)) {
				let content = column.content(at);
				content.first.state |= 2;
			}
		});
		if (machine && machine.broken) {
			let view = machine.framesView;
			view.lines.forEach(data => {
				if (data.path == path) {
					let at = data.line - 1;
					if ((0 <= at) && (at < length)) {
						let content = column.content(at);
						content.first.state |= 1;
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
		shell.behavior.debugFeature.doToggleBreakpoint(this.data.url, at + 1);
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
} from "find";

export var CodeView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	Behavior: CodeViewBehavior,
	contents: [
		Container($, {
			left:0, right:10, top:30, bottom:10,
			contents: [
				Scroller($, {
					left:50, right:0, top:0, bottom:0, clip:true, active:true, 
					Behavior: class  extends CodeScrollerBehavior {
						onScrolled(scroller) {
							scroller.next.first.next.scrollTo(0, scroller.scroll.y);
						}
					},
					contents: [
						Code($, { anchor:"CODE", left:0, top:0, skin:textSkin, style:textStyle, active:true, editable:true, Behavior:CodeEditorBehavior }),
						HorizontalScrollbar($, { bottom:-10 }),
						VerticalScrollbar($, { right:-10 }),
					],
				}),
				Container($, {
					left:0, width:60, top:0, bottom:0, clip:true,
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
			left:0, right:10, top:30, bottom:10, skin:grayBorderSkin,
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
		ReplaceLine($, { anchor:"REPLACE" }),
		FindLine($, { anchor:"FIND" }),
		Line($, {
			left:0, right:10, top:0, height:30, skin:grayHeaderSkin, active:true, 
			Behavior: class extends Behavior {
				onCreate(line, data) {
					this.data = data;;
				}
				onDocumentChanged(line, document) {
					if (this.data.url == document.url)
						line.first.visible = document.dirty;
				}
				onTouchBegan(line) {
					this.data.CODE.focus();
				}
			},
			contents: [
				Content($, { width:30, height:30, skin:fileGlyphsSkin, variant:5, visible:false }),
				Label($, { left:0, right:0, style:tableHeaderStyle, string:Files.toPath($.url) }),
				Content($, { 
					anchor:"MARKERS", width:30, height:30, skin:whiteButtonsSkin, variant:7, state:0, active:false, 
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
	left:0, right:10, top:30, height:30, skin: grayHeaderSkin, visible:false,
	contents: [
		Content($, { width:30, height:30, skin:fileGlyphsSkin, state:1, variant:1, active:true, 
			Behavior: class extends Behavior {
				onCreate(content, data) {
					this.data = data;
				}
				onTouchBegan(content) {
					content.state = 2;
				}
				onTouchEnded(content) {
					var container = content.container.container;
					var data = this.data;
					var findLine = data.FIND;
					var replaceLine = data.REPLACE;
					if (replaceLine.visible) {
						container.run(new FindTransition, container.first, findLine, replaceLine, 1);
					}
					else {
						container.run(new FindTransition, container.first, findLine, replaceLine, 2);
					}
				}
			}
		}),
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

var ReplaceLine = Line.template(function($) { return {
	left:0, right:10, top:60, height:26, skin:grayHeaderSkin, visible:false,
	contents: [
		Content($, { width:30 }),
		Scroller($, {
			left:0, right:0, top:0, bottom:4,
			skin: fieldScrollerSkin,
			clip:true,
			active:true,
			Behavior: FieldScrollerBehavior,
			contents: [
				Code($, {
					anchor:"REPLACE_FOCUS",
					left: 0, top:2, bottom:2,
					skin:fieldLabelSkin,
					style:findLabelStyle,
					string: $.replaceString,
					active:true,
					editable: true,
					field: true,
					Behavior: class extends CodeBehavior {
						onEdited(label) {
							var data = this.data;
							data.replaceString = label.string;
							label.next.visible = data.replaceString.length == 0;
							label.bubble("onReplaceEdited");
						}
					},
				}),
				Label($, {
					left: 0, top:0, bottom:0,
					style:findHintStyle,
					string:"Replace String",
					visible:$.replaceString ? false : true,
				}),
			],
		}),
		Container($, {
			width:80, top:-4, skin:whiteButtonSkin, active:false,
			Behavior: class extends ButtonBehavior {
				onFound(button, resultCount) {
					button.active = resultCount > 0;
				}
				onTap(button) {
					button.bubble("doReplaceSelection");
				}
			},
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Replace" }),
			],
		}),
		Container($, {
			width:60, top:-4, skin:whiteButtonSkin, active:false,
			Behavior: class extends ButtonBehavior {
				onFound(button, resultCount) {
					button.active = resultCount > 0;
				}
				onTap(button) {
					button.bubble("doReplaceAll");
				}
			},
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"All" }),
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
		if (replaceLine.visible) from++;
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
				this.replaceLine = undefined;
			}
			else if (to == 2) {
				replaceLine.visible = true;
				this.bodyTo = body.y + findLine.height + replaceLine.height;
				this.findLine = findLine
				this.findFrom = body.y - findLine.height - replaceLine.height;
				this.findTo = body.y;
				this.replaceLine = replaceLine
				this.replaceFrom = body.y - replaceLine.height;
				this.replaceTo = body.y + findLine.height;
			}
		}
		else if (from == 1) {
			if (to == 0) {
				this.bodyTo = findLine.y;
				this.findLine = findLine
				this.findFrom = findLine.y;
				this.findTo = findLine.y - findLine.height;
				this.replaceLine = undefined;
			}
			else if (to == 2) {
				replaceLine.visible = true;
				this.bodyTo = body.y + replaceLine.height;
				this.findLine = undefined;
				this.replaceLine = replaceLine
				this.replaceFrom = body.y - replaceLine.height;
				this.replaceTo = body.y;
			}
		}
		else {
			if (to == 0) {
				this.bodyTo = findLine.y;
				this.findLine = findLine
				this.findFrom = findLine.y;
				this.findTo = findLine.y - findLine.height - replaceLine.height;
				this.replaceLine = replaceLine
				this.replaceFrom = replaceLine.y;
				this.replaceTo = findLine.y - replaceLine.height;
			}
			else if (to == 1) {
				this.bodyTo = replaceLine.y;
				this.findLine = undefined;
				this.replaceLine = replaceLine
				this.replaceFrom = replaceLine.y;
				this.replaceTo = replaceLine.y - replaceLine.height;
			}
		}
		if (this.findLine)
			this.findLine.visible = true;;
		if (this.replaceLine)
			this.replaceLine.visible = true;;
		body.coordinates = { left:0, right:10, top:this.bodyFrom, height:container.y + container.height - 10 - this.bodyFrom }
		findLine.first.state = to > 1 ? 3 : 1;	
	}
	onEnd(container, body, findLine, replaceLine, to) {
		findLine.visible = to > 0;
		replaceLine.visible = to > 1;
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
		if (this.replaceLine)
			this.replaceLine.y = this.replaceFrom + ((this.replaceTo - this.replaceFrom) * fraction);
		this.body.y = this.bodyFrom + ((this.bodyTo - this.bodyFrom) * fraction);
		this.body.height = this.container.y + this.container.height - 10 - this.body.y;
	}
};

