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

const appTitle = "Wi-Fi";
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:66, height:60, variants:66, states:60, aspect:"fit" });
const secureSkin = new Skin({ texture: new Texture("./assets/lock.png"), x:0, y:0, width:20, height:20, variants: 20 });
const wifiSkin = new Skin({ texture:new Texture("./assets/wifi.png", 1), x:0, y:0, width:33, height:30, variants:33, states:30, aspect:"fit" });

const networkLineSkin = new Skin({ fill:[WHITE, PASTEL_GREEN, PASTEL_GREEN, PASTEL_GREEN], stroke:PASTEL_GREEN, borders:{ bottom:1 } });
const networkSSIDStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left", left:10 });
const lookingForNetworksStyle = new Style({ font:NORMAL_FONT, size:14, color:GRAY, horizontal:"left", left:10 });
const passwordStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left", left:10 });
const noPasswordStyle = new Style({ font:NORMAL_FONT, size:14, color:GRAY, horizontal:"left", left:10 });

const headerTitleStyle = new Style({ font:BOLD_FONT, size:14, color:WHITE, horizontal:"left", left:10 });
const headerSubtitleStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"left", left:10 });

const categoryStyle =  new Style({ font:NORMAL_FONT, size:14, color:DARKER_GRAY, horizontal:"left" });

// BEHAVIORS

import { 
	ButtonBehavior, 
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

import { 
	DropDialog,
} from "common/menu";

import {
	LineBehavior,
} from "shell/behaviors";

import {
	ScrollerBehavior,
} from "common/scrollbar";

import {
	HelperBehavior,
	WaitContent,
} from "features/devices/behaviors";

import {
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

import {
	AppViewHeader,
} from "features/devices/viewer";

class WifiTileBehavior extends TileBehavior {
	computeStrength(value) {
		let low = -120;
		let high = -40;
		if (value < low) value = low;
		if (value > high) value = high;
		return Math.round(4 * ((value - low) / (high - low)));
	}
	onDisplaying(container) {
		this.onFinished(container);
		super.onDisplaying(container);
		let data = this.data;
		data.device.getNetworkSSID().then(ssid => {
			container.first.next.string = ssid;
		});
	}
	onFinished(container) {
		let data = this.data;
		data.device.getNetworkLevel().then(level => {
			if (level) {
				let icon = data.STRENGTH;
				icon.variant = this.computeStrength(level);
			}
			container.time = 0;
			container.start();
		});
	}
}

class WifiViewBehavior extends HelperBehavior {
	computeStrength(value) {
		let low = -120;
		let high = -40;
		if (value < low) value = low;
		if (value > high) value = high;
		return Math.round(4 * ((value - low) / (high - low)));
	}
	doRefresh(container, refreshing) {
		refreshing ? container.start() : container.stop();
	}
	doUpdate(container) {
		this.data.device.getNetworkList().then(json => {
			this.doUpdateList(container, json);
		});
	}
	doUpdateList(container, json) {
		let duration = 1000;
		if (json && this.refreshing) {
			let data = this.data;
			let device = data.device;
			let currentNetwork = undefined;
			let existing = {};
			let networks = data.networks = json.filter(function(network) {
				let ssid = network.ssid;
				return (ssid == "") || existing.hasOwnProperty(ssid) ? false : (existing[ssid] = true);
			});
			networks.sort(function(a, b) {
				a = a.ssid.toLowerCase();
				b = b.ssid.toLowerCase();
				if (a < b)
					return -1;
				else if (a > b)
					return 1;
				return 0;
			});
			var currentSSID = model.SSID;
			for (let network of networks) {
				if ("flags" in network) {
					if (network.flags.indexOf("WPA") >= 0)
						network.security = "wpa";
					else
						network.security = "none";
				}
				network.secured = network.security.indexOf("none") == -1	
				if ("signal_level" in network)
					network.strength = this.computeStrength(network.signal_level);
				else
					network.strength = 4;
				if (network.ssid == currentSSID)
					currentNetwork = network;
			}
			if (currentNetwork) {
				networks.splice(networks.indexOf(currentNetwork), 1);
				data.currentNetwork = currentNetwork;
			}
			duration = (networks.length > 0) ? 10000 : 1000;
			this.onNetworksChanged(container);
		}
		container.duration = duration;
		container.start();
	}
	onCreate(container, $) {
		this.data = $;
		this.data.currentNetwork = null;
		this.data.networks = [];
		this.data.device.configuring = false;
		this.refreshing = true;
	}
	onDisplaying(container) {
		var json = this.data.device.network;
		if (json) {
			this.data.currentNetwork = {
				secured: false,
				ssid: json.ssid,
				strength: this.computeStrength(json.level),
			};
			this.onNetworksChanged(container);
		}
	}
	onDeviceHelperUp(container) {
		super.onDeviceHelperUp(container);
		let data = this.data;
		let spinner = data.SPINNER;
		spinner.visible = false;
		spinner.stop();
		let scroller = new WifiScroller(this.data);
		container.insert(scroller, spinner);
		this.data.device.networkScan(true);
		this.doUpdate(container);
	}
	onFinished(container) {
		container.time = 0;
		this.doUpdate(container);
	}
	onNetworksChanged(container) {
		let data = this.data;
		let device = data.device;
		let column = container.first.first;
		column.empty(0);
		if (data.currentNetwork) {
			column.add(new CategorySeparator("Current Network"));
			column.add(new CurrentNetworkLine({ device, network:data.currentNetwork }));
		}
		column.add(new CategorySeparator("Available Networks"));
		data.networks.forEach(network => {
			column.add(new NetworkLine({ device, network }));
		});
		column.add(new LookingForNetworksLine());
		column.add(new Separator());
	}
	onUndisplayed(container) {
		container.stop();
		if (!this.data.device.configuring)
			this.data.device.networkScan(false);
		delete this.data.device.configuring;
		delete this.data.networks;
		delete this.data.currentNetwork;
	}
};

class NetworkLineBehavior extends LineBehavior {
	onTap(line) {
		let data = this.data;
		let description = {
			Template:NetworkSheet,
			button:line,
			device:data.device,
			network:data.network,
			variant:1,
		};
		this.dialog = new DropDialog(description);
		shell.add(this.dialog);
		line.bubble("doRefresh", false);
	}
	onDialogClosed(line, item) {
		this.dialog = null;
		shell.focus();
		line.bubble("doRefresh", true);
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
}

class NetworkSheetBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		let data = this.data;
		if (data.network.secured)
			data.FIELD.focus();
	}
	onEnter(sheet) {
		let data = this.data;
		let device = data.device;
		let network = data.network;
		let query = {
			ssid: network.ssid,
			security: network.security,
			adhoc: false,
			hidden: false,
		}
		if (device.constructor.tag == "Element") {
			query.security = network.security;
			query.save = true;
			if (network.secured) {
				let path = mergeURI(shell.url, "../../certs/element.crt");
				// let cert = Files.readText(mergeURI(shell.url, "../../certs/element.cert.pem"));
				let cert = Files.readChunk(path);
				let key = Crypt.x509.decode(cert).getKey();
				let pk = new Crypt.PKCS1_5(key.rsaKey);
				let passwd = data.FIELD.string;
				let length = passwd.length;
				let c = new Chunk(length);
				for (let i = 0; i < length; i++)
					c.poke(i, passwd.charCodeAt(i));
				let e = pk.encrypt(c);
				query.encryptedPassword = e.toString();
			}
		}
		else {
			query.authentication = network.security.toUpperCase()
			if (network.secured)
				query.password = data.FIELD.string;
			else
				query.password = "";
		}
		device.configuring = true;
		device.networkScan(false);
		device.networkConfigure(query);
	}
}

// TEMPLATES

import {
	SpinnerContent,
} from "features/devices/behaviors";

export function WifiTest(device) {
	return !device.isSimulator();
}

export const WifiTile = Container.template($ => ({
	left:0, top:0, height:130, skin:greenTileSkin, style:tileStyle, duration: 5000,
	Behavior: WifiTileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin, variant:4, anchor:"STRENGTH" }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:model.SSID}),
		Label($, { left:0, right:0, top:iconSkin.height+40, style:infoStyle, string:$.device.ip }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

export const WifiView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:bodySkin,
	Behavior: WifiViewBehavior,
	contents: [
		SpinnerContent($, { anchor:"SPINNER" }),
		AppViewHeader({ skin:greenHeaderSkin, title:appTitle, device:$.device }),
	],
}));

var WifiScroller = Scroller.template($ => ({
	left:1, right:1, top:60, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior,
	contents: [
		Column($, {
			width:640, top:0,
			contents: [
				CategorySeparator("Available Networks", {}),
				LookingForNetworksLine($, {}),
				Separator($, {}),
			],
		}),
	],
}));

var CurrentNetworkLine = Line.template($ => ({
	left:0, right:0, height:40, skin: networkLineSkin, active:false,
	Behavior: NetworkLineBehavior,
	contents: [
		Container($, {
			width:40, height:40,
			contents: [
				Content($, { right:-5, bottom:0, skin:secureSkin, visible:$.network.secured, variant:1 }),
				Content($, { top:5, skin:wifiSkin, visible:true, state:1, variant:$.network.strength }),
			],
		}),
		Label($, { style:networkSSIDStyle, string:$.network.ssid }),
	]
}));

var LookingForNetworksLine = Line.template($ => ({ 
	left:0, right:0, height:40, skin: whiteSkin,
	contents: [
		Container($, {
			width:40, height:40,
			contents: [
				WaitContent($, {}),
			],
		}),
		Label($, { left:0, right:0, height:30, style:lookingForNetworksStyle, string:"Looking for networks...", }),
	]
}));

var NetworkLine = Line.template($ => ({
	left:0, right:0, height:40, skin: networkLineSkin, active:true,
	Behavior: NetworkLineBehavior,
	contents: [
		Container($, {
			width:40, height:40,
			contents: [
				Content($, { right:-5, bottom:0, skin:secureSkin, visible:$.network.secured, variant:1 }),
				Content($, { top:5, skin:wifiSkin, visible:true, state:1, variant:$.network.strength }),
			],
		}),
		Label($, { style:networkSSIDStyle, string:$.network.ssid }),
	]
}));

var Separator = Content.template($ => ({ left:0, right:0, height:10 }));
var CategorySeparator = Line.template($ => ({
	left:0, right:0, height:30,
	contents: [
		Label($, { style:categoryStyle, string:$ }),
	],
}));

var NetworkSheet = Column.template($ => ({
	left:0, right:0, height:80, active:true,
	Behavior: NetworkSheetBehavior,
	contents: [
		Line($, {
			left:0, right:0, height:40,
			contents: [
				Container($, {
					width:40, height:40,
					contents: [
						Content($, { right:-5, bottom:0, skin:secureSkin, visible:$.network.secured, variant:1 }),
						Content($, { top:5, skin:wifiSkin, visible:true, state:1, variant:$.network.strength }),
					],
				}),
				Label($, { style:networkSSIDStyle, string:$.network.ssid }),
			]
		}),
		Line($, {
			left:0, right:0, height:40,
			contents: [
				$.network.secured ? PasswordLine($, {}) : NoPasswordLine($, {}),
				Container($, {
					width:80, skin:whiteButtonSkin, active:true, name:"onClose", Behavior: ButtonBehavior,
					contents: [
						Label($, { left:0, right:0, style:whiteButtonStyle, string:"Cancel" }),
					],
				}),
				Container($, {
					width:80, skin:greenButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
					contents: [
						Label($, { left:0, right:0, style:greenButtonStyle, string:"Join" }),
					],
				}),
				Content($, { width:3 }),
			],
		})
	],
}));

var PasswordLine = Line.template($ => ({
	left:0, right:0, height:40,
	contents: [
		Label($, { width:80, style:passwordStyle, string:"Password" }),
		Scroller($, {
			left:3, right:3, top:5, bottom:5, skin: fieldScrollerSkin, clip:true, active:true,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					anchor:"FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:fieldLabelStyle, editable:true, hidden:true,
					Behavior: FieldLabelBehavior,
				}),
			],
		}),
	]
}));

var NoPasswordLine = Line.template($ => ({
	left:0, right:0, height:40,
	contents: [
		Label($, { left:0, right:0, style:noPasswordStyle, string:"No password" }),
	]
}));
