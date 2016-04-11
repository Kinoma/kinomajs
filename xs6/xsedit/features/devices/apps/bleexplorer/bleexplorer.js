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
	greenBodySkin,
	greenHeaderSkin,
} from "shell/assets";

import {
	greenTileSkin, 
	tileStyle,
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

const appTitle = "BLE Explorer";
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:78, height:56, aspect:"fit" });

// BEHAVIORS

class BLEExplorerViewBehavior extends Behavior {
}

// TEMPLATES

import {
	SpinnerContent,
} from "features/devices/behaviors";

import {
	AppViewHeader,
} from "features/devices/viewer";

export const BLEExplorerTile = Container.template($ => ({
	left:0, top:0, height:iconSkin.height+50, skin:greenTileSkin, style:tileStyle,
	Behavior: TileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

export const BLEExplorerView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:greenBodySkin,
	contents: [
		AppViewHeader({ skin:greenHeaderSkin, title:appTitle, device:$.device }),
		Container($, {
			left:1, right:1, top:60, bottom:1,
			contents: [
				Container($, {
					left:1, right:1, top:0, bottom:1,
					Behavior: BLEExplorerViewBehavior,
					contents:[
						SpinnerContent($, { anchor:"SPINNER" }),
					],
				}),
			]
		}),
	],
}));
