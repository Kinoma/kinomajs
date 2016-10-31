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
	mxFramesView,
	mxLocalsView,
	mxGlobalsView,
	mxFilesView,
	mxBreakpointsView,
	mxGrammarsView,
	mxFileView,
	mxLogView,
} from "shell/main";

// ASSETS

import {
	BLACK,
	GRAYS,
	CYAN,
	ORANGE,
	WHITE,
	
	LIGHT_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BOLD_FONT,
	
	buttonSkin,
	buttonStyle,
	buttonsSkin,
	glyphsSkin,
	waitSkin,
	waitStyle,
	
	headerHeight,
	lineHeight,
	lineIndent,
} from "shell/assets";


var paneBackgroundSkin = new Skin({ fill:GRAYS[6] });
var paneSeparatorSkin = new Skin({ fill:GRAYS[14] });
var tableHeaderSkin = new Skin({ fill:[GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]], stroke:GRAYS[14], borders: { bottom:1 } });
var tableFooterSkin = new Skin({ fill:GRAYS[2], stroke:GRAYS[14], borders: { bottom:1 }  });
var tableLineSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], GRAYS[2]]  });

var tableHeaderStyle = new Style({ font:BOLD_FONT, size:12, color:GRAYS[75], horizontal:"left" });
var tableLineStyle = new Style({ font:NORMAL_FONT, size:12, color:[BLACK, BLACK, BLACK, WHITE], horizontal:"left" });
var debugNameStyle = new Style({ font:NORMAL_FONT, size:12, color:[BLACK, BLACK, BLACK, WHITE] });
var debugValueStyle = new Style({ font:LIGHT_FONT, size:12, color:[BLACK, BLACK, BLACK, WHITE] });

var callLineSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], ORANGE]  });
var callLineStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"left" });

var fileLineSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], CYAN]  });
var fileLineStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"left" });

// BEHAVIORS

import { 
	ButtonBehavior, 
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
	SpinnerBehavior,
} from "shell/behaviors";

class DebugPaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onMachineChanged(container, machine) {
		if (this.machine == machine) {
			let spinner = container.last;
			let scroller = container.first;
			let column = scroller.first;
			let data = this.data;
			spinner.empty(0);
			column.empty(0);
			if (machine.broken) {
				column.add(CallTable(data, {}));
				column.add(DebugTable(data, { Behavior:LocalsTableBehavior }));
				column.add(DebugTable(data, { Behavior:ModulesTableBehavior }));
				column.add(DebugTable(data, { Behavior:GlobalsTableBehavior }));
			}
			else {
				spinner.add(RunningContent(data, {}));
			}
		}
	}
	onMachineDeselected(container, machine) {
		if (machine)
			machine.debugScroll = container.first.scroll;
	}
	onMachineSelected(container, machine) {
		this.machine = machine;
		if (machine) {
			this.onMachineChanged(container, machine);
			container.first.scroll = machine.debugScroll;
		}
	}
};

class DebugButtonBehavior extends ButtonBehavior {
	onCreate(container, data) {
		super.onCreate(container, data);
		this.can = "can" + container.name;
		this.do = "do" + container.name;
	}
	onMachineChanged(container, machine) {
		container.active = this.data[this.can]();
	}
	onMachineSelected(container, machine) {
		container.active = this.data[this.can]();
	}
	onTap(container) {
		this.data[this.do]();
	}
};

class DebugTableBehavior extends TableBehavior {
	addLines(column) {
		let header = column.first;
		let view = this.view;
		column.empty(1);
		if (view && view.expanded) {
			header.behavior.expand(header, true);
			view.lines.forEach(data => column.add(new (this.lineTemplate)(data)));
			if (view.lineIndex >= 0) {
				let line = column.content(view.lineIndex + 1);
				line.behavior.select(line, true);
			}
			column.add(new this.footerTemplate(data));
		}
		else
			header.behavior.expand(header, false);
		model.onHover(shell);
	}
	hold(column) {
		let header = column.first;
		let result = DebugHeader(this.data, {left:0, right:0, top:0, height:header.height, skin:header.skin});
		let view = this.view;
		result.behavior.expand(result, view && view.expanded);
		result.last.string = header.last.string;
		return result;
	}
	onCreate(column, data) {
		let machine = data.currentMachine;
		let view = machine ? machine.views[this.viewIndex] : null;
		this.data = data;
		this.machine = machine;
		this.view = view;	
		this.addLines(column);
	}
	onMachineSelected(column, machine) {
		this.machine = machine;
		if (machine) {
			this.view = machine.views[this.viewIndex];
			this.addLines(column);
			column.visible = true;
		}
		else {
			this.view = null;
			column.empty(1);
			column.visible = false;
		}
	}
	onMachineViewChanged(column, viewIndex) {
		if (this.viewIndex == viewIndex)
			this.addLines(column);
	}
	toggle(column) {
		var view = this.view;
		if (view)
			view.expanded = !view.expanded;
		this.addLines(column);
	}
	trigger(column, line) {
	}
};

class DebugHeaderBehavior extends HeaderBehavior {
};

class DebugLineBehavior extends LineBehavior {
	onTap(line) {
		let behavior = line.container.behavior;
		let data = this.data;
		behavior.data.doDebugToggle(behavior.viewIndex, data.name, data.value);
	}
};

class CallTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = CallFooter;
		this.lineTemplate = CallLine;
		this.viewIndex = mxFramesView;
		super.onCreate(column, data);
		column.HEADER.last.string = "CALLS";
	}
	trigger(column, line) {
		this.view.lineIndex = line.index - 1;
		let content = column.first.next;
		while (content) {
			if (content.behavior)
				content.behavior.select(content, content == line);
			content = content.next;
		}
	}
};

class CallHeaderBehavior extends HeaderBehavior {
};

class CallLineBehavior extends LineBehavior {
	onTap(line) {
		let column = line.container;
		let behavior = column.behavior;
		let data = this.data;
		behavior.machine.framesView.lineIndex = line.index;
		behavior.data.doDebugFile(behavior.viewIndex, data.path, data.line, data.value);
		behavior.trigger(column, line);
	}
	select(line, selectIt) {
		if (selectIt) {
			line.last.style = callLineStyle; 
			this.flags |= 4;
		}
		else {
			line.last.style = tableLineStyle;
			this.flags &= ~4;
		}
		this.changeState(line);
	}
};

class LocalsTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugLine;
		this.viewIndex = mxLocalsView;
		super.onCreate(column, data);
		column.HEADER.last.string = "LOCALS";
	}
};

class ModulesTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugLine;
		this.viewIndex = mxGrammarsView;
		super.onCreate(column, data);
		column.HEADER.last.string = "MODULES";
	}
};

class GlobalsTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugLine;
		this.viewIndex = mxGlobalsView;
		super.onCreate(column, data);
		column.HEADER.last.string = "GLOBALS";
	}
};

// TEMPLATES

export var DebugPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:paneBackgroundSkin, 
	Behavior: DebugPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:27, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:0, right:1, top:0, clip:true, Behavior:HolderColumnBehavior, 
					contents: [
					]
				}),
				Container($, {
					left:0, right:0, top:0, height:26, clip:true, Behavior:HolderContainerBehavior,
				}),
				VerticalScrollbar($, {}),
			]
		}),
		Content($, { left:0, right:0, top:26, height:1, skin:paneSeparatorSkin, }),
		DebugToolsHeader(model, { }),
		Container($, { left:0, right:0, top:27, bottom:0 }),
	]
}));

var DebugToolsHeader = Line.template($ => ({
	left:4, top:0, height:26,
	contents: [
		DebugToolButton($, { name:"Abort", variant:0 }),
		DebugToolButton($, { name:"Break", variant:12 }),
		DebugToolButton($, { name:"Go", variant:1 }),
		DebugToolButton($, { name:"Step", variant:2 }),
		DebugToolButton($, { name:"StepIn", variant:3 }),
		DebugToolButton($, { name:"StepOut", variant:4 }),
	],
}));


var DebugToolButton = Content.template($ => ({
	skin:buttonsSkin, active:false, Behavior: DebugButtonBehavior,
}));


var CallTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:CallTableBehavior,
	contents: [
		CallHeader($, { name:"HEADER" }),
	],
}));

var CallHeader = Line.template(function($) { return {
	left:0, right:0, height:27, skin:tableHeaderSkin, active:true,
	Behavior: CallHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:3, skin:glyphsSkin, variant:0 }),
		Label($, { left:0, right:0, style:tableHeaderStyle, string:"CALLS" }),
	],
}});

var CallFooter = Line.template(function($) { return {
	left:0, right:0, height:3, skin:tableFooterSkin,
}});


var CallLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:callLineSkin, active:true, 
	Behavior:CallLineBehavior,
	contents: [
		Content($, { width:lineIndent, }),
		Label($, { style:tableLineStyle, string:$.name }),
	]
}});

var DebugTable = Column.template($ => ({
	left:0, right:0, active:true,
	contents: [
		DebugHeader($, { name:"HEADER" }),
	],
}));

var DebugHeader = Line.template(function($) { return {
	left:0, right:0, height:27, skin:tableHeaderSkin, active:true,
	Behavior: DebugHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:3, skin:glyphsSkin, variant:0 }),
		Label($, { left:0, right:0, style:tableHeaderStyle }),
	],
}});

var DebugFooter = Line.template(function($) { return {
	left:0, right:0, height:3, skin:tableFooterSkin,
}});

var DebugLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:tableLineSkin, active:$.state > 0, 
	Behavior:DebugLineBehavior,
	contents: [
		Content($, { width:lineIndent + ($.column * 20) }),
		Content($, { skin:glyphsSkin, state:$.state }),
		Label($, { style:debugNameStyle, string:$.name }),
		Label($, { style:debugValueStyle, string:$.state == 0 ? (" = " + $.value) : "" }),
	]
}});

var RunningContent = Content.template($ => ({ skin:waitSkin, Behavior: SpinnerBehavior}));
