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
	popupItemStyle,
} from "common/menu";

import {
	BLACK,
	grayBodySkin,
} from "shell/assets";

import {
	BLUE,
	WARM_GRAY,
	
	blackSkin,
	blackStyle,
	centerStyle,
	greenSkin,
	headerSize,
	iconSize,
	lightGraySkin,
	localize,
	smallStyle,
	whiteSkin,
	navigationSkin,
	warmGraySkin,

} from "features/devices/assets";

var timeZoneMap = [ "Samoa", "Hawaii", "Alaska", "Pacific", "Mountain", 
				"Central", "Eastern", "Atlantic", "Uruguay", "SGSSI",
				"Azores", "Greenwich Mean", "Central European", "Eastern European", "Indian Ocean",
				"Arabian", "Pakistan", "Bangladesh", "Thailand",
				"China", "Japan", "Australian Eastern", "Vanuatu", "New Zealand"
			];
			
var zoneNumberMap = [ -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 ];
export function getTimezone(zoneNumber) {
	for (var i=0; i < zoneNumberMap.length; i++) {
		if (zoneNumberMap[i] == zoneNumber)
			return timeZoneMap[i];
	}
}


const checkBoxSkin = new Skin({ texture:new Texture("./assets/lighter-checkbox.png"), x:5, y:5, width:30, height:30, states:40 });
const mapSkin = new Skin({ texture:new Texture("./assets/sprites/background-320w.png", 1), x:0, y:0, width:320, height:170 });
const timeZonesTexture1 = new Texture("assets/sprites/1-5-9-sprite.png" , 1);
const timeZonesTexture2 = new Texture("assets/sprites/2-6-10-sprite.png", 1);
const timeZonesTexture3 = new Texture("assets/sprites/3-7-11-sprite.png", 1);
const timeZonesTexture4 = new Texture("assets/sprites/4-8-12-sprite.png", 1);

const minusElevenSkin = new Skin({ texture:timeZonesTexture1, x:0, y:0, width:18, height:170 });
const minusTenSkin = new Skin({ texture:timeZonesTexture2, x:0, y:0, width:38, height:170 });
const minusNineSkin = new Skin({ texture:timeZonesTexture3, x:0, y:0, width:44, height:170 });
const minusEightSkin = new Skin({ texture:timeZonesTexture4, x:20, y:0, width:38, height:170 });
const minusSevenSkin = new Skin({ texture:timeZonesTexture1, x:24, y:0, width:42, height:170 });
const minusSixSkin = new Skin({ texture:timeZonesTexture2, x:40, y:0, width:44, height:170 });
const minusFiveSkin = new Skin({ texture:timeZonesTexture3, x:60, y:0, width:34, height:170 });
const minusFourSkin = new Skin({ texture:timeZonesTexture4, x:76, y:0, width:40, height:170 });
const minusThreeSkin = new Skin({ texture:timeZonesTexture1, x:80, y:0, width:58, height:170 });
const minusTwoSkin = new Skin({ texture:timeZonesTexture2, x:106, y:0, width:28, height:170 });
const minusOneSkin = new Skin({ texture:timeZonesTexture3, x:114, y:0, width:34, height:170 });
const zeroSkin = new Skin({ texture:timeZonesTexture4, x:120, y:0, width:40, height:170 });
const oneSkin = new Skin({ texture:timeZonesTexture1, x:136, y:0, width:46, height:170 });
const twoSkin = new Skin({ texture:timeZonesTexture2, x:152, y:0, width:38, height:170 });
const threeSkin = new Skin({ texture:timeZonesTexture3, x:166, y:0, width:46, height:170 });
const fourSkin = new Skin({ texture:timeZonesTexture4, x:176, y:0, width:36, height:170 });
const fiveSkin = new Skin({ texture:timeZonesTexture1, x:186, y:0, width:54, height:170 });
const sixSkin = new Skin({ texture:timeZonesTexture2, x:196, y:0, width:44, height:170 });
const sevenSkin = new Skin({ texture:timeZonesTexture3, x:212, y:0, width:48, height:170 });
const eightSkin = new Skin({ texture:timeZonesTexture4, x:212, y:0, width:58, height:170 });
const nineSkin = new Skin({ texture:timeZonesTexture1, x:240, y:0, width:46, height:170 });
const tenSkin = new Skin({ texture:timeZonesTexture2, x:252, y:0, width:42, height:170 });
const elevenSkin = new Skin({ texture:timeZonesTexture3, x:264, y:0, width:48, height:170 });
const twelveSkin = new Skin({ texture:timeZonesTexture4, x:274, y:0, width:44, height:170 });

const leftStyle = new Style({ font: "Open Sans", size:14, color:BLACK, horizontal:"left", left:10 });
const rightStyle = new Style({ font: "Open Sans", size:14, color:BLACK, horizontal:"right", right:10 });




export var TimezoneContent = Column.template($ => ({
	left:0, right:0, top:0, height:230,
	Behavior: class extends Behavior {
		doUpdate(column) {
			var data = this.data;
			var timezone = data.device.TIMEZONE;
			var d = new Date();
			var utc = d.getTime();
			var newDate = new Date(utc + (3600000 * (timezone.zone + (timezone["daylight-savings"] ? 1 : 0))));
			data.date = newDate;
			column.distribute("onUpdate");
		}
		onCreate(column, data) {
			this.data = data;
			this.doUpdate(column);
		}
		onTimezoneChanged(column, notify) {
			var data = this.data;
			this.doUpdate(column);
			if (notify)
				data.device.setTimezone(data.device.TIMEZONE);
		}
	},
	contents: [
		Container($, {
			left:0, right:0, height:30,
			contents:[
				Label($, {
					left:0, top:0, height:30, style:leftStyle, string:getTimezone($.device["TIMEZONE"].zone),
					Behavior: class extends Behavior {
						onCreate(label, data) {
							this.data = data;
							this.onUpdate(label);
						}
						onUpdate(label) {
							var timezone = this.data.device.TIMEZONE;
							let zone = timezone.zone;
							label.string = getTimezone(zone);
						}
					}
				}),
				Line($, {
					right:0, top:0, height:30, style:rightStyle, active:true,
					Behavior: class extends Behavior {
						onCreate(container, data) {
							this.data = data;
							this.onUpdate(container);
						}
						onTouchBegan(container, id, x, y, ticks) {
							let state = container.first.state;
							container.first.state = (state == 1) ? 2 : 1;
						}
						onTouchCancelled(container, id, x, y, ticks) {
						}
						onTouchEnded(container, id, x, y, ticks) {
							let state = container.first.state;
							var timezone = this.data.device.TIMEZONE;
							timezone["daylight-savings"] = state == 2;
							container.bubble("onTimezoneChanged", true);

						}
						onUpdate(container) {
							var timezone = this.data.device.TIMEZONE;
							container.first.state = timezone["daylight-savings"] ? 2 : 1;
						}
					},
					contents:[
						Content($, { skin:checkBoxSkin }),
						Label($, { height:30, style:rightStyle, string:"Daylight Savings" }),
					],
				}),
			],
		}),
		Container($, {
			width:320, height:170, skin:mapSkin, active:true,
			Behavior: class extends Behavior {
				onCreate(container, data) {
					this.data = data;
					this.onUpdate(container);
				}
				onTouchBegan(container, id, x, y, ticks) {
					this.selectZone(container, id, x, false);
				}
				onTouchCancelled(container, id, x, y, ticks) {
				}
				onTouchEnded(container, id, x, y, ticks) {
					this.selectZone(container, id, x, true);
				}
				onTouchMoved(container, id, x, y, ticks) {
					this.selectZone(container, id, x, false);
				}
				onUpdate(container) {
					var data = this.data;
					var index = -11;
					var timezone = this.data.device.TIMEZONE;
					let zone = timezone.zone;
					for (let content of container) {
						content.visible = (index == zone);
						index++;
					}
				}
				selectZone(container, id, x, notify) {
					var timezone = this.data.device.TIMEZONE;
					x = x - container.x;
					if (x < 0) x = 0;
					if (x >= container.width) x = container.width - 1;
					let zone = Math.floor(24 * x / 320) - 11;
					timezone.zone = zone;
					container.bubble("onTimezoneChanged", notify);
				}
			},
			contents:[
				Content($, { left:0, skin:minusElevenSkin }),
				Content($, { left:0, skin:minusTenSkin }),
				Content($, { left:0, skin:minusNineSkin }),
				Content($, { left:20, skin:minusEightSkin }),
				Content($, { left:24, skin:minusSevenSkin }),
				Content($, { left:40, skin:minusSixSkin }),
				Content($, { left:60, skin:minusFiveSkin }),
				Content($, { left:76, skin:minusFourSkin }),
				Content($, { left:80, skin:minusThreeSkin }),
				Content($, { left:106, skin:minusTwoSkin }),
				Content($, { left:114, skin:minusOneSkin }),
				Content($, { left:120, skin:zeroSkin }),
				Content($, { left:136, skin:oneSkin }),
				Content($, { left:152, skin:twoSkin }),
				Content($, { left:166, skin:threeSkin }),
				Content($, { left:176, skin:fourSkin }),
				Content($, { left:186, skin:fiveSkin }),
				Content($, { left:196, skin:sixSkin }),
				Content($, { left:212, skin:sevenSkin }),
				Content($, { left:212, skin:eightSkin }),
				Content($, { left:240, skin:nineSkin }),
				Content($, { left:252, skin:tenSkin }),
				Content($, { left:264, skin:elevenSkin }),
				Content($, { left:274, skin:twelveSkin }),
			],
		}),
		Container($, {
			left:0, right:0, height:30,
			contents:[
				Label($, {
					left:0, right:0, height:30, style:leftStyle,
					Behavior: class extends Behavior {
						onCreate(label, data) {
							this.data = data;
						}
						onUpdate(label) {
							var date = this.data.date;
							if (date)
								label.string = (date.getUTCMonth() + 1) + "/" + date.getUTCDate() + "/" + date.getUTCFullYear();
							else
								label.string = "";
						}
					},
				}),
				Label($, {
					left:0, right:0, height:30, style:rightStyle,
					Behavior: class extends Behavior {
						onCreate(label, data) {
							this.data = data;
						}
						onUpdate(label) {
							var date = this.data.date;
							if (date)
								label.string = date.getUTCHours() + ":" + date.getUTCMinutes();
							else
								label.string = "";
						}
					},
				}),
			],
		}),
	]
}));
