export function Configure(model) {
	if (model.screenConfigurator) {
		shell.run(new DialogCloseTransition, model.screenConfigurator);
		delete model.screenConfigurator;
	}
	else {
		model.screenConfigurator = new ScreenConfigurator(model);
		shell.run(new DialogOpenTransition, model.screenConfigurator);
	}
}

var WHITE = "#ffffff";
var TRANSPARENT = "transparent";

var PASTEL_GRAY = "#f7f7f7";
var HOVER_GRAY = "#ececec";
var CLICK_GRAY = "#c7c7c7";
var BORDER_GRAY = "#afafaf";
var TEXT_GRAY = "#202020";

const configureTexture = new Texture("./configure.png", 2);

const backgroundSkin = new Skin({ fill:"#e0e0e0" });
const headerSkin = new Skin({ stroke:BORDER_GRAY, borders:{ bottom:1 } });

const buttonSkin = new Skin({ texture: configureTexture, x:0, y:0, width:60, height:30, states:30, tiles: { left:15, right:15 } });
const popupButtonSkin = new Skin({ texture: configureTexture, x:60, y:0, width:60, height:30, states:30, tiles: { left:15, right:15 } });
const checkBoxSkin = new Skin({ texture: configureTexture, x:125, y:0, width:20, height:30, states:30, variants:30 });
const closeSkin = new Skin({ texture: configureTexture, x:180, y:0, width:30, height:30, states:30 });
const selectionSkin = new Skin({ texture: configureTexture, x:210, y:0, width:30, height:30, states:30, tiles: { left:5, right:5, top:5, bottom:5 } });


const scrollerSkin = new Skin({ fill: [ TRANSPARENT, WHITE ], stroke:BORDER_GRAY, borders:{ left:1, right:1, top:1, bottom:1 } });
const valueSkin = new Skin({ fill: [ TRANSPARENT, TRANSPARENT, "#a4ff03", "#a4ff03" ] });

const titleStyle = new Style({ font:"bold", size:20, color:"#404040", horizontal:"left", left:10 });

const nameStyle = new Style({ size:14, color:TEXT_GRAY, horizontal:"right", right:10 });
const valueStyle = new Style({ size:14, color:TEXT_GRAY, horizontal:"left", left:5 });
const unitStyle = new Style({ size:14, color:TEXT_GRAY, horizontal:"left", left:10 });

const buttonStyle = new Style({ size:14, color:[BORDER_GRAY, TEXT_GRAY, TEXT_GRAY, TEXT_GRAY] });
const popupButtonStyle = new Style({ size:14, color:TEXT_GRAY, horizontal:"left", left:5 });
const checkBoxStyle = new Style({ size:14, color:TEXT_GRAY, horizontal:"left", left:5 });

const popupDialogSkin = new Skin({ texture: configureTexture, x:240, y:0, width:30, height:30, 
	tiles: { left:12, right: 12, top:12, bottom: 12 },
	margins: { left:5, right: 5, top:12, bottom: 12 }
});
const popupCheckSkin = new Skin({ texture: configureTexture, x:245, y:35, width:20, height:20 });
const popupArrowsSkin = new Skin({ texture: configureTexture, x:245, y:65, width:20, height:10, states:10 });
const popupShadowsSkin = new Skin({ texture: configureTexture, x:245, y:95, width:20, height:10, states:10, tiles: { left:0, right:0 } });
const popupLineSkin = new Skin({ fill:[TRANSPARENT, TRANSPARENT, HOVER_GRAY, BORDER_GRAY] });

var horizontalScrollbarSkin = new Skin({ 
	fill: [ TRANSPARENT, PASTEL_GRAY, PASTEL_GRAY ],
	stroke: [ TRANSPARENT, HOVER_GRAY, HOVER_GRAY ],
	borders: { top:1 },
});
var verticalScrollbarSkin = new Skin({ 
	fill: [ TRANSPARENT, PASTEL_GRAY, PASTEL_GRAY ],
	stroke: [ TRANSPARENT, HOVER_GRAY, HOVER_GRAY ],
	borders: { left:1 },
});
var scrollbarThumbSkin = new Skin({
	fill: [ "#00e0e0e0", "#FFe0e0e0", HOVER_GRAY, CLICK_GRAY ],
});

import {
	ButtonBehavior,
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
	PopupDialogBehavior,
	PopupItemBehavior,
	PopupScrollerBehavior,
	PopupShadowBehavior,
	ScrollerBehavior,
	HorizontalScrollbarBehavior,
	VerticalScrollbarBehavior,
} from "behaviors";

class ScreenConfiguratorBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		container.distribute("onScreenChanged");
		container.distribute("onScreenModeChanged", this.data.screenMode);
	}
};

class CustomBehavior extends ButtonBehavior {
	onScreenModeChanged(container, mode) {
		container.first.variant = mode ? 0 : 1;
	}
	onTap(container) {
		shell.delegate("changeScreenMode", !this.data.screenMode);
	}
};

class SizeLineBehavior extends Behavior {
	onCreate(label, data) {
		this.data = data;
	}
	onEnter(line) {
		let scroller = line.first.next;
		let label = scroller.first;
		let value = parseInt(label.string);
		if (isNaN(value) || (value <= 160))
			value = 160;
		else if (value > 1600)
			value = 1600;
		shell.behavior.currentScreen[this.data.name] = value;
		shell.delegate("changeScreen");
		shell.focus();
	}
	onEscape(line) {
		let scroller = line.first.next;
		let label = scroller.first;
		label.string = shell.behavior.currentScreen[this.data.name];
		shell.focus();
	}
	onKeyDown(line, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if ((c == 9) || (c== 25)) {
			return true;
		}
		if ((c == 3) || (c== 13)) {
			this.onEnter(line);
			return true;
		}
		if (c == 27) {
			this.onEscape(line);
			return true;
		}
		return false;
	}
	onKeyUp(line, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		return (c == 3) || (c== 9) || (c== 13) || (c== 25) || (c== 27);
	}
	onScreenModeChanged(line, mode) {
		let scroller = line.first.next;
		let label = scroller.first;
		scroller.active = label.editable = mode ? false : true;
	}
};

class SizeLabelBehavior extends FieldLabelBehavior {
	onEdited(label) {
		let button = label.container.next.next;
		button.active = button.visible = true;
	}
	onScreenChanged(label) {
		label.string = shell.behavior.currentScreen[this.data.name];
	}
	onUnfocused(label) {
		let button = label.container.next.next;
		button.active = button.visible = false;
	}
};

class ScaleButtonBehavior extends ButtonBehavior {
	onMenuSelected(container, value) {
		if (!value)
			return;
		this.data.currentScreen.scale = parseFloat(value);
		shell.delegate("changeScreen");
	}
	onScreenChanged(container) {
		container.first.string = this.data.currentScreen.scale;
	}
	onScreenModeChanged(container, mode) {
		container.active = mode ? false : true;
	}
	onTap(container) {
		shell.add(new ScalePopupDialog({ 
			button:container, 
			items:[ "1", "1.5", "2" ],
			value: container.first.string,
		}));
	}
};

class KindGridBehavior extends Behavior {
	getCellWidth() {
		return 100;
	}
	getCellHeight(width) {
		return 80;
	}
	onCreate(column, data) {
		this.data = data;
	}
	onMeasureHorizontally(container, width) {
		return width;
	}
	onMeasureVertically(container, height) {
		var total = container.width;
		var c = Math.floor(total / this.getCellWidth());
		var gutter = 8;
		var width = Math.floor((total - ((c + 1) * gutter)) / c);
		if (width & 1) width--;
		var height = this.getCellHeight(width);
		var coordinates = {left:0, width: width, top:0, height: height};
		var xs = new Array(c);
		var ys = new Array(c);
		for (var i = 0; i < c; i++) {
			xs[i] = gutter + (i * (width + gutter));
			ys[i] = gutter;
		}
		var content = container.first;
		while (content) {
			var min = 0x7FFFFFFF;
			var j = 0;
			for (var i = 0; i < c; i++) {
				var y = ys[i];
				if (y < min) {
					min = y;
					j = i;
				}
			}
			coordinates.left = xs[j];
			coordinates.top = min;
			content.coordinates = coordinates;
			ys[j] = min + coordinates.height + gutter;
			content = content.next;
		}
		height = 0;
		for (var i = 0; i < c; i++) {
			var y = ys[i];
			if (y > height)
				height = y;
		}
		return height;
	}
	onSelectCell(container, index) {
		let data = this.data;
		data.currentScreen.kind = index;
		if (data.screenMode)
			shell.delegate("changeScreenMode", true);
		else
			shell.delegate("changeScreen");
	}
	onScreenChanged(container) {
		container.distribute("onCellSelected", this.data.currentScreen.kind);
	}
};

class KindCellBehavior extends ButtonBehavior {
	onCellSelected(line, index) {
		if (line.index == index) {
			line.active = false;
			this.changeState(line, 0);
		}
		else {
			line.active = true;
			this.changeState(line, 1);
		}
	}
	onTap(line) {
		line.container.focus();
		line.bubble("onSelectCell", line.index);
	}
};

export var ScreenConfigurator = Container.template($ => ({
	left:0, width:360, top:0, bottom:0, skin:backgroundSkin, active:true, backgroundTouch:true,
	Behavior: ScreenConfiguratorBehavior,
	contents: [
		Column($, {
			left:0, right:0, top:0, bottom:0,
			contents: [
				HeaderLine($, {}),
				Content($, { height:10 }),
				CustomLine($, {}),
				SizeLine({ title:"Width:", name:"width" }, {}),
				SizeLine({ title:"Height:", name:"height" }, {}),
				ScaleLine($, {}),
				Container($, {
					left:10, right:10, top:10, bottom:10, skin:scrollerSkin, state:1, 
					contents: [
						Scroller($, {
							left:1, right:1, top:1, bottom:1, active:true, clip:true, Behavior:ScrollerBehavior, 
							contents: [
								Layout($, {
									anchor:"KIND", left:0, right:0, top:0, 
									Behavior: KindGridBehavior,
									contents: $.resources.map($$ => new KindCell($$)),
								}),
								VerticalScrollbar($, {}),
							]
						}),
					],
				}),
			],
		}),
	],
}));

var HeaderLine = Line.template($ => ({
	left:0, right:0, top:0, height:40, skin:headerSkin,
	Behavior: class extends Behavior {
		onCreate(container, data) {
			this.data = data;
		}
		onScreenChanged(container) {
			let data = this.data;
			container.first.string = "Configure Screen \u2318" + data.screens.indexOf(data.currentScreen);
		}
	},
	contents: [ 
		Label($, { left:0, right:0, style:titleStyle, string:"Configure" }),
		Content($, { width:40, height:40, skin:closeSkin, active:true, Behavior: ButtonBehavior, name:"doConfigure" }),
	] 
}));

var CustomLine = Line.template($ => ({
	left:0, right:0, top:0, height:30,
	contents: [
		Content($, { width:80 }),
		Container($, {
			active:true, 
			Behavior:CustomBehavior,
			contents: [
				Content($, { left:0, width:20, top:0, height:30, skin:checkBoxSkin }),
				Label($, { left:20, top:0, height:30, style:checkBoxStyle, string:"Custom" }),
			],
		}),
	],
}));

var SizeLine = Line.template($ => ({
	left:0, right:0, height:30, skin:backgroundSkin, active:true,
	Behavior: SizeLineBehavior,
	contents: [
		Label($, { width:80, style:nameStyle, string:$.title }),
		Scroller($, {
			width:70, top:4, bottom:4, skin:scrollerSkin, clip:true,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					left: 0, top:2, bottom:2, skin:valueSkin, style:valueStyle,
					Behavior: SizeLabelBehavior
				}),
			],
		}),
		Label($, { width:100, style:unitStyle, string:"physical pixels" }),
		Container($, {
			width:60, skin:buttonSkin, visible:false, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:buttonStyle, string:"Set" }),
			],
		}),
	],
}));

var ScaleLine = Line.template($ => ({
	left:0, right:0, height:30, skin:backgroundSkin, 
	contents: [
		Label($, { width:80, style:nameStyle, string:"Scale:" }),
		Container($, { 
			width:70, skin:popupButtonSkin, variant:2, active:true,
			Behavior: ScaleButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:popupButtonStyle }),
			]
		}),
		Label($, { left:0, right:0, style:unitStyle, string:"physical pixel by logical pixel" }),
	],
}));

var ScalePopupDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PopupDialogBehavior,
	contents: [
		Scroller($, {
			active:true,
			Behavior: PopupScrollerBehavior,
			contents: [
				Column($, {
					left:0, right:0, top:0, skin: popupDialogSkin, 
					contents: $.items.map(item => new ScalePopupItem(item)),
				}),
				Container(-1, { 
					left:0, right:0, top:0, height:20, skin:popupShadowsSkin, state:0, active:true, 
					Behavior: PopupShadowBehavior,
					contents: [
						Content($, { right:5, top:5, skin:popupArrowsSkin, state:0 }),
					],
				}),
				Container(1, { 
					left:0, right:0, height:20, bottom:0, skin:popupShadowsSkin, state:1, active:true, 
					Behavior: PopupShadowBehavior,
					contents: [
						Content($, { right:5, bottom:5, skin:popupArrowsSkin, state:1 }),
					],
				}),
			],
		}),
	],
}));

var ScalePopupItem = Line.template($ => ({
	left:0, right:0, height:22, skin:popupLineSkin, active:true,
	Behavior:PopupItemBehavior,
	contents: [
		Content($, { width:22, height:22, skin:popupCheckSkin, visible:false }),
		Label($, {left:0, height:22, style:popupButtonStyle, string:$ }),
	]
}));

var HorizontalScrollbar = Container.template($ => ({
	left:0, right:0, height:10, bottom:0, skin: horizontalScrollbarSkin, active:true, Behavior:HorizontalScrollbarBehavior,
	contents: [
		Content($, { left:0, width:0, height:9, bottom:0, skin: scrollbarThumbSkin }),
	],
}));

var VerticalScrollbar = Container.template($ => ({
	width:10, right:0, top:0, bottom:0, skin: verticalScrollbarSkin, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { right:0, width:9, top:0, height:0, skin: scrollbarThumbSkin }),
	],
}));

var KindCell = Container.template($ => ({
	left:0, width:80, top:0, height:80, active:true,
	Behavior: KindCellBehavior,
	contents: [
		Container($, { 
			width:80, height:80, skin:selectionSkin, 
			contents: [
				Content($, { width:64, height:64, skin:$.iconSkin }),
			]
		}),
	],
}));


// TRANSITIONS

class DialogTransition extends Transition {
	constructor() {
		super(400);
	}
	onBegin(container, dialog) {
		this.dialog = dialog;
		var layer = this.layer = new Layer();
		layer.attach(dialog);
		this.delta = 0 - layer.width
	}
	onEnd(container, dialog) {
		this.layer.detach();
	}
}

export class DialogOpenTransition extends DialogTransition {
	onBegin(container, dialog) {
		container.add(dialog);
		super.onBegin(container, dialog);
	}
	onEnd(container, dialog) {
		super.onEnd(container, dialog);
	}
	onStep(fraction) {
		fraction = Math.quadEaseOut(fraction);
		this.layer.translation = { y: 0, x: this.delta * (1 - fraction) };
	}
}

export class DialogCloseTransition extends DialogTransition {
	onBegin(container, dialog) {
		super.onBegin(container, dialog);
	}
	onEnd(container, dialog) {
		super.onEnd(container, dialog);
		container.remove(dialog);
	}
	onStep(fraction) {
		fraction = Math.quadEaseIn(fraction);
		this.layer.translation = { y: 0, x: this.delta * fraction };
	}
}
