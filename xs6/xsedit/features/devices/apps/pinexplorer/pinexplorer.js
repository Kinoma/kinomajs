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
import {
	PinManager,
} from "pinManager";

// ASSETS

import {
	greenBodySkin,
	greenHeaderSkin,
} from "shell/assets";

import {
	greenTileSkin, 
	tileStyle,
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

import {
	whiteSkin
} from "features/devices/assets";

import {
	FilterButtonLine,
} from "controls";

const appTitle = "Pin Explorer";
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:74, height:56, aspect:"fit" });

// BEHAVIORS

import {
	ScrollerBehavior,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	HelperBehavior,
} from "features/devices/behaviors";

export var gPinFilterInfo = new Map();

function mapToJson(map) {
	return JSON.stringify([...map]);
}
function jsonToMap(jsonStr) {
	return new Map(JSON.parse(jsonStr));
}

class PinExplorerViewBehavior extends HelperBehavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDeviceHelperUp(container) {
		super.onDeviceHelperUp(container);
		let device = this.data.device;
		let filteringInfo = device.getPinFilteringInfo();
		gPinFilterInfo.set(device, filteringInfo);
		device.pinExplorerStart(container).then(url => {
			this.onPinExplorerLaunched(container, url);
		}, error => {
			debugger
		});
	}
	onUndisplayed(container) {
		let device = this.data.device;
		device.pinExplorerStop(container);
	}
	mapToJson(map) {
		return JSON.stringify([...map]);
	}
	onPinExplorerLaunched(container, url) {
		let device = this.data.device;
		let tag = this.data.device.constructor.tag;
		if (tag == "Element" || tag == "ElementShell") {		// avoid requiring Element to implement getPhysicalToLogicalMap
			let map = new Map();
			for (var i=1; i <= 16; i++)
				map.set(i, i);
			let jsonMap = this.mapToJson(map);
			this.onGotLogicalToPhysicalMap(container, jsonMap, url);
			return;
		}
		
		device.getLogicalToPhysicalMapJson().then(map => {
			this.onGotLogicalToPhysicalMap(container, map, url);
		});
	}
	onGotLogicalToPhysicalMap(container, map, url) {
		let data = this.data;
		let logicalToPhysicalMap = undefined;
		if (map.length != 0)										// special casing for Pins.getLogicalToPhysicalMapJson not being present
			logicalToPhysicalMap = jsonToMap(map);
		data.device.logicalToPhysicalMap = logicalToPhysicalMap;
		
		data.SPINNER.visible = false;
		data.SPINNER.stop();
		data.url = url;
		let pins = this.data.pins = Pins.connect(url);
		pins.invoke("configuration", configuration => this.onConfiguration(configuration))
	}
	onConfiguration(configuration) {	
		if (!this.data.PROBES.container) return;
		if (!configuration) {
			var pins = this.data.pins;
			pins.invoke("configuration", configuration => this.onConfiguration(configuration))
		}
		else {
			let data = this.data;
			data.FILTER_BUTTON_LINE.delegate("appendDeviceSpecificButtons");
			data.FILTER_BUTTON_LINE.delegate("appendAllNoneButtons");
			let probes = PinManager.createProbes(data.device, data.pins, configuration);
			data.probes = probes;
			data.PROBES.delegate("onUpdate");	
			trace(JSON.stringify(configuration) + "\n");
		}
	}
};

// TEMPLATES

import {
	SpinnerContent,
	ValueBehavior,
} from "features/devices/behaviors";

import {
	AppViewHeader,
} from "features/devices/viewer";

export const PinExplorerTile = Container.template($ => ({
	left:0, top:0, height:iconSkin.height+50, skin:greenTileSkin, style:tileStyle,
	Behavior: TileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

export const PinExplorerView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:greenBodySkin,
	Behavior: PinExplorerViewBehavior,
	contents:[
		AppViewHeader({ skin:greenHeaderSkin, title:appTitle, device:$.device }),
		FilterButtonLine($, { left:1, right:1, top:60, height:40, skin:whiteSkin, anchor:"FILTER_BUTTON_LINE"}),
		Container($, {
			left:1, right:1, top:60 + 40, bottom:1,
			contents: [
				SpinnerContent($, { anchor:"SPINNER" }),
				Scroller($, {
					left:0, right:0, top:0, bottom:0, active:true, clip:true,
					Behavior:ScrollerBehavior,
					contents: [
						Column($, {
							left:0, right:0, top:10, anchor:"PROBES",
							Behavior: class extends ValueBehavior {
								onUpdate(column) {
									let data = this.data;
									let filteredProbes = this.data.FILTER_BUTTON_LINE.delegate("filterProbes", data.probes);
									column.empty();
									if (filteredProbes) {
										filteredProbes.forEach(probe => {
											column.add(new probe.template(probe));
										});
									}
									column.container.adjust();
								}
							},
						}),
						VerticalScrollbar($, { right:-10 }),
					],
				}),
			]
		}),
	],
}));
