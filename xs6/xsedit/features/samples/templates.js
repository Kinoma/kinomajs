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
	model,
} from "shell/main";

import {
	BOLD_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BLACK,
	WHITE,

	LIGHT_GRAY,
	LIGHT_ORANGE,
	PASTEL_ORANGE,
	PASTEL_GREEN,
	ORANGE,
	
	PASTEL_GRAY,
	
	grayBodySkin,
	grayBorderSkin,
	grayHeaderSkin,
	greenBorderSkin,
	greenHeaderSkin,
	orangeBorderSkin,
	orangeHeaderSkin,
	tableHeaderStyle,
	whiteButtonsSkin,
} from "shell/assets";	

import {
	grayButtonSkin,
	grayButtonStyle,
	greenButtonSkin,
	greenButtonStyle,
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";	

const DIALOG_FONT = NORMAL_FONT;
const backgroundSkin = new Skin({ fill:WHITE });
export const dialogSeparatorSkin = new Skin({ fill:LIGHT_GRAY });
const dialogFieldLabelSkin = new Skin({ fill: [ "transparent","transparent",PASTEL_GREEN,PASTEL_GREEN ] });
export const dialogTitleStyle = new Style({ font:DIALOG_FONT, size:12, color:"#202020", horizontal:"left", left:10 });
export const dialogFieldNameStyle = new Style({ font:DIALOG_FONT, size:12, color:"#202020", horizontal:"right", right:10 });
export const dialogFieldValueStyle = new Style({ font:DIALOG_FONT, size:12, color:BLACK, horizontal:"left", left:4 });
const dialogFooterSkin = new Skin({ fill:PASTEL_GRAY });

const dialogWidth = 840;
const dialogHeight = 630;

const templatesTexture = new Texture("./assets/templates.png");
export const dialogSkin = new Skin({ texture: templatesTexture, x:0, y:0, width:40, height:40,
	tiles: { left:10, right: 10, top:10, bottom: 20 },
});
const dialogFieldScrollerSkin = new Skin({ texture: templatesTexture, x:0, y:80, width:20, height:20, states:20, variants:20,
	tiles: { left:5, right: 5, top:5, bottom: 5 },
	margins: { left:4, right: 4, top:4, bottom: 4 },
});
export const buttonSkin = new Skin({ texture: templatesTexture, x:40, y:0, width:40, height:30, states:30,
	tiles: { left:10, right: 10 },
});
export const buttonStyle = new Style({ font:DIALOG_FONT, size:13, color:[LIGHT_GRAY, "#202020", "#202020", WHITE] });
export const defaultButtonSkin = new Skin({ texture: templatesTexture, x:80, y:0, width:40, height:30, states:30,
	tiles: { left:10, right: 10 },
});
export const defaultButtonStyle = new Style({ font:DIALOG_FONT, size:13, color:[LIGHT_GRAY, WHITE, WHITE, WHITE] });

export const progressBarSkin = new Skin({ texture: templatesTexture, x:120, y:5, width:48, height:20, states:30,
	tiles: { left:6, right:6 },
});
export const progressCombSkin = new Skin({ texture: templatesTexture, x:120, y:95, width:48, height:20,
	tiles: { left:0, right:0 },
});

export const progressRightStyle = new Style({ font:DIALOG_FONT, size:12, color:"#202020", horizontal:"right", right:10 });
export const progressLeftStyle = new Style({ font:DIALOG_FONT, size:12, color:"#202020", horizontal:"left", left:10 });

const selectionSkin = new Skin({ texture: templatesTexture, x:0, y:40, width:20, height:20, states:20, variants:20,
	tiles: { left:5, right: 5, top:5, bottom: 5 },
});
const templateDeviceLineStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:["#202020", WHITE], left:5, right:5 });

const dialogLineStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:["#202020", WHITE], horizontal:"left", left:5 });
const itemSelectionSkin = new Skin({ fill:[ "transparent", "#4d4c4d" ] });


const templateTitleStyle = new Style({ font:BOLD_FONT, size:14, color:"#202020", left:10, right:10, top:5, bottom:5 });
const templateDescriptionStyle = new Style({ font:NORMAL_FONT, size:14, color:"#202020", horizontal:"left", left:10, right:10, bottom:5 });

const strings = {
	replaceFolderInfo: "A folder with the same name already exists. Replacing it will overwrite its current contents.",
	replaceFileInfo: "A file with the same name already exists. Replacing it will overwrite its current contents.",
	replacePrompt: "\"$\" already exists. Do you want to replace it?",
};

import {
	ButtonBehavior,
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

export class TemplatesModel {
	constructor() {
		let tags = model.samplesFeature.tags;
		let mask = 0;
		this.Configs = model.devicesFeature.Configs.filter(Config => {
			let name = Config.templateTag;
			if (name) {
				let tag = tags.find(tag => tag.name == name);
				if (tag) {
					mask |= tag.mask;
					return Config;
				}
			}
		});
		this.devicesMask = mask;
		this.includeSamples = true;
		this.selectConfig(this.Configs[0], 0);
		this.title = "";
		this.id = "";
		this.domain = model.projectsDomain;
	}
	run() {
		shell.run(new DialogOpenTransition, new TemplatesDialog(this));
	}
	selectConfig(Config, selection) {
		this.ConfigSelection = selection;
		let feature = model.samplesFeature;
		let name = Config.templateTag;
		let tag = feature.tags.find(tag => tag.name == name);
		if (tag) {
			let mask = this.deviceMask = tag.mask;
			let result = feature.templates.reduce((result, item) => {
				if (item.mask & mask)
					result |= item.mask;
				return result;
			}, 0);
			if (this.includeSamples) {
				result = feature.samples.items.reduce((result, item) => {
					if (item.mask & mask)
						result |= item.mask;
					return result;
				}, result);
			}
			let tags = [];
			feature.tags.forEach(tag => {
				let mask = tag.mask;
				if ((!(this.devicesMask & mask)) && (result & mask))
					tags.push({mask, name:tag.name, title:tag.title })
			});
			this.tags = tags.sort((a, b) => a.title.compare(b.title));
			let index = this.tags.findIndex(tag => tag.name == "emptySample");
			this.selectTag(this.tags[index], index);
		}
		else {
			this.deviceMask = 0;
			this.tags = [];
			this.selectTag(null, -1);
		}
	}
	selectTag(tag, selection) {
		this.tagSelection = selection;
		if (tag) {
			let feature = model.samplesFeature;
			let mask = this.deviceMask | tag.mask;
			let templates = feature.templates.filter(item => (item.mask & mask) == mask);
			if (this.includeSamples)
				templates = templates.concat(feature.samples.items.filter(item => (item.mask & mask) == mask));
			templates.forEach(template => template.sorter = template.title.toLowerCase());	
			this.templates = templates.sort((a, b) => a.sorter.compare(b.sorter));
			templates.forEach(template => delete template.sorter);	
			this.selectTemplate(this.templates[0], 0);
		}
		else {
			this.templates = [];
			this.selectTemplate(null, -1);
		}
	}
	selectTemplate(template, selection) {
		this.templateSelection = selection;
		this.template = template;
	}
}

class TemplatesDialogBehavior extends Behavior {
	
	onClose(dialog) {
		shell.run(new DialogCloseTransition, dialog);
	}
	onCreate(dialog, data) {
		this.data = data;
	}
	onDisplaying(dialog) {
		let data = this.data;
		this.tabs = [
			data.PROJECT_TITLE,
			data.APP_IDENTIFIER,
			data.DOMAIN_NAME,
		];
		dialog.distribute("onConfigSelected", data);
		dialog.distribute("onTagSelected", data);
		dialog.distribute("onTemplateSelected", data);
		dialog.distribute("onFolderChanged", Files.toPath(model.projectsDirectory));
	}
	onDisplayed(dialog) {
		model.onHover(shell);
	}
	onEscape(dialog) {
		this.onClose(dialog);
	}
	onEnter(dialog) {
		let data = this.data;
		let url = mergeURI(model.projectsDirectory, data.id);
		let type = Files.exists(url);
		url += "/";
		if (type) {
			system.alert({ 
				type:"stop", 
				prompt:strings.replacePrompt.replace("$", data.id), 
				info:type == Files.directoryType ? strings.replaceFolderInfo : strings.replaceFileInfo,
				buttons:["Replace", "Cancel"]
			}, ok => {
				if (ok === undefined)
					return;
				if (ok) {
					model.deleteDirectory(url);
					this.onClose(dialog);
					Files.ensureDirectory(url, true);
					model.samplesFeature.install(this.data);
				}
			});
		}		
		else {
			this.onClose(dialog);
			Files.ensureDirectory(url, true);
			model.samplesFeature.install(this.data);
		}		
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
			if (this.data.OK.active)
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
	onLocate(dialog) {
		var dictionary = { message:"Locate Kinoma Code Projects Folder", prompt:"Open", url:Files.documentsDirectory };
		system.openDirectory(dictionary, url => { 
			if (url) {
				model.projectsDirectory = url;
				dialog.distribute("onFolderChanged", Files.toPath(url));
			}
		});
	}
	onSamplesChanged(dialog) {
		let data = this.data;
		this.onSelectConfig(dialog, data.Configs[data.ConfigSelection], data.ConfigSelection);
	}
	onSelectConfig(dialog, item, selection) {
		let data = this.data;
		data.selectConfig(item, selection);
		dialog.distribute("onConfigSelected", data);
		dialog.distribute("onTagSelected", data);
		dialog.distribute("onTemplateSelected", data);
	}
	onSelectTag(dialog, item, selection) {
		let data = this.data;
		data.selectTag(item, selection);
		dialog.distribute("onTagSelected", data);
		dialog.distribute("onTemplateSelected", data);
	}
	onSelectTemplate(dialog, item, selection) {
		let data = this.data;
		data.selectTemplate(item, selection);
		dialog.distribute("onTemplateSelected", data);
	}
};
 
class ConfigsColumnBehavior extends Behavior {	
	onCreate(column, data) {
		this.data = data;
	}
};

class ConfigLineBehavior extends Behavior {	
	onCreate(line, data) {
		this.data = data;
	}
	onConfigSelected(line, model) {
		line.first.state = line.last.state = line.index == model.ConfigSelection ? 1 : 0;
	}
	onTouchEnded(line) {
		line.bubble("onSelectConfig", this.data, line.index);
	}
};

class TagsColumnBehavior extends Behavior {	
	onCreate(column, data) {
		this.data = data;
	}
	onConfigSelected(column, model) {
		column.empty();
		model.tags.forEach($ => column.add(new TagLine($)));
	}
};

class TagLineBehavior extends Behavior {	
	onCreate(line, data) {
		this.data = data;
	}
	onTagSelected(line, model) {
		line.first.state = line.state = line.index == model.tagSelection ? 1 : 0;
	}
	onTouchEnded(line) {
		line.bubble("onSelectTag", this.data, line.index);
	}
};

class TemplatesColumnBehavior extends Behavior {	
	onCreate(column, data) {
		this.data = data;
	}
	onTagSelected(column, model) {
		column.empty();
		model.templates.forEach($ => column.add(new TemplateLine($)));
	}
};

class TemplateLineBehavior extends Behavior {	
	onCreate(line, data) {
		this.data = data;
	}
	onTemplateSelected(line, model) {
		line.first.state = line.state = line.index == model.templateSelection ? 1 : 0;
	}
	onTouchEnded(line) {
		line.bubble("onSelectTemplate", this.data, line.index);
	}
};

class TemplateTextBehavior extends Behavior {	
	onTemplateSelected(text, model) {
		let template = model.template;
		if (template) {
			if (template.thumbnail) {
				text.format([
					{ style:templateTitleStyle, spans: [
						{ string:template.title },
					] },
					{ style:templateDescriptionStyle, spans: [
						{ content: new TemplateThumbnail(template), align:"left"  },
						{ string:template.description },
					] }
				]);
			}
			else {
				text.format([
					{ style:templateTitleStyle, spans: [
						{ string:template.title },
					] },
					{ style:templateDescriptionStyle, spans: [
						{ string:template.description },
					] }
				]);
			}
		}
		else
			text.string = "";
	}
};

class DialogFieldLabelBehavior extends FieldLabelBehavior {
	onFocused(label) {
		label.container.variant = 1;
	}
	onUnfocused(label) {
		label.container.variant = 0;
	}
};

class TitleFieldLabelBehavior extends DialogFieldLabelBehavior {
	onEdited(label) {
		let data = this.data;
		data.title = label.string;
		data.id = label.string.replace(/[^\.a-zA-Z0-9-]/g, "-").toLowerCase();
		data.APP_IDENTIFIER.string = data.id;
		data.OK.active = data.title.length && data.id.length && data.domain.length;
	}
};

class DNSFieldLabelBehavior extends DialogFieldLabelBehavior {
	insertKey(label, key) {
		label.insert(key.replace(/[^\.a-zA-Z0-9-]/g, "-").toLowerCase());
	}
};

class IDFieldLabelBehavior extends DNSFieldLabelBehavior {
	onEdited(label) {
		let data = this.data;
		data.id = label.string;
		data.OK.active = data.title.length && data.id.length && data.domain.length;
	}
};

class DomainFieldLabelBehavior extends DNSFieldLabelBehavior {
	onEdited(label) {
		let data = this.data;
		data.domain = label.string;
		data.OK.active = data.title.length && data.id.length && data.domain.length;
	}
};

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

export var TemplatesDialog = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: TemplatesDialogBehavior,
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
						Content($, { height:2, }),
						Label($, { left:0, right:0, height:38, style:dialogTitleStyle, string:"Choose a template for the new project:" }),
						Content($, { left:0, right:0, height:1, skin:dialogSeparatorSkin, }),
						Line($, {
							left:0, right:0, top:0, bottom:0, skin:backgroundSkin,
							contents: [
								Container($, {
									width:120, top:0, bottom:0,
									contents: [
										Scroller($, {
											left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
											contents: [
												Column($, {
													left:0, right:0, top:0, 
													Behavior: ConfigsColumnBehavior,
													contents: $.Configs.map($$ => new ConfigLine($$)),
												}),
												VerticalScrollbar($, {}),
											]
										}),
									],
								}),
								Content($, { width:1, top:0, bottom:0, skin:dialogSeparatorSkin, }),
								Container($, {
									width:180, top:0, bottom:0,
									contents: [
										Scroller($, {
											left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
											contents: [
												Column($, {
													left:0, right:0, top:0, 
													Behavior: TagsColumnBehavior,
												}),
												VerticalScrollbar($, {}),
											]
										}),
									],
								}),
								Content($, { width:1, top:0, bottom:0, skin:dialogSeparatorSkin, }),
								Container($, {
									width:240, top:0, bottom:0,
									contents: [
										Scroller($, {
											left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
											contents: [
												Column($, {
													left:0, right:0, top:0, 
													Behavior: TemplatesColumnBehavior,
												}),
												VerticalScrollbar($, {}),
											]
										}),
									],
								}),
								Content($, { width:1, top:0, bottom:0, skin:dialogSeparatorSkin, }),
								Container($, {
									left:0, right:0, top:0, bottom:0,
									contents: [
										Scroller($, {
											left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
											contents: [
												Text($, { left:0, right:0, top:0, Behavior:TemplateTextBehavior }),
												VerticalScrollbar($, {}),
											]
										}),
									],
								}),
							],
						}),
						Content($, { left:0, right:0, height:1, skin:dialogSeparatorSkin, }),
						Content($, { left:0, right:0, height:4, }),
						Line($, {
							left:0, right:0, height:30, 
							contents: [
								Label($, { width:190, style:dialogFieldNameStyle, string:"Project Title:" }),
								Scroller($, {
									left:4, right:4, top:4, bottom:4, skin:dialogFieldScrollerSkin, clip:true, active:true,
									Behavior: FieldScrollerBehavior,
									contents: [
										Label($, {
											anchor:"PROJECT_TITLE", left: 0, top:2, bottom:2, skin:dialogFieldLabelSkin, style:dialogFieldValueStyle, editable:true,
											Behavior: TitleFieldLabelBehavior
										}),
									],
								}),
								Content($, { width:190 }),
							],
						}),
						Line($, {
							left:0, right:0, height:30, 
							contents: [
								Label($, { width:190, style:dialogFieldNameStyle, string:"App Identifier:" }),
								Scroller($, {
									left:4, right:4, top:4, bottom:4, skin:dialogFieldScrollerSkin, clip:true, active:true,
									Behavior: FieldScrollerBehavior,
									contents: [
										Label($, {
											anchor:"APP_IDENTIFIER", left: 0, top:2, bottom:2, skin:dialogFieldLabelSkin, style:dialogFieldValueStyle, editable:true,
											Behavior: IDFieldLabelBehavior
										}),
									],
								}),
								Content($, { width:190 }),
							],
						}),
						Line($,{
							left:0, right:0, height:30, 
							contents: [
								Label($, { width:190, style:dialogFieldNameStyle, string:"Projects Folder:" }),
								Scroller($, {
									left:4, right:4, top:4, bottom:4, skin:dialogFieldScrollerSkin, state:1, clip:true, active:true,
									Behavior: ScrollerBehavior,
									contents: [
										Label($, { 
											left: 0, top:2, bottom:2, style:dialogFieldValueStyle,
											Behavior: class extends Behavior {
												onFolderChanged(label, path) {
													label.string = path;
												}
											}
										}),
									],
								}),
								Container($, { 
									name:"onLocate", width:80, skin:buttonSkin, active:true, Behavior:ButtonBehavior,
									contents: [
										Label($, { left:0, right:0, style:buttonStyle, string:"Locate" }),
									],
								}),
								Content($, { width:110 }),
							],
						}),
						Line($, {
							left:0, right:0, height:30, 
							contents: [
								Label($, { width:190, style:dialogFieldNameStyle, string:"Domain Name:" }),
								Scroller($, {
									left:4, right:4, top:4, bottom:4, skin:dialogFieldScrollerSkin, clip:true, active:true,
									Behavior: FieldScrollerBehavior,
									contents: [
										Label($, {
											anchor:"DOMAIN_NAME", left: 0, top:2, bottom:2, skin:dialogFieldLabelSkin, style:dialogFieldValueStyle, string:$.domain, editable:true,
											Behavior: DomainFieldLabelBehavior
										}),
									],
								}),
								Content($, { width:190 }),
							],
						}),
						Content($, { left:0, right:0, height:4, }),
						Content($, { left:0, right:0, height:1, skin:dialogSeparatorSkin, }),
						Container($, {
							left:0, right:0, height:38, 
							contents: [
								Container($, {
									width:80, right:110, top:4, skin:buttonSkin, active:true, name:"onEscape",
									Behavior:ButtonBehavior,
									contents: [
										Label($, { left:0, right:0, style:buttonStyle, string:"Cancel" }),
									],
								}),
								Container($, {
									anchor:"OK", width:106, right:4, top:4, skin:defaultButtonSkin, name:"onEnter",
									Behavior:ButtonBehavior,
									contents: [
										Label($, { left:0, right:0, style:defaultButtonStyle, string:"New Project" }),
									],
								}),
							],
						}),
					]
				}),
			]
		}),
	],
}));

var ConfigLine = Container.template($ => ({
	left:0, right:0, height:100, active:true,
	Behavior: ConfigLineBehavior,
	contents: [
		Container($, { 
			width:64, top:10, height:64, skin:selectionSkin, 
			contents: [
				Content($, { width:60, height:60, skin:$.iconSkin }),
			]
		}),
		Label($, { top:77, height:22, skin:selectionSkin, variant:1, style:templateDeviceLineStyle, string:$.product }),
	],
}));

var TagLine = Line.template($ => ({
	left:0, right:0, height:30, skin:itemSelectionSkin, active:true,
	Behavior: TagLineBehavior,
	contents: [
		Label($, { left:0, right:0, style:dialogLineStyle, string:$.title }),
	],
}));

var TemplateLine = Line.template($ => ({
	left:0, right:0, height:30, skin:itemSelectionSkin, active:true,
	Behavior: TemplateLineBehavior,
	contents: [
		Label($, { left:0, right:0, style:dialogLineStyle, string:$.title }),
	],
}));

var TemplateThumbnail = Container.template($ => ({
	width:90, height:60,
	contents:[
		Content($, { left:10, width:80, top:0, height:60, skin:grayBodySkin }),
		Thumbnail($, { left:10, width:80, top:0, height:60, url:$.thumbnail }),
	],
}));


// TRANSITIONS

class DialogTransition extends Transition {
	constructor() {
		super(400);
	}
	onBegin(container, dialog) {
		this.dialog = dialog;
		var layer = this.layer = new Layer();
		layer.attach(dialog.first);
		this.delta = 0 - layer.height
	}
	onEnd(container, dialog) {
		this.layer.detach();
	}
}

export class DialogOpenTransition extends DialogTransition {
	onBegin(container, dialog) {
		system.beginModal();
		container.add(dialog);
		super.onBegin(container, dialog);
	}
	onEnd(container, dialog) {
		super.onEnd(container, dialog);
		if ("tabs" in dialog.behavior)
			dialog.behavior.tabs[0].focus();
	}
	onStep(fraction) {
		fraction = Math.quadEaseOut(fraction);
		this.layer.translation = { x: 0, y: this.delta * (1 - fraction) };
	}
}

export class DialogCloseTransition extends DialogTransition {
	onBegin(container, dialog) {
		shell.focus();
		super.onBegin(container, dialog);
	}
	onEnd(container, dialog) {
		super.onEnd(container, dialog);
		container.remove(dialog);
		system.endModal();
	}
	onStep(fraction) {
		fraction = Math.quadEaseIn(fraction);
		this.layer.translation = { x: 0, y: this.delta * fraction };
	}
}






