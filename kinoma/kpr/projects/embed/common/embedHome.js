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
	model,
} from "embedShell";

var locale = require("locale");

export const blackSkin = new Skin({ fill:"black" });
export const graySkin = new Skin({ fill:"gray" });
export const kinomaSkin = new Skin({ fill:"#75b602" });
export const whiteSkin = new Skin({ fill:"white" });

const homeStyle = new Style({ font:"Fira Sans", size:20, color:"black" });
const homeTitleStyle = new Style({ vertical:"bottom" });

const homeAboutSkin = new Skin({ texture:new Texture("./assets/about.png"), x:0, y:0, width:460, height:349, aspect:"fit" });
const homeWiFiSkin = new Skin({ texture:new Texture("./assets/wifi.png"), x:0, y:0, width:460, height:349, aspect:"fit" });
const homeWiFiSkins = [
	new Skin({ texture:new Texture("./assets/wifi0.png"), x:0, y:0, width:460, height:349, aspect:"fit" }),
	new Skin({ texture:new Texture("./assets/wifi1.png"), x:0, y:0, width:460, height:349, aspect:"fit" }),
	new Skin({ texture:new Texture("./assets/wifi2.png"), x:0, y:0, width:460, height:349, aspect:"fit" }),
	new Skin({ texture:new Texture("./assets/wifi3.png"), x:0, y:0, width:460, height:349, aspect:"fit" }),
	new Skin({ texture:new Texture("./assets/wifi4.png"), x:0, y:0, width:460, height:349, aspect:"fit" }),
];

//const homeFooterSkin = new Skin({ fill:"white", stroke:"gray", borders:{ top:1 } });
//const homeFooterStyle = new Style({ color:"gray" });

const homeFooterSkin = new Skin({ fill:"#75b602" });
const homeFooterBarSkin = new Skin({ fill:blendColors(0.20, "#75b602", "black") });
const homeFooterStyle = new Style({ color:"white" });

export var HomeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin, style:homeStyle, active:true, clip:true,
	Behavior: class extends Behavior {
		onCreate(container) {
			container.duration = 500;
		}
		onDisplaying(container) {
			let width = container.width;
			let height = container.height;
			let size = Math.sqrt(width**2 + height**2);
			let $ = width > height ? {
				w2: Math.round(width / 2),
				w3: Math.round(width / 3),
				h2: Math.round(height / 2),
				h8: Math.round(height / 8),
				h12: Math.round(height / 12),
				h24: Math.round(height / 24),
			} : {
				w2: Math.round(height / 2),
				w3: Math.round(height / 3),
				h2: Math.round(width / 2),
				h8: Math.round(width / 8),
				h12: Math.round(width / 12),
				h24: Math.round(width / 24),
			};
			container.style.size = $.h12;
			container.empty();
			container.add(new HomeAboutColumn($));
			container.add(new HomeDeviceColumn($));
			container.add(new HomeWiFiColumn($));
			container.add(new HomeFooter($));
		}
		onTimeChanged(container) {
			let about = container.first;
			let device = about.next;
			let wifi = device.next;
			let footer = wifi.next;
			let fraction = this.reverse ? 1 - Math.quadEaseIn(container.fraction) : Math.quadEaseOut(container.fraction);
			about.x = container.x - Math.round(fraction * about.width);
			device.y = container.y - Math.round(fraction * device.height);
			wifi.x = container.x + container.width + Math.round((fraction - 1) * wifi.width);
			footer.y = container.y + container.height + Math.round((fraction - 1) * footer.height);
		}
		onTouchBegan(container) {
			this.reverse = false;
			container.duration = 500;
			container.time = 0;
			container.start();
		}
		onTouchEnded(container) {
			this.reverse = true;
			container.duration = 500;
			container.time = 500 - container.time;
			container.start();
		}
	},
}));

var HomeAboutColumn = Container.template($ => ({
	left:0, width:$.w3, height:$.h2, bottom:$.h8 + $.h24,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:2 * $.h12, skin:homeAboutSkin }),
		Label($, { left:0, right:0, height:$.h12, bottom:$.h12, style:homeTitleStyle, string:getEnvironmentVariable("CORE_VERSION") }),
	],
}));

var HomeDeviceColumn = Container.template($ => ({
	width:$.w2, top:$.h24, height:$.h2,
	Behavior: class extends Behavior {
		onCreate(column, data) {
			this.onDeviceChanged(column);
			this.onNameChanged(column);
		}
		onDeviceChanged(column) {
			column.first.url = "xkpr://shell/description/picture";
			column.first.next.string = system.device;
		}
		onNameChanged(column) {
			column.last.string = model.name;
		}
	},
	contents: [
		Picture($, { left:0, right:0, top:0, bottom:2 * $.h12, }),
		Label($, { left:$.h12, right:$.h12, height:$.h12, bottom:$.h12, style:homeTitleStyle }),
		Label($, { left:$.h12, right:$.h12, height:$.h12, bottom:0, style:homeTitleStyle }),
	],
}));

var HomeWiFiColumn = Container.template($ => ({
	width:$.w3, right:0, height:$.h2, bottom:$.h8 + $.h24,
	Behavior: class extends Behavior {
		onCreate(column, data) {
			this.onNetworkChanged(column, model.network);
		}
		onNetworkChanged(column, network) {
			let level = column.first;
			let ssid = level.next;
			let ip = ssid.next;
			if (network.active) {
				let low = -120;
				let high = -40;
				let value = network.signal_level;
				if (value < low)
					value = low;
				if (value > high)
					value = high;
				value = Math.round(4 * ((value - low) / (high - low)));
				level.skin = homeWiFiSkins[value];
			}
			else
				level.skin = homeWiFiSkin;
			ssid.string = network.ssid;
			ip.string = network.ip_address;
		}
	},
	contents: [
		Content($, { left:0, right:0, top:0, bottom:2 * $.h12, skin:homeWiFiSkin }),
		Label($, { left:0, right:0, height:$.h12, bottom:$.h12, style:homeTitleStyle }),
		Label($, { left:0, right:0, height:$.h12, bottom:0, style:homeTitleStyle }),
	],
}));

var HomeFooter = Container.template($ => ({
	left:0, right:0, height:$.h8, bottom:0, skin:homeFooterSkin,
	Behavior: class extends Behavior {
		onCreate(container, data) {
			this.size = 0;
			this.url = null;
			this.onTimeChanged(container);
			container.interval = 500;
			container.start();
		}
		onTimeChanged(container) {
			if (this.url) {
				let info = Files.getInfo(this.url);
				let offset = info ? info.size : 0;
				container.first.width = container.width - Math.round(container.width * offset / this.size);
			}
			else {
				container.last.string = locale.formatDateTime(new Date(), undefined, 0, undefined);
			}
		}
		onUploadBegan(container, url, size) {
			let parts = parseURI(url);
			this.size = parseInt(size);
			this.url = url;
			container.first.width = container.width;
			container.last.string = parts.name;
			container.interval = 50;
		}
		onUploadEnded(container, url) {
			this.size = 0;
			this.url = null;
			container.first.width = 0;
			container.last.string = locale.formatDateTime(new Date(), undefined, 0, undefined);
			container.interval = 500;
		}
	},
	contents: [
		Content($, { width:0, right:0, top:0, bottom:0, skin:homeFooterBarSkin }),
		Label($, { style:homeFooterStyle }),
	],
}));

class HomeTransition extends Transition {
	constructor() {
		super(250);
	}
	onBegin(container, home, host) {
		this.home = home;
		this.hostLayer = new Layer({alpha: false});
		this.hostLayer.attach(host);
		this.hostLayer.origin = { x: this.hostLayer.width / 2, y: this.hostLayer.height / 2 };
	}
	onEnd(container, home, host) {
		this.hostLayer.detach();
	}
	onStep(fraction) {
		let home = this.home;
		let about = home.first;
		let device = about.next;
		let wifi = device.next;
		let footer = wifi.next;
		about.x = home.x - Math.round(fraction * about.width);
		device.y = home.y - Math.round(fraction * device.height);
		wifi.x = home.x + home.width + Math.round((fraction - 1) * wifi.width);
		footer.y = home.y + home.height + Math.round((fraction - 1) * footer.height);
		this.hostLayer.scale = { x:0.1 + ((1 - 0.1) * fraction), y:0.1 + ((1 - 0.1) * fraction)};
		this.hostLayer.opacity = fraction;
	}
}

export class LaunchTransition extends HomeTransition {
	constructor() {
		super();
	}
	onBegin(container, home, host) {
		container.add(host);
		host.debugging = true;
		host.launch();
		host.adapt();
		super.onBegin(container, home, host);
	}
	onEnd(container, home, host) {
		super.onEnd(container, home, host);
        container.remove(home);
	}
	onStep(fraction) {
		super.onStep(Math.quadEaseOut(fraction));
	}
}

export class QuitTransition extends HomeTransition {
	constructor() {
		super();
	}
	onBegin(container, home, host) {
		container.insert(home, host);
		host.quitting();
		super.onBegin(container, home, host);
	}
	onEnd(container, home, host) {
		super.onEnd(container, home, host);
		host.quit();
		host.debugging = false;
        container.remove(host);
		shell.purge();
	}
	onStep(fraction) {
		super.onStep(1 - Math.quadEaseIn(fraction));
	}
}



