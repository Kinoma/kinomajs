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
	PASTEL_CYAN,
	CYAN,
	DARK_CYAN,
	DARKER_CYAN,
	FIXED_FONT,
	PASTEL_GREEN,
	GREEN,
	DARK_GREEN,
	DARKER_GREEN,
	PASTEL_GRAY,
	GRAY,
	DARK_GRAY,
	DARKER_GRAY,
	PASTEL_RED,
	RED,
	DARK_RED,
	DARKER_RED,
	WHITE,
	buttonsSkin,
	glyphsSkin,
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
} from "assets";

var textSkin = new Skin({ fill:["transparent", "transparent", "#e0e0e0", "#cbe1fa"] })
var textStyle = new Style({ 
	font:FIXED_FONT,
	size:12, 
	horizontal:"left",
	left:8, right:8,
	color: [ "black", "#103ffb", "#b22821", "#008d32" ]
})

const CONSOLE_BREAKPOINT_KIND = 0;
const CONSOLE_DEBUGGER_KIND = 1;
const CONSOLE_ERROR_KIND = 2;
const CONSOLE_STEP_KIND = 4;
const CONSOLE_THROW_KIND = 1;
const CONSOLE_WARNING_KIND = 3;

var consoleLineNameNumberSkins = [
	new Skin({ fill:[GREEN, GREEN, DARK_GREEN, DARKER_GREEN]} ),
	new Skin({ fill:[CYAN, CYAN, DARK_CYAN, DARKER_CYAN]} ),
	new Skin({ fill:[RED, RED, DARK_RED, DARKER_RED]} ),
	new Skin({ fill:[GRAY, GRAY, DARK_GRAY, DARKER_GRAY]} ),
];
var consoleLineReasonSkins = [
	new Skin({ stroke:PASTEL_GREEN, borders: { right:1, top:1, bottom:1 }, fill:[WHITE, WHITE, PASTEL_GREEN, PASTEL_GREEN] }),
	new Skin({ stroke:PASTEL_CYAN, borders: { right:1, top:1, bottom:1 }, fill:[WHITE, WHITE, PASTEL_CYAN, PASTEL_CYAN] }),
	new Skin({ stroke:PASTEL_RED, borders: { right:1, top:1, bottom:1 }, fill:[WHITE, WHITE, PASTEL_RED, PASTEL_RED] }),
	new Skin({ stroke:PASTEL_GRAY, borders: { right:1, top:1, bottom:1 }, fill:[WHITE, WHITE, PASTEL_GRAY, PASTEL_GRAY] }),
];
var consoleLineNameStyle = new Style({ font:FIXED_FONT, size:12, color:"white", left:4 });
var consoleLineNumberStyle = new Style({ font:FIXED_FONT, size:12, color:"white", right:4 });
var consoleLineReasonStyles = [
	new Style({ font:FIXED_FONT, size:12, color:DARKER_GREEN, left:4, right:4 }),
	new Style({ font:FIXED_FONT, size:12, color:DARKER_CYAN, left:4, right:4 }),
	new Style({ font:FIXED_FONT, size:12, color:DARKER_RED, left:4, right:4 }),
	new Style({ font:FIXED_FONT, size:12, color:DARKER_GRAY, left:4, right:4 }),
];

const whiteSkin =new Skin({ fill:WHITE} ),


import {
	model
} from "main";

import { 
	ButtonBehavior, 
} from "common/control";

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import { 
	CodeBehavior, 
	LineBehavior,
} from "behaviors";

class ConsolePaneBehavior extends Behavior {
	doClearConsole(container) {
		let scroller = container.first.first;
		let code = scroller.first;
		let column = scroller.next.first;
		code.behavior.doClear(code);
		column.empty();
		this.cr = false;
		this.output = "";
	}
	doLog(container, text) {
		this.output += text;
		var lines = this.output.split("\n");
		var c = lines.length - 1;
		for (var i = 0; i < c; i++)
			this.doLogLine(container, lines[i]);
		this.output = lines[i];
	}
	doLogLine(container, text) {
		let scroller = container.first.first;
		let code = scroller.first;
		let column = scroller.next.first;
		let height = code.height - code.lineHeight - ((22 - code.lineHeight) >> 1);
		
		text = text.toString();
		var result = this.splitError(text);
		var kind = -1, reason;
		if (result) {
			kind = result.kind;
			reason = result.reason;
			if ((kind == "error") || (kind == "# Error") || (kind == "# Exception"))
				kind = CONSOLE_ERROR_KIND;
			else if ((kind == "warning") || (kind == "# Warning"))
				kind = CONSOLE_WARNING_KIND;
			else if ((kind == "# Break") && (reason == "breakpoint!")) 
				kind = CONSOLE_BREAKPOINT_KIND;
			else if ((kind == "# Break") && (reason == "debugger!")) 
				kind = CONSOLE_DEBUGGER_KIND;
			else if ((kind == "# Break") && (reason == "step!")) 
				kind = CONSOLE_STEP_KIND;
			else if ((kind == "# Break") && (reason == "throw!")) 
				kind = CONSOLE_THROW_KIND;
			else if (kind == "# Break")
				kind = CONSOLE_ERROR_KIND;
			else
				kind = -1;
		}
		if (kind >= 0) {
			let url = Files.toURI(result.path);
			let parts = parseURI(url);
			let name = parts.name;
			let line = parseInt(result.line);
			let data = {kind, url, name, line, reason, lineHeight:code.lineHeight};
			if (kind != CONSOLE_STEP_KIND) {
				column.add(new ConsoleLine(data));
				if (code.length)
					code.behavior.doLog(code, "\n");
				code.behavior.doLog(code, `${ name } (${ line }): ${ reason }`);
			}
			if (kind == CONSOLE_ERROR_KIND)
				model.addError(url, line, reason);
		}
		else {
			column.add(new Content({height: code.lineHeight}));
			if (code.length)
				code.behavior.doLog(code, "\n");
			code.behavior.doLog(code, text);
		}
		scroller.scrollTo(0, 0x7FFFFFFF);
	}
	doLogRaw(container, text) {
		let scroller = container.first.first;
		let code = scroller.first;
		let column = scroller.next.first;
		code.behavior.doLog(code, text);
		let height = code.height - column.height;
		if (height)
			column.add(new Content({height}));
		scroller.scrollTo(0, 0x7FFFFFFF);
	}
	onCreate(container, data) {
		this.data = data;
		this.pathLineRegExp = /([^:]+):([0-9]+): ([^:]+): (.+)/;
		this.output = "";
	}
	onMachineDeselected(container, machine) {
		let scroller = container.first.first;
		if (machine)
			machine.consoleScroll = scroller.scroll;
		else
			model.consoleScroll = scroller.scroll;
	}
	onMachineSelected(container, machine) {
		this.doClearConsole(container);
		let scroller = container.first.first;
		if (machine) {
			machine.logView.lines.forEach(line => this.doLog(container, line));
			scroller.scroll = machine.consoleScroll;
		}
		else {
			model.consoleLines.forEach(line => this.doLog(container, line));
			scroller.scroll = model.consoleScroll;
		}
			
	}
	splitError(error) @ "ConsolePaneBehavior_prototype_splitError";
};

class ConsoleCodeBehavior extends CodeBehavior {
	doClear(code) {
		code.select(0, code.length);
		code.insert("");
	}
	doLog(code, text) {
		code.select(code.length, 0);
		code.insert(text);
		code.container.reveal(code.selectionBounds);
	}
};

class ConsoleColumnBehavior extends CodeBehavior {
};

class ConsoleHeaderBehavior extends LineBehavior {
	changeArrowState(line, state) {
		line.first.state = state;
	}
	onDisplaying(line) {
		let divider = line.container.container.container.next;
		this.changeArrowState(line, (divider.behavior.status) ? 3 : 1);
	}
	onDividerChanged(line, divider) {
		if (divider == line.container.container.container.next)
			this.changeArrowState(line, (divider.behavior.status) ? 3 : 1);
	}
	onMouseEntered(line, x, y) {
		line.state = 1;
		let content = line.last;
		content.visible = true;
	}
	onMouseExited(line, x, y) {
		line.state = 0;
		let content = line.last;
		content.visible = false;
	}
	onTouchBegan(line) {
		super.onTouchBegan(line);
		this.changeArrowState(line, 2);
	}
	onTap(line) {
		shell.delegate("doToggleConsole");
	}
};

class ConsoleLineBehavior extends ButtonBehavior {
	onDisplaying(line, data) {
		this.onTap(line);
	}
	onMouseEntered(container, x, y) {
		super.onMouseEntered(container, x, y);
		return true;
	}
	onMouseExited(container, x, y) {
		super.onMouseExited(container, x, y);
		return true;
	}
	onTap(line) {
		var data = this.data;
		model.doOpenURL(data.url, data.line);
	}
};

export var ConsolePane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	Behavior: ConsolePaneBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:0, bottom:0,
			contents: [
				Scroller($, {
					left:0, right:0, top:27, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior,
					Behavior: class  extends ScrollerBehavior {
						onScrolled(scroller) {
							scroller.next.scrollTo(0, scroller.scroll.y);
						}
					},
					contents: [
						Code($, { anchor:"CONSOLE_CODE", left:0, top:0, skin:textSkin, style:textStyle, active:true, selectable:true, Behavior:ConsoleCodeBehavior }),
						HorizontalScrollbar($, {}),
						VerticalScrollbar($, {}),
					],
				}),
				Scroller($, {
					left:0, right:0, top:27, bottom:0, clip:true,
					contents: [
						Column($, { left:0, right:0, top:0, Behavior:ConsoleColumnBehavior, }),
					],
				}),
				Content($, { left:0, right:0, top:26, height:1, skin:paneBorderSkin, }),
				Line($, {
					left:0, right:0, top:0, height:26, skin:paneHeaderSkin, active:true, Behavior:ConsoleHeaderBehavior,
					contents: [
						Content($, { width:30, height:26, skin:glyphsSkin, variant:0 }),
						Label($, { 
							left:0, right:0, style:paneHeaderStyle, string:"LOG",
							Behavior: class extends ButtonBehavior {
								onMachineSelected(label, machine) {
									if (machine)
										label.string = "CONSOLE";
									else
										label.string = "LOG";
								}
							},
						}),
						Content($, {
							skin:buttonsSkin, variant:5, active:true, visible:false, 
							Behavior: class extends ButtonBehavior {
								onTap(button) {
									button.bubble("doClearConsole");
								}
							},
						}),
					],
				}),
			],
		}),
	],
}));

var ConsoleLine = Line.template(function($) { return {
	left:0, right:0, height:$.lineHeight, active:true,
	Behavior: ConsoleLineBehavior,
	contents: [
		Content($, { width:6 }),
		Label($, { height:18, skin:consoleLineNameNumberSkins[$.kind], style:consoleLineNameStyle, string:$.name }),
		Label($, { height:18, skin:consoleLineNameNumberSkins[$.kind], style:consoleLineNumberStyle, string:" (" + $.line + ")" }),
		Label($, { height:18, skin:consoleLineReasonSkins[$.kind], style:consoleLineReasonStyles[$.kind], string:$.reason }),
		Content($, { left:0, right:0, height:18 }),
	],
}});
