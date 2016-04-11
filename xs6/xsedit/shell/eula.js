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
	ModalContainerBehavior,
	model,
} from "shell/main";

import {
	markdownOptions,
} from "features/files/markdown";

export function About() {
	shell.run(new DialogOpenTransition, new AboutDialog(model));
}

class AboutDialogBehavior extends Behavior {
	onCreate(about, data) {
		this.data = data;
	}
	onEnter(about) {
		shell.run(new DialogCloseTransition, about);
	}
	onEscape(about) {
		shell.run(new DialogCloseTransition, about);
	}
	onDisplayed(about) {
		let markdown = this.data.MARKDOWN;
		let url = markdownOptions.url = mergeURI(shell.url, "./assets/about.md");
		let text = Files.readText(url);
		text = text.replace("$VERSION", getEnvironmentVariable("VERSION"));
		text = text.replace("$CORE_VERSION", getEnvironmentVariable("CORE_VERSION"));
		markdown.formatMarkdown(text, markdownOptions);
		about.focus();
	}
	onKeyDown(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if ((c == 9) || (c== 25)) {
			let tabs = this.tabs;
			let index = tabs.findIndex(label => label.focused);
			let limit = tabs.length - 1;
			if (shiftKey)
				index--;
			else
				index++;
			if (index < 0)
				index = limit;
			else if (index > limit)
				index = 0;
			tabs[index].focus();
			return true;
		}
		if ((c == 3) || (c== 13)) {
			this.onEnter(dialog);
			return true;
		}
		if (c == 27) {
			this.onEscape(dialog);
			return true;
		}
		return false;
	}
	onKeyUp(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		return (c == 3) || (c== 9) || (c== 13) || (c== 25) || (c== 27);
	}
}

class EulaDialogBehavior extends Behavior {
	onCreate(eula, data) {
		this.data = data;
	}
	onEnter(eula) {
		model.eula = true;
		shell.run(new DialogCloseTransition, eula);
		model.checkForUpdate();
	}
	onEscape(eula) {
		model.filesFeature.doQuit();
	}
	onDisplayed(eula) {
		let markdown = this.data.MARKDOWN;
		let url = markdownOptions.url = mergeURI(shell.url, "./assets/code-hardware-eula.md");
		markdown.formatMarkdown(Files.readText(url), markdownOptions);
		system.beginModal();
		eula.focus();
	}
	onKeyDown(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if ((c == 9) || (c== 25)) {
			let tabs = this.tabs;
			let index = tabs.findIndex(label => label.focused);
			let limit = tabs.length - 1;
			if (shiftKey)
				index--;
			else
				index++;
			if (index < 0)
				index = limit;
			else if (index > limit)
				index = 0;
			tabs[index].focus();
			return true;
		}
		if ((c == 3) || (c== 13)) {
			this.onEnter(dialog);
			return true;
		}
		if (c == 27) {
			this.onEscape(dialog);
			return true;
		}
		return false;
	}
	onKeyUp(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		return (c == 3) || (c== 9) || (c== 13) || (c== 25) || (c== 27);
	}
}

class MarkdownTextBehavior extends Behavior {
	onCreate(markdown, data, dictionary) {
		this.data = data;
		this.map = new Map();
	}
};

import {
	PASTEL_GREEN,
	WHITE,
	backgroundSkin,
} from "assets";

import {
	greenButtonSkin,
	greenButtonStyle,
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";

import { 
	ButtonBehavior, 
} from "common/control";

import {
	ScrollerBehavior,
	VerticalScrollbar,
} from "common/scrollbar";

const eulaGraySkin = new Skin({ fill:"#6000" });
const eulaDialogSkin = new Skin({ fill:PASTEL_GREEN, stroke:"#333", borders: { left:4, right:4, top:4, bottom:4 }});

import {
	DialogCloseTransition,
	DialogOpenTransition,
	buttonSkin,
	buttonStyle,
	defaultButtonSkin,
	defaultButtonStyle,
	dialogSeparatorSkin,
	dialogSkin,
} from "features/samples/templates";

const dialogWidth = 800;
const dialogHeight = 600;

var AboutDialog = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: AboutDialogBehavior,
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
						Content($, { left:0, right:0, height:4, }),
						Scroller($, {
							left:0, right:0, top:0, bottom:0, clip:true, active:true, skin:backgroundSkin,
							Behavior: ScrollerBehavior,
							contents: [
								Text($, { anchor:"MARKDOWN", left:5, right:5, top:0, Behavior:MarkdownTextBehavior }),
								VerticalScrollbar($, { }),
							],
						}),
						Content($, { left:0, right:0, height:1, skin:dialogSeparatorSkin, }),
						Container($, {
							left:0, right:0, height:38, 
							contents: [
								Container($, {
									width:96, right:4, top:4, skin:defaultButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
									contents: [
										Label($, { left:0, right:0, style:defaultButtonStyle, string:"OK" }),
									],
								}),
							],
						}),
					],
				}),
			],
		}),
	],
}));

export var Eula = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, skin:eulaGraySkin,
	Behavior: EulaDialogBehavior,
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
						Content($, { left:0, right:0, height:4, }),
						Scroller($, {
							left:0, right:0, top:0, bottom:0, clip:true, active:true, skin:backgroundSkin,
							Behavior: ScrollerBehavior,
							contents: [
								Text($, { anchor:"MARKDOWN", left:5, right:5, top:0, Behavior:MarkdownTextBehavior }),
								VerticalScrollbar($, { }),
							],
						}),
						Content($, { left:0, right:0, height:1, skin:dialogSeparatorSkin, }),
						Container($, {
							left:0, right:0, height:38, 
							contents: [
								Container($, {
									width:96, right:100, top:4, skin:buttonSkin, active:true, name:"onEscape", Behavior: ButtonBehavior,
									contents: [
										Label($, { left:0, right:0, style:buttonStyle, string:"Disagree" }),
									],
								}),
								Container($, {
									width:96, right:4, top:4, skin:defaultButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
									contents: [
										Label($, { left:0, right:0, style:defaultButtonStyle, string:"Agree" }),
									],
								}),
							],
						}),
					],
				}),
			],
		}),
	],
}));

