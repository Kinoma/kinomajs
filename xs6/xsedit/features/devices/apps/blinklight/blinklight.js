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
import Pins from "pins";

// ASSETS

import {
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";

import {
	ButtonBehavior
} from "common/control";

import {
	DropDialogBehavior,
	dropSideSkin,
	dropSkin,
	menuLineSkin,
	menuLineStyle,
	popupCheckSkin,
} from "common/menu";

import { 
	model,
} from "shell/main";

import {
	BLACK,
	DARK_GRAY,
	PASTEL_GRAY,
	PASTEL_GREEN,
	SEMIBOLD_FONT,
	TRANSPARENT,
	WHITE,
	greenBodySkin,
	greenHeaderSkin,
} from "shell/assets";

import { 
	LineBehavior,
} from "shell/behaviors";

import {
	greenTileSkin, 
	tileStyle,
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

import {
	HelperBehavior,
} from "features/devices/behaviors";

import {
	PreferencesTable,
	ToggleLine,
	ToggleTable,
} from "shell/preferences";

const appTitle = "Blink Light";

const bodySkin = new Skin({ fill:WHITE, stroke:PASTEL_GREEN, borders:{ left:1, right:1, top:1, bottom:1 } });

const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:80, height:60, aspect:"fit" });

// BEHAVIORS

export function BlinkLightTest(device) {
	return !device.isSimulator();
}

class BlinkLightViewBehavior extends HelperBehavior {
	onCreate(container, data) {
		this.data = data;
		this.data.cabBlink = false;
		data.config = { interval:false, color:{ red:true, green:true, blue:true } };
		let config = data.config;
		let preferences = {
			items: [
				{
					Template: PreferencesTable,
					expanded: true,
					comment: "LED color",
					name: "BLINK COLOR",
					items: [
						{
							Template: ToggleLine,
							comment: "Add Red component",
							name: "Red",
							get value() {
								return config.color.red;
							},
							set value(it) {
								config.color.red = it ? true : false;
								container.delegate("doBlink");
							},
						},
						{
							Template: ToggleLine,
							comment: "Add Green component",
							name: "Green",
							get value() {
								return config.color.green;
							},
							set value(it) {
								config.color.green = it ? true : false;
								container.delegate("doBlink");
							},
						},
						{
							Template: ToggleLine,
							comment: "Add Blue component",
							name: "Blue",
							get value() {
								return config.color.blue;
							},
							set value(it) {
								config.color.blue = it ? true : false;
								container.delegate("doBlink");
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "BLINK SPEED",
					items: [
						{
							Template: ToggleLine,
							comment: "Blink faster",
							name: "Fast",
							get value() {
								return config.interval;
							},
							set value(it) {
								config.interval = it ? true : false;
								container.delegate("doBlink");
							},
						},
					],
				},
			],
		};
		data.PREFERENCES.insert(new BlinkLightPreferences(preferences), data.PREFERENCES.first);
	}
	doBlink(container) {
		if (this.data.cabBlink)
			this.data.device.blinkLight(this.data.config);
	}
	onDeviceHelperUp(container) {
		super.onDeviceHelperUp(container);
		this.data.cabBlink = true;
		this.doBlink(container, this.data.color);
	}
	onUndisplayed(container) {
		let device = this.data.device;
		this.data.device.blinkLight();
	}
};

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalCenterScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	AppViewHeader,
} from "features/devices/viewer";

export const BlinkLightView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:bodySkin, clip:true,
	Behavior: BlinkLightViewBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:60, bottom:0, //skin:grayBorderSkin,
			contents: [
				Scroller($, {
					left:1, right:1, top:0, bottom:1, clip:true, active:true, anchor:"PREFERENCES",
					Behavior:ScrollerBehavior,
					contents: [
						HorizontalCenterScrollbar($, { left:-1, right:-1, bottom:-10 }),
						VerticalScrollbar($, { right:-10 }),
					],
				}),
			],
		}),
		AppViewHeader({ skin:greenHeaderSkin, title:appTitle, device:$.device }),
	],
}));

export const BlinkLightTile = Container.template($ => ({
	left:0, top:0, height:iconSkin.height+50, skin:greenTileSkin, style:tileStyle,
	Behavior: TileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

const BlinkLightPreferences = Column.template($ => ({
	left:0, right:0, top:0,
	contents: [
		$.items.map(item => new item.Template(item)),
	],
}));

