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
	GRAY,
	GREEN,
	ORANGE,
	WHITE,
	DARK_GRAY,
	LIGHT_GRAY,
	PASTEL_GRAY,
} from "shell/assets";

export const iconSize = 45;
export const headerSize = 30;

export const BLUE = "blue";
export const WARM_GRAY = "#868686";

export const blackSkin = new Skin({ fill:BLACK });
export const greenSkin = new Skin({ fill:GREEN });
export const lightGraySkin = new Skin({ fill:LIGHT_GRAY });
export const pastelGraySkin = new Skin({ fill:PASTEL_GRAY });
export const warmGraySkin = new Skin({ fill:WARM_GRAY });
export const whiteSkin = new Skin({ fill:WHITE });

export const blackStyle = new Style({ color:BLACK });
export const centerStyle = new Style({ horizontal:"center" });
export const defaultStyle = new Style({ font:"Open Sans", size:18, color:BLACK, left:10, right:10 });
export const leftStyle = new Style({ horizontal:"left", color:BLACK });
export const lightGrayStyle = new Style({ fill:PASTEL_GRAY });
export const rightStyle = new Style({ horizontal:"right", color:BLACK });
export const smallStyle = new Style({ size:14 });
export const whiteStyle = new Style({ color:WHITE });
export const headerTitleStyle = new Style({ font: "Open Sans Bold", size:14, color:WHITE, horizontal:"left", left:10 })
export const headerSubtitleStyle = new Style({ font: "Open Sans Semibold", size:12, color:WHITE, horizontal:"left", left:10 })

export const navigationSkin = new Skin({ texture:new Texture("./assets/navigation.png", 1), x:0, y:0, width:60, height:60, variants:60, states:60, aspect:"fit" });

export const BLL_EXPLORER = "BLL Explorer",
export const FILES = "Files",
export const LOGS = "Loga",
export const PIN_EXPLORER = "Pin Explorer",
export const FRONT_PINS = "Front Pins",
export const SETTINGS = "Settings",
export const WIFI = "WiFi",

var strings = {
	"KINOMA_DEVICE_VIEWER":"Kinoma Device Viewer",
	"NO_DEVICE":"No Device",
	"SELECT":"Select",
	"SELECT_DEVICE":"Select Device",
	
	"bllexplorer":"BLL Explorer",
	"files":"Files",
	"logs":"Logs",
	"pinexplorer":"Pin Explorer",
	"frontpins":"Front Pins",
	"settings":"Settings",
	"wifi":"Wifi",

	"DEVICE_NAME":"Device Name",
	"TIMEZONE":"Timezone",
	"STARTUP_APP":"Startup App",
	"DEBUGGING":"Debugging",
	"UPDATE_APPLICATION":"Kinoma Software",
	"UPDATE_FIRMWARE":"Operating System",
	"CLEAR_CACHES":"Clear Caches",
	"MAC_ADDRESS":"MAC Address",
	"ABOUT":"About",
	
	"UPDATE":"Update",
};
export function localize(id) {
	if (id in strings)
		return strings[id];
	return id;
}

export var fieldHintStyle = new Style({ font: "Open Sans", size:14, color:GRAY, horizontal:"left", left:7 });
export var fieldLabelSkin = new Skin({ fill: [ "transparent","transparent","#e0e0e0", "#cbe1fa" ] });
export var fieldScrollerSkin = new Skin({ fill: [ "white","white" ] });
export const fieldLabelStyle = new Style({ font: "Open Sans", size:14, color:BLACK, horizontal:"right", left:7 })

const wifiTexture = new Texture("./assets/wifi.png", 2);
export const deviceDebugSkin = new Skin({ texture:wifiTexture, x:0, y:0, width:20, height:20, states:20 });
export const deviceAddressSkin = new Skin({ texture:wifiTexture, x:20, y:0, width:20, height:20, states:20, variants:20 });






