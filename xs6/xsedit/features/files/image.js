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
	GREEN,
	PASTEL_GRAY,
	PASTEL_GREEN,
	PASTEL_ORANGE,
	ORANGE,
	WHITE,
	grayBodySkin,
	grayBorderSkin,
	grayHeaderSkin,
	tableHeaderStyle,
	whiteButtonsSkin,
} from "shell/assets";

var imageBackgroundTexture = new Texture("assets/imageBackground.png", 1);
var imageBackgroundSkin = new Skin({ texture:imageBackgroundTexture, x:0, y:0, width:16, height:16, tiles:{ left:0, right:0, top:0, bottom:0 } });

// BEHAVIORS

import { 
	ButtonBehavior, 
} from "common/control";

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

// TEMPLATES

export var ImageView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	contents: [
		Scroller($, {
			left:0, right:10, top:30, bottom:10, skin:grayBodySkin, active:true, Behavior:ScrollerBehavior,
			contents: [
				Container($, { 
					skin:imageBackgroundSkin,
					contents: [
						Picture($, { url:$.url, aspect:"draw" }),
					],
				}),
				HorizontalScrollbar($, { left:0, bottom:-10 }),
				VerticalScrollbar($, { right:-10, top:-30 }),
			],
		}),
		Line($, {
			left:0, right:10, top:0, height:30, skin:grayHeaderSkin,
			contents: [
				Content($, { width:10 }),
				Label($, { left:0, right:0, style:tableHeaderStyle, string:Files.toPath($.url) }),
				Content($, { 
					width:30, height:30, skin:whiteButtonsSkin, variant:0, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("doClose");
						}
					},
				}),
			],
		}),
	],
}));
