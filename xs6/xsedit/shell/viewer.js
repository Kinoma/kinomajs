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
// MODEL

export class Viewer {
	constructor(feature) {
		this.feature = feature;
		this.Template = NoPane;
	}
	accept(parts) {
		return false;
	}
	create(parts, url) {
		return new this.Template(this.feature.model);
	}
	static fromURI(url) {
		if (url) {
			let parts = parseURI(url);
			let viewer = this.viewers.find(viewer => viewer.accept(parts, url));
			if (viewer) 
				return viewer.create(parts, url);
			return new ErrorView({ url, error:"No viewer found!" });
		}
		return new NoPane();
	}
	static register(viewers) {
		this.viewers = this.viewers.concat(viewers);
	}
};
Viewer.viewers = [];

// ASSETS

import {
	DARKER_RED,
	WHITE,
	tableHeaderStyle,
	greenBorderSkin,
	greenHeaderSkin,
	redBorderSkin,
	redHeaderSkin,
	whiteButtonsSkin,
} from "assets";

export var errorStyle = new Style({ font:"Open Sans", size:16, color:DARKER_RED }),
var noBodySkin = new Skin({ fill:"#f0f0f0" });
var noHeaderStyle = new Style({ font:"Open Sans Semibold", size:32, color:WHITE, horizontal:"left"});
var noTitleStyle = new Style({ font:"Open Sans", size:26, color:"#606060", });
var noCommentStyle = new Style({ font:"Open Sans", size:16, color:"#606060", top:20 });
var iconSkin = new Skin({ texture:new Texture("./assets/kinoma.png", 2), x:0, y:0, width:200, height:60 });

// BEHAVIORS

import { 
	ButtonBehavior, 
} from "common/control";

import {
	ScrollerBehavior,
	HorizontalCenterScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	PathLayout,
} from "features/files/find";

// TEMPLATES

export var CloseButton = Content.template($ => ({
	width:30, height:30, skin:whiteButtonsSkin, variant:0, state:1, active:true, 
	Behavior: class extends ButtonBehavior {
		onTap(button) {
			button.bubble("doClose");
		}
	},
}));

export var ErrorView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:redBorderSkin,
	contents: [
		Scroller($, {
			left:1, right:1, top:30, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				Label($, { style: errorStyle, string:$.error }),
			],
		}),
		Line($, {
			left:0, right:0, top:0, height:30, skin:redHeaderSkin,
			contents: [
				PathLayout($, { left:0, right:0 }),
				CloseButton($, {}),
			],
		}),
	],
}));

var NoPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	contents: [
		Container($, {
			left:0, right:10, top:60, bottom:10, skin:greenBorderSkin, 
			contents: [
				Scroller($, {
					left:1, right:1, top:0, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior,
					contents: [
						Line($, {
							top:0, active:true,
							Behavior: class extends Behavior {
								onTouchEnded(line) {
									launchURI("http://kinoma.com/buy/");
								}
							},
							contents: [
								Container($, {
									left:0, right:0, top:20, bottom:0,
									contents: [
										Column($, { 
											width:240, top:0, 
											contents: [
												Label($, { width:200, height:80, style:noTitleStyle, string:"Kinoma Create" }),
												Picture($, { width:200, height:200, url:"./assets/Create.png" }),
												Text($, { width:200, style:noCommentStyle, string:"Develop with the comprehensive IoT prototyping product" }),
											],
										}),
									],
								}),
								Content($, { width:100 }),
								Container($, {
									left:0, right:0, top:20, bottom:0,
									contents: [
										Column($, { 
											width:240, top:0, 
											contents: [
												Label($, { width:200, height:80, style: noTitleStyle, string:"Kinoma Element" }),
												Picture($, { width:200, height:200, url:"./assets/Element.png" }),
												Text($, { width:200, style:noCommentStyle, string:"The smallest JavaScript-powered embedded prototyping platform" }),
											],
										}),
									],
								}),
							],
						}),
						HorizontalCenterScrollbar($, { left:-1, right:-1, bottom:-10 }),
						VerticalScrollbar($, { right:-10, }),
					],
				}),
			],
		}),
		Line($, {
			left:0, right:10, top:0, height:60, skin:greenHeaderSkin,
			contents: [
				Content($, { left:5, width:200, height:60, skin:iconSkin }),
			],
		}),
	],
}));


