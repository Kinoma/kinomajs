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
export const BOLD_FONT = "Open Sans Bold";
export const LIGHT_FONT = "Open Sans Light";
export const NORMAL_FONT = "Open Sans";
export const SEMIBOLD_FONT = "Open Sans Semibold";

export const BLACK = "black";
export const TRANSPARENT = "transparent";
export const WHITE = "white";

export const PASTEL_ALPHA = 0.70;
export const LIGHT_ALPHA = 0.50;
export const DARK_ALPHA = 0.10;
export const DARKER_ALPHA = 0.20;

export const CYAN = "#6497ff";
export const PASTEL_CYAN = blendColors(PASTEL_ALPHA, CYAN, WHITE);
export const LIGHT_CYAN = blendColors(LIGHT_ALPHA, CYAN, WHITE);
export const DARK_CYAN = blendColors(DARK_ALPHA, CYAN, BLACK);
export const DARKER_CYAN = blendColors(DARKER_ALPHA, CYAN, BLACK);

export const GRAY = "#a0a0a0";
export const PASTEL_GRAY = blendColors(PASTEL_ALPHA, GRAY, WHITE);
export const LIGHT_GRAY = blendColors(LIGHT_ALPHA, GRAY, WHITE);
export const DARK_GRAY = blendColors(DARK_ALPHA, GRAY, BLACK);
export const DARKER_GRAY = blendColors(DARKER_ALPHA, GRAY, BLACK);

export const GREEN = "#7fbd3b";
export const PASTEL_GREEN = blendColors(PASTEL_ALPHA, GREEN, WHITE);
export const LIGHT_GREEN = blendColors(LIGHT_ALPHA, GREEN, WHITE);
export const DARK_GREEN = blendColors(DARK_ALPHA, GREEN, BLACK);
export const DARKER_GREEN = blendColors(DARKER_ALPHA, GREEN, BLACK);

export const ORANGE = "#fe9d27";
export const PASTEL_ORANGE = blendColors(PASTEL_ALPHA, ORANGE, WHITE);
export const LIGHT_ORANGE = blendColors(LIGHT_ALPHA, ORANGE, WHITE);
export const DARK_ORANGE = blendColors(DARK_ALPHA, ORANGE, BLACK);
export const DARKER_ORANGE = blendColors(DARKER_ALPHA, ORANGE, BLACK);

export const RED = "#ee434a";
export const PASTEL_RED = blendColors(PASTEL_ALPHA, RED, WHITE);
export const LIGHT_RED = blendColors(LIGHT_ALPHA, RED, WHITE);
export const DARK_RED = blendColors(DARK_ALPHA, RED, BLACK);
export const DARKER_RED = blendColors(DARKER_ALPHA, RED, BLACK);

export const YELLOW = "#f2db1d";
export const PASTEL_YELLOW = blendColors(PASTEL_ALPHA, YELLOW, WHITE);
export const LIGHT_YELLOW = blendColors(LIGHT_ALPHA, YELLOW, WHITE);
export const DARK_YELLOW = blendColors(DARK_ALPHA, YELLOW, BLACK);
export const DARKER_YELLOW = blendColors(DARKER_ALPHA, YELLOW, BLACK);


export const grayBodySkin = new Skin({ fill:PASTEL_GRAY });
export const grayBorderSkin = new Skin({ stroke:PASTEL_GRAY, borders: { left:1, right:1, bottom:1 } });
export const grayFooterSkin = new Skin({ stroke:PASTEL_GRAY, borders: { top:1 } });
export const grayHeaderSkin = new Skin({ fill:[GRAY, DARK_GRAY, DARKER_GRAY, GRAY] });
export const grayLineSkin = new Skin({ fill:[WHITE, PASTEL_GRAY, LIGHT_GRAY, PASTEL_GRAY], stroke:PASTEL_GRAY, borders: { left:1, right:1 } });

export const greenBodySkin = new Skin({ fill:PASTEL_GREEN });
export const greenBorderSkin = new Skin({ stroke:PASTEL_GREEN, borders: { left:1, right:1, bottom:1 } });
export const greenFooterSkin = new Skin({ stroke:PASTEL_GREEN, borders: { top:1 } });
export const greenHeaderSkin = new Skin({ fill:[GREEN, DARK_GREEN, DARKER_GREEN, GREEN] });
export const greenLineSkin = new Skin({ fill:[WHITE, PASTEL_GRAY, LIGHT_GRAY, PASTEL_GREEN], stroke:PASTEL_GREEN, borders: { left:1, right:1 } });

export const orangeBodySkin = new Skin({ fill:PASTEL_ORANGE });
export const orangeBorderSkin = new Skin({ stroke:PASTEL_ORANGE, borders: { left:1, right:1, bottom:1 } });
export const orangeFooterSkin = new Skin({ stroke:PASTEL_ORANGE, borders: { top:1 } });
export const orangeHeaderSkin = new Skin({ fill:[ORANGE, DARK_ORANGE, DARKER_ORANGE, ORANGE] });
export const orangeLineSkin = new Skin({ fill:[WHITE, PASTEL_GRAY, LIGHT_GRAY, PASTEL_ORANGE], stroke:PASTEL_ORANGE, borders: { left:1, right:1 } })

export const redBorderSkin = new Skin({ stroke:PASTEL_RED, borders: { left:1, right:1, bottom:1 } });
export const redHeaderSkin = new Skin({ fill:[RED, DARK_RED, DARKER_RED, RED] });

export const tableHeaderStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:WHITE, horizontal:"left" });
export const tableLineStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left" });

export const BUTTON_DISABLED = "#cdcdcd";
export const BUTTON_ENABLED = "#363636";

const fileGlyphsTexture = new Texture("assets/fileGlyphs.png", 2);
export const fileGlyphsSkin = new Skin({ texture:fileGlyphsTexture, x:0, y:0, width:20, height:20, variants:20, states:20 });

const buttonsTexture = new Texture("assets/blackButtons.png", 2);
export const blackButtonSkin = new Skin({ texture:buttonsTexture, x:0, y:0, width:60, height:30, states:30, tiles:{ left:15, right:15 } });
export const blackButtonsSkin = new Skin({ texture:buttonsTexture, x:90, y:0, width:30, height:30, variants:30, states:30 });
export const blackButtonStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:[BUTTON_DISABLED, BUTTON_ENABLED, BUTTON_ENABLED, WHITE ] });
export const blackMenuSkin = new Skin({ texture:buttonsTexture, x:180, y:0, width:60, height:30, states:30, tiles:{ left:15, right:15 } });

const whiteButtonsTexture = new Texture("assets/whiteButtons.png", 2);
export const whiteButtonSkin = new Skin({ texture:whiteButtonsTexture, x:0, y:0, width:60, height:30, states:30, tiles:{ left:15, right:15 } });
export const whiteButtonsSkin = new Skin({ texture:whiteButtonsTexture, x:90, y:0, width:30, height:30, variants:30, states:30 });
export const whiteButtonStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:[BUTTON_DISABLED, WHITE, WHITE, BUTTON_ENABLED ] });

export const backgroundSkin = new Skin({ fill:WHITE });

export const menuHeaderStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:WHITE, horizontal:"left" });
export const menuLineSkin = new Skin({ fill:[TRANSPARENT, TRANSPARENT, PASTEL_GRAY, LIGHT_GRAY] });
export const menuLineStyle = new Style({ font:SEMIBOLD_FONT, size:14, color: [BUTTON_DISABLED, BLACK, BLACK, BLACK] })

export const featureEmptyStyle = new Style({ font:SEMIBOLD_FONT, size:18, color: LIGHT_GRAY })

export const fieldLabelSkin = new Skin({ fill: [ "transparent","transparent","#e0e0e0", "#cbe1fa" ] });
export const fieldScrollerSkin = new Skin({ fill: WHITE });





