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
	model,
} from "shell/main";

import {
	greenButtonSkin,
	greenButtonStyle,
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";

import {
	BOLD_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BLACK,
	DARKER_GRAY,
	GRAY,
	PASTEL_GRAY,
	PASTEL_GREEN,
	WHITE,
	grayBorderSkin,
	greenHeaderSkin,
} from "shell/assets";

import {
	fieldLabelSkin,
	fieldLabelStyle,
	fieldScrollerSkin,
	whiteSkin,
} from "features/devices/assets";

import {
	greenTileSkin,
	infoStyle,
	tileStyle,
} from "features/devices/tiles";

const bodySkin = new Skin({ fill:WHITE, stroke:PASTEL_GREEN, borders:{ left:1, right:1, top:1, bottom:1 } });

const appTitle = "Build Config";
const iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60 });

// BEHAVIORS

import {
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

import {
	AppViewHeader,
} from "features/devices/viewer";

import {
	LocateDirectoryLine,
	PreferencesTable,
	ToggleLine,
} from "shell/preferences";

class AndroidTileBehavior extends TileBehavior {
}

class AndroidViewBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		let device = data.device;
		let options = device.options
		let output = device.output
		let preferences = {
			items: [
				{
					Template: PreferencesTable,
					expanded: true,
					name: "BUILD OPTIONS",
					items: [
						{
							Template: ToggleLine,
							comment: "Build a debug version",
							name: "Debug",
							get value() {
								return options.debug;
							},
							set value(it) {
								options.debug = it ? true : false;
							},
						},
						{
							Template: ToggleLine,
							comment: "Enable XS debugging",
							name: "XS Debug",
							get value() {
								return options.xs;
							},
							set value(it) {
								options.xs = it ? true : false;
							},
						},
						{
							Template: ToggleLine,
							comment: "Enable instrumentation",
							name: "Instrumentation",
							get value() {
								return options.instrumentation;
							},
							set value(it) {
								options.instrumentation = it ? true : false;
							},
						},
						{
							Template: ToggleLine,
							comment: "Debug memory leaks",
							name: "Memory",
							get value() {
								return options.memory;
							},
							set value(it) {
								options.memory = it ? true : false;
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: false,
					name: "OUTPUT OPTIONS",
					items: [
						{
							Template: LocateDirectoryLine,
							event: "onProjectsDirectoryChanged",
							message: "Locate the Output Folder",
							name: "Output Folder",
							get value() {
								return output.directory;
							},
							set value(it) {
								output.directory = it;
							},
						},
					],
				},
			],
		};
		data.PREFERENCES.insert(new AndroidPreferences(preferences), data.PREFERENCES.first);
	}
	onDeviceSelected(container, device) {
		if (this.data.device != device) {
			trace("onDeviceSelected -> doCLose\n");
			container.bubble("doClose");
		}
	}
};

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalCenterScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	SpinnerContent,
} from "features/devices/behaviors";

export const AndroidTile = Container.template($ => ({
	left:0, top:0, height:105, skin:greenTileSkin, style:tileStyle,
	Behavior: AndroidTileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle}),
		Container($, { left:0, right:0, top:0, bottom:0 }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

export const AndroidView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:bodySkin,
	Behavior: AndroidViewBehavior,
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

const AndroidPreferences = Column.template($ => ({
	left:0, right:0, top:0,
	contents: [
		$.items.map(item => new item.Template(item)),
	],
}));


