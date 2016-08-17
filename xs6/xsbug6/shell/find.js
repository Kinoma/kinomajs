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
	BLACK,
	BOLD_FONT,
	DARKER_GRAY,
	DARKER_RED,
	FIXED_FONT,
	GRAY,
	LIGHT_GRAY,
	LIGHT_ORANGE,
	PASTEL_CYAN,
	PASTEL_ORANGE,
	PASTEL_GRAY,
	PASTEL_GREEN,
	PASTEL_YELLOW,
	DARK_GRAY,
	RED,
	SEMIBOLD_FONT,
	WHITE,
	buttonsSkin,
	redHeaderSkin,
	fieldLabelSkin,
	fieldScrollerSkin,
} from "shell/assets";	

export const findHintStyle = new Style({ font: FIXED_FONT, size:12, color:"silver", horizontal:"left", left:5, right:5});
export const findLabelStyle = new Style({ font: FIXED_FONT, size:12, color:"black", horizontal:"left", left:5, right:5});
var findModesTexture = new Texture("assets/findModes.png", 2);
var findModesSkin = new Skin({ texture: findModesTexture, x:0, y:0, width:20, height:20, variants:20, states:20 });

const pathSpanStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:["#aaaaaa","#aaaaaa","#777777","#444444"], horizontal:"left" });
const pathNameStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:BLACK, horizontal:"left" });

// BEHAVIORS

import { 
	ButtonBehavior, 
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

import { 
	CodeBehavior, 
} from "shell/behaviors";

export function findModeToPattern(findMode, findString) {
	let findPattern = findString;
	if (findPattern) {
		if (!(findMode & 8))
			findPattern = findPattern.replace(/(\\|\^|\$|\.|\+|\*|\?|\(|\)|\[|\]|\{|\}|\|)/g, "\\$1");
		if (findMode & 2)
			findPattern = "\\b" + findPattern;
		if (findMode & 4)
			findPattern = findPattern + "\\b";
	}
	return findPattern;
}

export function findModeToCaseless(findMode) {
	return (findMode & 1) == 0;
}

class FindModeBehavior extends ButtonBehavior {
	changeState(container, state) {
		switch (state) {
		case 1: container.state = this.data.findMode & this.mask ? 2 : 0; break;
		case 2: container.state = this.data.findMode & this.mask ? 3 : 1; break;
		case 3: container.state = this.data.findMode & this.mask ? 1 : 3; break;
		}
	}
	onCreate(container, data, dictionary) {
		super.onCreate(container, data, dictionary);
		this.mask = dictionary.mask;
	}
	onFound(container) {
		this.changeState(container, 1);
	}
	onRestore(container) {
		this.changeState(container, 1);
	}
	onTap(container) {
		var data = this.data;
		data.findMode ^= this.mask;
		this.changeState(container, 2);
		container.bubble("onFindEdited");
	}
}

class PathSpanBehavior extends ButtonBehavior {
	constructor(url) {
		super();
		this.url = url;
	}
	onMouseEntered(content, x, y) {
		content.state = 2;
	}
	onMouseExited(content, x, y) {
		content.state = 1;
	}
	onTap(content) {
		launchURI(this.url);
	}
	onTouchBegan(content, id, x, y, ticks) {
		content.state = 3;
		content.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(content, id, x, y, ticks) {
		if (content == content.container.hit(x, y)) {
			content.state = 2;
			this.onTap(content);
		}
		else
			content.state = 1;
	}
	onTouchMoved(content, id, x, y, ticks) {
		content.state = (content == content.container.hit(x, y)) ? 3 : 2;
	}
}

// TEMPLATES

export var FindField = Line.template($ => ({
	left:0, right:0, top:4, bottom:4, skin: fieldScrollerSkin,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, bottom:0,
			clip:true,
			active:true,
			Behavior: FieldScrollerBehavior,
			contents: [
				Code($, {
					anchor:"FIND_FOCUS",
					left: 0, top:2, bottom:2,
					skin:fieldLabelSkin,
					style:findLabelStyle,
					string: $.findString,
					active:true,
					editable: true,
					field: true,
					Behavior: class extends CodeBehavior {
						onEdited(code) {
							var data = this.data;
							data.findString = code.string;
							code.next.visible = data.findString.length == 0;
							code.bubble("onFindEdited");
						}
						onRestore(code) {
							var data = this.data;
							code.string = data.findString;
							code.next.visible = data.findString.length == 0;
							data.FIND_FOCUS = code;
						}
					},
				}),
				Label($, {
					left: 0, top:0, bottom:0,
					style:findHintStyle,
					string:$.findHint,
					visible:$.findString ? false : true,
				}),
			],
		}),
		Container($, {
			width:80, top:0, bottom:0,
			contents: [
				Content($, { width:20, right:62, height:20, skin:findModesSkin, variant:0, active:true, Behavior:FindModeBehavior, mask:1 }),
				Content($, { width:20, right:42, height:20, skin:findModesSkin, variant:1, active:true, Behavior:FindModeBehavior, mask:2 }),
				Content($, { width:20, right:22, height:20, skin:findModesSkin, variant:2, active:true, Behavior:FindModeBehavior, mask:4 }),
				Content($, { width:20, right:2, height:20, skin:findModesSkin, variant:3, active:true, Behavior:FindModeBehavior, mask:8 }),
			],
		}),
	],
}));
	
export var PathLayout = Layout.template($ => ({
	left:0, right:0, clip:true,
	Behavior: class extends Behavior {
		onCreate(layout, data) {
			this.data = data;;
		}
		onDisplaying(layout, data) {
			if (layout.container.skin == redHeaderSkin)
				layout.first.next.state = 1;
		}
		onMeasureVertically(layout, height) {
			let text = layout.first;
			let flag = text.next.visible = layout.width < text.width;
			text.coordinates = flag ? { right:0 } : { left:0 };
			return height;
		}
	},
	contents: [
		Text($, { 
			left:0, style:pathSpanStyle, active:true,
			Behavior: class extends Behavior {
				onCreate(text, data) {
					let separator = (system.platform == "win") ? "\\" : "/";
					let path = Files.toPath(data.url);
					let items = path.split(separator);
					let url = "file://";
					let name = items.pop();
					items = items.map(string => {
						url += string + "/";
						string += separator;
						let behavior = new PathSpanBehavior(url);
						return { behavior, string };
					});
					items.push({ string:name, style:pathNameStyle });
					text.format([{ spans:items }]);
				}
			},
		}),
		Content($, {
			left:0, width:30, height:30, skin:buttonsSkin, variant:13, visible:false,
		}),
	],
}));
