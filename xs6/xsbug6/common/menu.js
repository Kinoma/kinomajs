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
export var dialogBoxTexture = new Texture("assets/dialogBox.png", 1);
export var dialogBoxSkin = new Skin({ texture: dialogBoxTexture, x:0, y:0, width:70, height:70, 
	tiles: { left:30, right: 30, top:30, bottom: 30 },
	margins: { left:20, right: 20, top:20, bottom: 20 }
});

var bulletTexture = new Texture("assets/bullet.png", 2);
export var bulletSkin = new Skin({ texture: bulletTexture, x:0, y:0, width:20, height:20 });

export const menuLineSkin = new Skin({ fill:["transparent", "transparent", "#f2f2f2", "#e0e0e0"] });
export const menuLineStyle = new Style({ font: "bold", color: ["gray", "black", "black", "black"], horizontal:"left" })

import {
	ButtonBehavior
} from "control";

import {
	ScrollerBehavior
} from "scrollbar";

export class MenuButtonBehavior extends ButtonBehavior {
	changeState(container, state) {
		super.changeState(container, this.waiting ? 3 : state);
	}
	onCreate(container, data) {
		super.onCreate(container, data);
		this.waiting = false;
	}
	onDescribeMenu(container) {
		return {
			items:[],
			selection:-1,
			context: shell,
		};
	}
	onMenuCanceled(container, selection) {
	}
	onMenuSelected(container, selection) {
	}
	onTap(container) {
		if (container.first)
			container.first.state = 1;
		let data = this.onDescribeMenu(container);
		if (data.items.length) {
			var context = data.context;
			data.button = container;
			if (!("ItemTemplate" in data))
				data.ItemTemplate = MenuItemLine;
			if (!("horizontal" in data))
				data.horizontal = "left";
			if (!("vertical" in data))
				data.vertical = "bottom";
			data.left = container.x - context.x;
			data.right = context.x + context.width - container.x - container.width;
			if (data.horizontal == "left")
				delete data.right;
			else if (data.horizontal == "right")
				delete data.left;
			if (data.vertical == "bottom")
				data.top = container.y - context.y + container.height;
			else if (data.vertical == "top")
				data.top = container.y - context.y;
			else
				data.top = container.y - context.y - (data.selection * container.height);
			context.add(new MenuDialog(data));
			this.waiting = true;
			this.changeState(container, 3);
		}
		else
			container.delegate("onMenuEmpty");
	}
};

export class MenuItemBehavior extends ButtonBehavior {
	onTap(line) {
		line.bubble("onSelected", this.data);
	}
}

export var MenuDialog = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: class extends Behavior {
		onTouchEnded(container, id, x, y, ticks) {
			var layout = container.first;
			if (!layout.hit(x, y))
				layout.behavior.onCancel(layout);
		}
	},
	contents: [
		Layout($, {
			[$.horizontal]:$[$.horizontal],
			left:$.left, right:$.right,
			top:$.top,
			skin: dialogBoxSkin,
			Behavior: class extends Behavior {
				onCancel(layout) {
					this.onClose(layout);
					this.data.button.delegate("onMenuCanceled", item);
				}
				onClose(layout) {
					let data = this.data;
					let button = data.button;
					let context = data.context;
					context.remove(context.last);
					button.behavior.waiting = false;
					button.behavior.changeState(button, 1);
				}
				onCreate(layout, data) {
					this.data = data;
					if (data.selection >= 0) {
						let column = layout.first.first;
						let content = column.content(data.selection);
						content.first.visible  = true;
					}
				}
				onMeasureHorizontally(layout, width) {
					if (layout.left && layout.right)
						return layout.width;
					let size = layout.first.first.measure();
					return size.width;
				}
				onMeasureVertically(layout, height) {
					let data = this.data;
					let button = data.button;
					let context = data.context;
					let delta = (context.y + context.height) - (button.y + button.height) - 20;
					let size = layout.first.first.measure();
					return Math.min(size.height, delta);
				}
				onSelected(layout, item) {
					this.onClose(layout);
					this.data.button.delegate("onMenuSelected", item);
				}
			},
			contents: [
				Scroller($, {
					left:0, right:0, top:0, bottom:0, clip: true, active:true,
					Behavior: ScrollerBehavior,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							contents: $.items.map(item => {
								return new $.ItemTemplate(item);
							}),
						})
					]
				}),
			],
		}),
	],
}));

var MenuItemLine = Line.template($ => ({
	left:0, right:0, height:30, skin:menuLineSkin, active:true,
	Behavior:MenuItemBehavior,
	contents: [
		Content($, { width:30, height:30, skin:bulletSkin, visible:false }),
		Label($, {left:0, height:30, style:menuLineStyle, string:$.title }),
	]
}));


export var popupTexture = new Texture("assets/popup.png", 1);
export var popupSkin = new Skin({ texture: popupTexture, x:0, y:0, width:40, height:50, 
	tiles: { left:10, right: 10, top:20, bottom: 20 },
	margins: { left:3, right: 3, top:20, bottom: 20 }
});
export var dropSkin = new Skin({ texture: popupTexture, x:0, y:0, width:40, height:50, states:50,
	tiles: { left:10, right: 10, top:20, bottom: 20 },
	margins: { left:3, right: 3, top:10, bottom: 10 }
});
export var dropArrowSkin = new Skin({ texture: popupTexture, x:40, y:60, width:40, height:20 });
export var dropSideSkin = new Skin({ texture: popupTexture, x:40, y:60, width:20, height:40, variants:20 });
export var popupShadowsSkin = new Skin({ texture: popupTexture, x:40, y:0, width:40, height:20, states:20,
	tiles: { left:20, right:0 },
});
export var popupCheckSkin = new Skin({ texture: popupTexture, x:80, y:0, width:20, height:20 });
export var popupArrowsSkin = new Skin({ texture: popupTexture, x:80, y:20, width:20, height:10, states:10 });
export var popupItemStyle = new Style({ font: "Open Sans", size:14, color: ["gray", "black", "black", "black"], horizontal:"left", left:10 });
export const menuIconSkin = new Skin({ texture:popupTexture, x:80, y:40, width:20, height:20, states:20 });

export class PopupButtonBehavior extends ButtonBehavior {
	onDescribeMenu(container) {
		return {
			items:[],
			context: shell,
		};
	}
	onMenuSelected(container, value) {
	}
	onTap(container) {
		container.first.state = 1;
		let data = this.onDescribeMenu();
		if (data.items.length) {
			data.button = container;
			if (!("ItemTemplate" in data))
				data.ItemTemplate = PopupItem;
			data.context.add(new PopupDialog(data));
		}
		else
			container.delegate("onMenuEmpty");
	}
};

class PopupDialogBehavior extends Behavior {	
	onCreate(layout, data) {
		this.data = data;
		let index = data.items.findIndex(item => item == data.value);
		let column = layout.first.first;
		if ((0 <= index) && (index < column.length)) {
			this.selection = index;
			column.content(index).first.visible = true;
		}
	}
	onDisplayed(layout) {
		shell.behavior.onHover();
	}
	onMeasureHorizontally(layout, width) {
		return width;
	}
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button;
		let scroller = layout.first;
		let column = scroller.first;
		let index = this.selection;
		let step = column.first.measure().height;
		let menuCoordinates = { left:button.x, width:button.width, top:0, height:0 }
		let listCoordinates = { left:0, right:0, top:0 };

		let y = button.y + ((button.height - step) >> 1);
		let listTop = y - (index * step);
		scroller.coordinates = { left:button.x, width:button.width, top:0, bottom:0 };
		scroller.tracking = true;
		scroller.scrollTo(0, 0 - listTop);
		return height;
	}
	onSelected(layout, item) {
		let data = this.data;
		let context = data.context;
		data.context.remove(this.data.context.last);
		data.button.bubble("onMenuSelected", item);
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var column = layout.first.first;
		if (!column.hit(x, y))
			this.onSelected(layout, null);
	}
};

class PopupScrollerBehavior extends Behavior {	
	onTouchScrolled(scroller, touched, dx, dy) {
		var content = scroller.first;
		var size = scroller.height;
		var range = content.height;
		if (dy < 0) {
			var offset = (content.y + range + content.skin.margins.bottom) - (scroller.y + size);
			if (offset > 0)
				scroller.scrollBy(0, Math.min(-dy, offset));
		}
		else if (dy > 0) {
			var offset = scroller.y - content.y + content.skin.margins.top;
			if (offset > 0)
				scroller.scrollBy(0, -Math.min(dy, offset));
		}
	}
};

class PopupShadowBehavior extends Behavior {	
	onCreate(shadow, delta) {
		this.delta = delta;
		this.acceleration = 0.01;
	}
	onMouseEntered(shadow, x, y) {
		this.now = Date.now();
		shadow.start();
	}
	onMouseExited(shadow, x, y) {
		shadow.stop();
	}
	onTimeChanged(code) {
		var scroller = code.container;
		var now = Date.now();
		scroller.scrollBy(0, this.delta * this.acceleration * (now - this.now));
	}
	onScrolled(shadow) {
		var scroller = shadow.container;
		var content = scroller.first;
		var size = scroller.height;
		var range = content.height;
		if (this.delta > 0) {
			var offset = (content.y + range + content.skin.margins.bottom) - (scroller.y + size);
			if (offset > 0) {
				shadow.active = shadow.visible = true;
			}
			else {
				shadow.active = shadow.visible = false;
				shadow.stop();
			}
		}
		else {
			var offset = scroller.y - content.y + content.skin.margins.top;
			if (offset > 0) {
				shadow.active = shadow.visible = true;
			}
			else {
				shadow.active = shadow.visible = false;
				shadow.stop();
			}
		}
	}
}

export var PopupDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PopupDialogBehavior,
	contents: [
		Scroller($, {
			active:true,
			Behavior: PopupScrollerBehavior,
			contents: [
				Column($, {
					left:0, right:0, top:0, skin: popupSkin, 
					contents: $.items.map(item => {
						return new $.ItemTemplate(item);
					}),
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

export var PopupItem = Line.template($ => ({
	left:0, right:0, height:30, skin:menuLineSkin, active:true,
	Behavior:MenuItemBehavior,
	contents: [
		Content($, { width:30, height:30, skin:popupCheckSkin, visible:false }),
		Label($, {left:0, height:30, style:popupItemStyle, string:$.toString() }),
	]
}));

export class DropDialogBehavior extends Behavior {	
	onClose(layout, item) {
		let data = this.data;
		shell.remove(shell.last);
		data.button.bubble("onDialogClosed", item);
	}
	onCreate(layout, data) {
		this.data = data;
	}
	onDisplayed(layout) {
		shell.behavior.onHover();
	}
	onMeasureHorizontally(layout, width) {
		return width;
	}
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button;
		let scroller = layout.first;
		let size = scroller.first.measure();
		let delta = shell.height - button.y - button.height - 20;
		let scrollerCoordinates = { left:button.x, width:button.width, top:button.y, height:0 }
		scroller.coordinates = { left:button.x, width:button.width, top:button.y, height:Math.min(size.height, delta) }
		return height;
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first.first;
		if (!content.hit(x, y))
			this.onClose(layout);
	}
};

export var DropDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: DropDialogBehavior,
	contents: [
		Scroller($, {
			skin:dropSkin, state:$.variant, clip:true, active:true,
			Behavior: ScrollerBehavior,
			contents: [
				$.Template($, {}),
			]
		}),
	],
}));
