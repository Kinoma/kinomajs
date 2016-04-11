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

class PinExplorerViewBehavior extends HelperBehavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDeviceHelperUp(container) {
		super.onDeviceHelperUp(container);
		let device = this.data.device;
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
	onPinExplorerLaunched(container, url) {
		let data = this.data;
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
			trace(JSON.stringify(configuration) + "\n");
			let data = this.data;
			let probes = PinManager.createProbes(data.device, data.pins, configuration);
			data.probes = probes;
			data.PROBES.delegate("onUpdate");
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
		Container($, {
			left:1, right:1, top:60, bottom:1,
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
									let probes = data.probes;
									column.empty();
									if (probes) {
										probes.forEach(probe => {
											if (probe.isFront) 
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
