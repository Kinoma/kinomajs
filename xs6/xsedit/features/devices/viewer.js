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
	headerTitleStyle,
	headerSubtitleStyle,
} from "assets";

import {
	CloseButton,
} from "shell/viewer";

// TEMPLATES

export var AppViewHeader = Line.template($ => ({
	left:0, right:0, top:0, height:60, skin:$.skin,
	contents: [
		Content($, { width:60, skin:$.device.constructor.iconSkin }),
		Column($, { 
			left:0, right:0, height:40,
			contents: [
				Label($, { left:0, right:0, height:20, style:headerTitleStyle, string:$.title }),
				Label($, {
					left:0, right:0, height:20, style:headerSubtitleStyle, string:$.device.name,
					Behavior: class extends Behavior {
						onDeviceSelected(label, device) {
							label.string = device ? device.name : "";
						}
					}
				}),
			],
		}),
		CloseButton($, {}),
	],
}));

import {
	BOLD_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BLACK,
} from "shell/assets";

import {
	ProgressBar,
} from "common/dialog";

import {
	DialogCloseTransition,
	DialogOpenTransition,
	buttonSkin,
	buttonStyle,
	defaultButtonSkin,
	defaultButtonStyle,
	dialogSeparatorSkin,
	dialogSkin,
	progressLeftStyle,
	progressRightStyle,
} from "features/samples/templates";


const updateTitleStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:BLACK, horizontal:"left" });
const updateInfoStyle = new Style({ font:NORMAL_FONT, size:10, color:BLACK, horizontal:"left" });

const dialogWidth = 430;
const dialogHeight = 140;

class UpdateDialogBehavior extends Behavior {
	onCreate(dialog, data) {
		this.data = data;
	}
	onDisplayed(dialog) {
		system.beginModal();
		dialog.focus();
		let data = this.data;
		if ("duration" in data) {
			dialog.duration = data.duration;
			dialog.start();
		}
	}
	onFinished(dialog) {
		let data = this.data;
		data.status.then(json => {
			data.SUBTITLE.string = json.message;
			if (("offset" in json) && ("size" in json))
				data.PROGRESS.delegate("onValueChanged", json.offset, json.size);
			if ("error" in json)
				data.doUpdateError(json);
			else if ("finished" in json)
				data.doUpdateFinished(json);
			else {
				dialog.time = 0;
				dialog.start();
			}
		});
	}
	onKeyDown(dialog, key, repeat, ticks) {
		return false;
	}
	onKeyUp(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		return (c == 3) || (c== 9) || (c== 13) || (c== 25) || (c== 27);
	}
}

export var UpdateDialog = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: UpdateDialogBehavior,
	contents: [
		Layout($, {
			top:0, skin:dialogSkin, clip:true, active:true,
			Behavior: class extends Behavior {
				onMeasureHorizontally(layout, width) {
					return Math.min(dialogWidth, shell.width - 40);
				}
				onMeasureVertically(layout, height) {
					return Math.min(dialogHeight, shell.height - 20);
				}
			},
			contents: [
				Column($, {
					left:5, right:5, top:0, bottom:10, 
					contents: [
						Line($, {
							left:0, right:0, height:80,
							contents: [
								Content($, { width:105, skin:$.device.constructor.iconSkin }),
								Column($, {
									left:0, right:10,
									contents: [
										Text($, { anchor:"TITLE", left:0, right:0, style:updateTitleStyle }),
										Text($, { anchor:"INFO", left:0, right:0, style:updateInfoStyle }),
									]
								}),
							]
						}),
						Line($, {
							left:0, right:0, height:20,
							contents: [
								Label($, { anchor:"SUBTITLE", left:0, right:0, height:20, style:progressLeftStyle }),
								Label($, { anchor:"COUNT", width:80, height:20, style:progressRightStyle }),
							]
						}),
						ProgressBar($, { anchor:"PROGRESS", left:4, right:4, }),
					],
				}),
			],
		}),
	],
}));

export function openDialog(dialog) {
	shell.run(new DialogOpenTransition, dialog);
}

export function closeDialog(dialog) {
	shell.run(new DialogCloseTransition, dialog);
}
