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

// ASSETS

import {
	BOLD_FONT,
	FIXED_FONT,
	LIGHT_FONT,
	NORMAL_FONT,
	PASTEL_YELLOW,
	SEMIBOLD_FONT,
	BLACK,
	CYAN,
	GRAYS,
	WHITE,

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

var fileLineStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"left" });
var fileLineSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], CYAN] });

var tableHeaderStyle = new Style({ font:BOLD_FONT, size:12, color:GRAYS[75], horizontal:"left" });
var tableLineStyle = new Style({ font:NORMAL_FONT, size:12, color:[BLACK, BLACK, BLACK, WHITE], horizontal:"left" });
var debugNameStyle = new Style({ font:NORMAL_FONT, size:12, color:[BLACK, BLACK, BLACK, WHITE] });
var debugValueStyle = new Style({ font:LIGHT_FONT, size:12, color:[BLACK, BLACK, BLACK, WHITE] });
var infoLineStyle = new Style({ font:NORMAL_FONT, size:12, color:GRAYS[50] , horizontal:"left" });

const searchEmptyStyle = new Style({ font:NORMAL_FONT, size:12, color:GRAYS[50] });

const resultLineSkin = new Skin({ fill:[WHITE, GRAYS[2], GRAYS[6], WHITE] });
const resultLabelSkin = new Skin({ fill:["transparent", "transparent", PASTEL_YELLOW, PASTEL_YELLOW] });
const resultLabelStyle = new Style({ font: FIXED_FONT, size:12, color:"#505050", horizontal:"left" });
const resultCountStyle = new Style({ font: NORMAL_FONT, size:12, color:GRAYS[50], horizontal:"right", right:5 });

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
	SpinnerBehavior,
} from "shell/behaviors";

class FilePaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		this.onHomesChanged(container);
	}
	onHomesChanged(container, home) {
		let scroller = container.first;
		let column = scroller.first;
		let data = this.data;
		let items = data.homes.items;
		let target = null;
		column.empty(1);
		if (items.length) {
			column.add(new SearchTable(data.search));
			items.forEach(item => { 
				let table = new HomeTable(item);
				column.add(table);
				if (item == home)
					target = table
			});
			if (target) {
				let bounds = target.bounds;
				bounds.x -= column.x;
				bounds.y -= column.y;
				scroller.reveal(bounds);
			}
		}
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

class FileLineBehavior extends LineBehavior {
	onCreate(line, data) {
		super.onCreate(line, data);
		this.onURLChanged(line, model.url);
	}
	onTap(line) {
		model.doOpenURL(this.data.url);
	}
	onURLChanged(line, url) {
		if (this.data.url == url) {
			line.last.style = fileLineStyle; 
			this.flags |= 4;
		}
		else {
			line.last.style = tableLineStyle;
			this.flags &= ~4;
		}
		this.changeState(line);
	}
};

class BreakpointTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			for (let item of data.items)
				column.add(new BreakpointLine(item));
			column.add(new BreakpointFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	hold(column) {
		return BreakpointHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		this.expand(column, data.expanded);
	}
	onBreakpointsChanged(column, data) {
		var data = this.data;
		this.expand(column, data.expanded);
	}
}

class BreakpointHeaderBehavior extends HeaderBehavior {
	reveal(line, revealIt) {
		line.last.visible = revealIt;
	}
};

class BreakpointLineBehavior extends LineBehavior {
	onTap(line) {
		let data = this.data;
		model.doOpenURL(data.url, data.line);
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
			let name = info ? info.path : null;
			let order = former ? name ? former.name.compare(name) : -1 : 1;
			if (order < 0) {
				model.doCloseFile(former.url);
				i++;
				former = (i < c) ? formers[i] : null;
			}
			else if (order > 0) {
				let item;
				if (info.type == Files.fileType) {
					if (name.endsWith(".js") || name.endsWith(".json") || name.endsWith(".xml") || name.endsWith(".xs"))
						item = { depth, kind:"file", name, url:mergeURI(data.url, name) };
					else
						item = { depth, kind:"info", name, url:mergeURI(data.url, name) };
				}
				else
					item = { depth, kind:"folder", name, url:mergeURI(data.url, name + "/"), expanded:false, items:[] };
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

class HomeTableBehavior extends FolderTableBehavior {
	hold(column) {
		return HomeHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onDirectoryChanged(column) {
		super.onDirectoryChanged(column);
		if (this.data.expanded) {
			column.add(new HomeFooter(data));
		}
	}
};

class HomeHeaderBehavior extends FolderLineBehavior {
	reveal(line, revealIt) {
		line.CLOSE.visible = revealIt;
	}
};

class HomeCloseButtonBehavior extends ButtonBehavior {
	onTap(button) {
		model.doCloseDirectory(this.data.url);
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
			table.add(new SearchFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
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
				items: model.homes.items.map(item => ({ name:item.name, url:item.url }))
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
		model.doOpenURL(line.container.behavior.data.url, data.offset + "-" + (data.offset + data.length));
	}
};

// TEMPLATES

import {
	FindField,
	findModeToCaseless,
	findModeToPattern,
} from "find";

export var FilePane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, skin:paneBackgroundSkin,
	Behavior: FilePaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:0, right:0, top:0, Behavior:HolderColumnBehavior, 
					contents: [
						BreakpointTable($.breakpoints, {}),
					]
				}),
				Container($, {
					left:0, right:0, top:0, height:26, clip:true, Behavior:HolderContainerBehavior,
				}),
				VerticalScrollbar($, {}),
			]
		}),
	]
}});

var BreakpointTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:BreakpointTableBehavior,
	contents: [
		BreakpointHeader($, { name:"HEADER" }),
	],
}));

var BreakpointHeader = Line.template(function($) { return {
	left:0, right:0, height:27, skin:tableHeaderSkin, active:true,
	Behavior: BreakpointHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:3, skin:glyphsSkin, variant:0, state:1 }),
		Label($, { left:0, right:0, style:tableHeaderStyle, string:"BREAKPOINTS" }),
		Content($, { top:0, skin:buttonsSkin, variant:5, active:true, visible:false, 
			Behavior: class extends ButtonBehavior {
				onBreakpointsChanged(button) {
					button.active = model.canClearAllBreakpoints();
				}
				onTap(button) {
					model.doClearAllBreakpoints();
				}
			},
		}),
	],
}});

var BreakpointFooter = Line.template(function($) { return {
	left:0, right:0, height:3, skin:tableFooterSkin,
}});

var BreakpointLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:tableLineSkin, active:true, 
	Behavior:BreakpointLineBehavior,
	contents: [
		Content($, { width:lineIndent, }),
		Label($, { style:debugNameStyle, string:$.name }),
		Label($, { style:debugValueStyle, string:" (" + $.line + ")" }),
	]
}});

var HomeTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: HomeTableBehavior,
	contents: [
		HomeHeader($, {}),
	],
}});

var HomeHeader = Line.template(function($) { return {
	left:0, right:0, height:27, skin:tableHeaderSkin, active:true,
	Behavior: HomeHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:3, skin:glyphsSkin, state:$.expanded ? 3 : 1, variant:0 }),
		Label($, { name:"TITLE", left:0, right:0, style:tableHeaderStyle, string:$.name }),
		Content($, { name:"CLOSE", top:0, skin:buttonsSkin, variant:6, state:1, active:true, visible:false, Behavior:HomeCloseButtonBehavior }),
	],
}});

var HomeFooter = Line.template(function($) { return {
	left:0, right:0, height:3, skin:tableFooterSkin,
}});

var FolderTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: FolderTableBehavior,
	contents: [
		FolderHeader($, {}),
	],
}});

var FolderHeader = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:tableLineSkin, active:true,
	Behavior: FolderLineBehavior,
	contents: [
		Content($, { width:lineIndent + (($.depth - 1) * 20) }),
		Content($, { skin:glyphsSkin, state:$.expanded ? 3 : 1, variant:0}),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
	],
}});

var FileLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:fileLineSkin, active:true,
	Behavior: FileLineBehavior,
	contents: [
		Content($, { width:lineIndent + (($.depth - 1) * 20) }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
	],
}});

var InfoLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:tableLineSkin,
	Behavior: LineBehavior,
	contents: [
		Content($, { width:lineIndent + (($.depth - 1) * 20) }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:infoLineStyle, string:$.name }),
	],
}});

var FileKindTemplates = {
	file: FileLine,
	folder: FolderTable,
	home: HomeTable,
	info: InfoLine,
};


var SearchTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: SearchTableBehavior,
	contents: [
		SearchHeader($, {}),
	],
}});

var SearchHeader = Line.template(function($) { return {
	left:0, right:1, height:27, skin:tableHeaderSkin, active:true,
	Behavior: SearchHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:3, skin:glyphsSkin, state:$.expanded ? 3 : 1, variant:0 }),
		FindField($, { top:2, bottom:3 }),
		Content($, { width:2 }),
	],
}});

var SearchFooter = Line.template(function($) { return {
	left:0, right:0, height:3, skin:tableFooterSkin,
}});

var SearchSpinner = Container.template($ => ({
	left:0, right:0, height:26, skin:tableLineSkin, 
	contents: [
		Content($, { skin:waitSkin, Behavior: SpinnerBehavior }),
	],
}));

var SearchEmpty = Line.template($ => ({
	left:0, right:0, height:lineHeight, skin:tableLineSkin, 
	contents: [
		Label($, { left:0, right:0, style:searchEmptyStyle, string:"Not Found!" }),
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
	left:0, right:0, height:lineHeight, skin:tableLineSkin, active:true,
	Behavior: ResultHeaderBehavior,
	contents: [
		Content($, { width:lineIndent }),
		Content($, { skin:glyphsSkin, state:$.expanded ? 3 : 1, variant:0}),
		Label($, { left:0, right:0, style:tableLineStyle, string:$.name }),
		Label($, { style:resultCountStyle, string:$.count }),
	],
}});

var ResultLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:resultLineSkin, active:true,
	Behavior: ResultLineBehavior,
	contents: [
		Content($, { width:lineIndent + 40 }),
		Label($, { left:0, skin:resultLabelSkin, style:resultLabelStyle, string:$.string, selectable:true }),
	],
}});

