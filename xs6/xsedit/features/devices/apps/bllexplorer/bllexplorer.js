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
	ProbeBehavior,
} from "probeManager";

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

const appTitle = "BLL Explorer";
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:78, height:56, aspect:"fit" });

// BEHAVIORS

class BLLExplorerViewBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplayed(container) {
		this.data.discovery = new Pins.discover(service => this.onPinsFound(service), service => this.onPinsLost(service));
	}
	onUndisplayed(container) {
		data = this.data;
		if (data.discovery)
			data.discovery.close();
		if (data.pins)
			data.pins.close();
	}
	onConnection(status) {
		if (status == "closed") {
			this.doClose();
		}
	}
	onMetadata(bll, metadata) {
		if (!this.data.PROBES.container) return;
		this.bllLength--;
		let data = this.data;
		let configuration = data.configuration;
		for (let i in configuration) {
			let probe = configuration[i];
			let require = probe.require;
			if (probe.require == bll) {
				probe.metadata = metadata;
			}
		}
		if (!this.bllLength) {
			// clean empty metadata
			data.PROBES.delegate("onUpdate");
		}
	}
	onConfiguration(configuration) {
		if (!this.data.PROBES.container) return;
		let blls = [];
		let data = this.data;
		data.configuration = configuration;
		for (let i in configuration) {
			let probe = configuration[i];
			let require = probe.require;
			if (!blls.find(item => item == require))
				blls.push(require);
		}
		data.blls = blls;
		this.bllLength = blls.length;
		for (let bll of blls) {
			data.pins.invoke("metadata/" + bll, metadata => this.onMetadata(bll, metadata));
		}
	}
	onPinsFound(service) {
		let data = this.data;
		if (data.device.ip == service.ip) {
			data.service = service;
			data.pins = Pins.connect(service);
 			data.pins.invoke("configuration", configuration => this.onConfiguration(configuration));
		}
	}
	onPinsLost(service) {
		let data = this.data;
		if (data.service && (data.service.name == service.name)) {
			data.service = undefined;
			data.pins.close();
			data.pins = undefined;
			data.configuration = undefined;
			data.PROBES.delegate("onUpdate");
		}
	}
};

// TEMPLATES

import {
	ScrollerBehavior,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	SpinnerContent,
	ValueBehavior,
} from "features/devices/behaviors";

import {
	AppViewHeader,
} from "features/devices/viewer";

export const BLLExplorerTile = Container.template($ => ({
	left:0, top:0, height:iconSkin.height+50, skin:greenTileSkin, style:tileStyle,
	Behavior: TileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

export const BLLExplorerView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:greenBodySkin,
	contents: [
		AppViewHeader({ skin:greenHeaderSkin, title:appTitle, device:$.device }),
		Container($, {
			left:1, right:1, top:60, bottom:1,
			contents: [
				Container($, {
					left:1, right:1, top:0, bottom:1,
					Behavior: BLLExplorerViewBehavior,
					contents:[
						SpinnerContent($, { anchor:"SPINNER" }),
						Scroller($, {
							left:0, right:0, top:0, bottom:0, active:true, clip:true,
							Behavior:ScrollerBehavior,
							contents: [
								Column($, {
									left:0, right:0, top:0, anchor:"PROBES",
									Behavior: class extends ValueBehavior {
										onUpdate(column) {
											let data = this.data;
											let configuration = data.configuration;
											column.empty();
											if (configuration) {
												data.SPINNER.visible = false;
												data.SPINNER.stop();
												for (let i in configuration) {
													let probe = ProbeBehavior.createProbe(data.pins, i, configuration[i]);
													if (probe)
														column.add(probe);
												}
											}
											else {
												data.SPINNER.visible = true;
												data.SPINNER.start();
											}
											column.container.adjust();
										}
									},
								}),
								VerticalScrollbar($, { right:-10 }),
							],
						}),
					],
				}),
			]
		}),
	],
}));
