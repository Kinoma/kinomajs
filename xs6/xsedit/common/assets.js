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
	DARK_GRAY,
	GRAY,
	GREEN,
	ORANGE,
	RED,
	WHITE,
} from "shell/assets";

import {
	EffectSkin,
} from "control";

const buttonMaskTexture = new Texture("./assets/buttonMask.png");

var disabledEffect = new Effect;
disabledEffect.colorize(blendColors(0.8, GREEN, WHITE), 1);
var whiteEffect = new Effect;
whiteEffect.colorize(WHITE, 1);
var grayEffect = new Effect;
grayEffect.colorize(GRAY, 1);
var greenEffect = new Effect;
greenEffect.colorize(GREEN, 1);
var orangeEffect = new Effect;
orangeEffect.colorize(ORANGE, 1);
var redEffect = new Effect;
redEffect.colorize(RED, 1);
var selectedEffect = new Effect;
selectedEffect.colorize(BLACK, 1);
export const whiteButtonSkin = new EffectSkin([disabledEffect, whiteEffect, whiteEffect, selectedEffect], {
	texture: buttonMaskTexture,
	x:0, y:0, width:40, height:30, 
	tiles: { left:15, right:15 },
	states:30,
});
export const whiteButtonStyle = new Style({ font:"Open Sans Semibold", size:14, color:[ DARK_GRAY, BLACK, BLACK, WHITE ] });
export const greenButtonSkin = new EffectSkin([disabledEffect, greenEffect, greenEffect, selectedEffect], {
	texture: buttonMaskTexture,
	x:0, y:0, width:40, height:30, 
	tiles: { left:15, right:15 },
	states:30,
});
export const greenButtonStyle = new Style({ font:"Open Sans Semibold", size:14, color:[ DARK_GRAY, WHITE, WHITE, WHITE ] });
export const orangeButtonSkin = new EffectSkin([disabledEffect, orangeEffect, orangeEffect, selectedEffect], {
	texture: buttonMaskTexture,
	x:0, y:0, width:40, height:30, 
	tiles: { left:15, right:15 },
	states:30,
});
export const orangeButtonStyle = new Style({ font:"Open Sans Semibold", size:14, color:[ DARK_GRAY, WHITE, WHITE, WHITE ] });
export const redButtonSkin = new EffectSkin([disabledEffect, redEffect, redEffect, selectedEffect], {
	texture: buttonMaskTexture,
	x:0, y:0, width:40, height:30, 
	tiles: { left:15, right:15 },
	states:30,
});
export const redButtonStyle = new Style({ font:"Open Sans Semibold", size:14, color:[ DARK_GRAY, WHITE, WHITE, WHITE ] });

export const grayButtonSkin = new EffectSkin([disabledEffect, grayEffect, grayEffect, selectedEffect], {
	texture: buttonMaskTexture,
	x:0, y:0, width:40, height:30, 
	tiles: { left:15, right:15 },
	states:30,
});
export const grayButtonStyle = new Style({ font:"Open Sans Semibold", size:14, color:[ DARK_GRAY, WHITE, WHITE, WHITE ] });
