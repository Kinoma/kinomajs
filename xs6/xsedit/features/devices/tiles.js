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
	BLACK,
	WHITE,
	BUTTON_ENABLED,
	GREEN,
	PASTEL_GREEN,
	DARK_GRAY,
	DARK_GREEN,
	DARKER_GRAY,
	DARKER_GREEN,
	ORANGE,
	PASTEL_ORANGE,
	DARK_ORANGE,
	DARKER_ORANGE,
	GRAY,
	LIGHT_GRAY,
	LIGHT_GREEN,
	NORMAL_FONT,
	PASTEL_GRAY,
	SEMIBOLD_FONT,
	grayHeaderSkin,
	greenHeaderSkin,
	orangeHeaderSkin,
	tableHeaderStyle,
	whiteButtonSkin,
	whiteButtonStyle,
	whiteButtonsSkin,
} from "shell/assets";

import {
	ButtonBehavior,
} from "common/control";

export const greenTileSkin = new Skin({ 
	fill:[GREEN, GREEN, DARK_GREEN, DARKER_GREEN]
});
export const orangeTileSkin = new Skin({ 
	fill:[ORANGE, ORANGE, DARK_ORANGE, DARKER_ORANGE]
});
const tileSelectionTexture = new Texture("assets/tile.png", 2);
export const tileSelectionSkin = new Skin({ texture:tileSelectionTexture, x:0, y:0, width:20, height:20, states:20, variants:20 });
const projectTileSkin = new Skin({ 
	fill:[WHITE, WHITE, PASTEL_GRAY, LIGHT_GRAY],
	stroke:PASTEL_GRAY,
	borders: { left:1, right:1, top:1, bottom:1 },
});

export const tileStyle = new Style({font:SEMIBOLD_FONT, size:16, color:WHITE, left:5, right:5, horizontal:"center" });
export const infoStyle = new Style({size:12 });
const projectTileStyle = new Style({font:NORMAL_FONT, size:14, color:BLACK, left:5, right:5, horizontal:"center" });
const projectTileFooterStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:[WHITE, WHITE, WHITE, BUTTON_ENABLED ] });

// BEHAVIORS

export class TileBehavior extends ButtonBehavior {
	changeState(container, state) {
		container.state = state;
		container.last.state = state;
	}
	onCreate(container, $) {
		container.active = true;
		super.onCreate(container, $);
	}
	onTap(container) {
		let data = this.data;
		Promise.resolve(data).then(data => {
			shell.delegate("doOpenURL", data.url);
		});
	}
	onURLChanged(container, url) {
		container.active = this.data.url != url;
	}
};

class ProjectTileBehavior extends TileBehavior {
	changeState(container, state) {
		container.state = state;
		container.last.state = state;
	}
	onDisplaying(container) {
		this.onMachinesChanged(container, shell.behavior.debugFeature.machines);
	}
	onMachinesChanged(container, machines) {
		let title = this.data.project.title;
		container.active = !machines.some(machine => machine.title == title);
	}
	onProjectChanged(container, project) {
		if (this.data.project == project) {
			if (project.icon)
				container.first.first.next.next.string = project.title;
			else
				container.first.first.next.string = project.title;
		}
	}
	onTap(container) {
		shell.behavior.filesFeature.selectProject(this.data.project);
		container.bubble("doRun");
	}
	onURLChanged(container, url) {
		// nop
	}
};

// TEMPLATES

export var ProjectTile = Container.template($ => ({
	left:0, top:0, skin:projectTileSkin, style:projectTileStyle,
	Behavior: ProjectTileBehavior,
	contents: [
		Column($, {
			left:0, right:0, top:0,
			contents: [
				Content($, { height:10 }),
				$.project.icon ? Thumbnail($, { width:60, height:60, url:$.project.icon }) : null,
				Text($, { width:120, string:$.project.title }),
				Content($, { height:10 }),
				$.device.constructor.tag == "Create" ? ProjectTileFooter($, {}) : null,
			]
		}),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin, variant:2 }),
	]
}));


import {
	model
} from "shell/main";

import tool from "shell/tool";

class ProjectTileFooterBehavior extends Behavior {
	changeState(container, state) {
		let button = container.first;
		let label = button.first;
		switch (state) {
		case 0: 
			container.skin = grayHeaderSkin;
			button.active = false;
			label.string = "Checking...";
			break;
		case 1: 
			container.skin = greenHeaderSkin;
			button.active = true;
			label.string = "Install";
			break;
		case 2: 
			container.skin = orangeHeaderSkin;
			button.active = true;
			label.string = "Delete";
			break;
		case 3: 
			container.skin = greenHeaderSkin;
			button.active = false;
			label.string = "Installing...";
			break;
		case 4: 
			container.skin = orangeHeaderSkin;
			button.active = false;
			label.string = "Deleting...";
			break;
		}
	}
	onComplete(container, message, result) {
		let data = this.data;
		this.installed = result.success 
			&& (result.url.indexOf("kdt/cache") < 0)
			&& (result.url.indexOf("applications/" + data.project.id.split(".").reverse().join(".")) >= 0);
		this.changeState(container, this.installed ? 2 : 1);
	}
	onCreate(container, data) {
		this.data = data;
		this.installed = undefined;
	}
	onDisplaying(container) {
		let data = this.data;
		let device = data.device;
		let url = mergeURI(device.toolURL, "./app/check?id=" + encodeURIComponent(data.project.id))
		let message = new Message(url);
		message.setRequestHeader("Connection", "Close");
		message.setRequestHeader("Authorization", "Basic " + device.authorization);
		container.invoke(message, Message.JSON);
	}
	onMouseEntered(container, x, y) {
		return true;
	}
	onMouseExited(container, x, y) {
		return true;
	}
	onToggle(container) {
		let data = this.data;
		if (this.installed) {
			this.changeState(container, 4);
			tool.delete(data.device, data.project, model.debugFeature, () => {
				this.installed = false;
				this.changeState(container, 1);
			});
		}
		else {
			this.changeState(container, 3);
			tool.install(data.device, data.project, model.debugFeature, () => {
				this.installed = true;
				this.changeState(container, 2);
			});
		}
	}
}

class ProjectTileButtonBehavior extends ButtonBehavior {
}

var ProjectTileFooter = Container.template($ => ({
	left:0, right:0, height:30, skin:grayHeaderSkin, active:true,
	Behavior: ProjectTileFooterBehavior,
	contents: [
		Container($, {
			width:80, skin:whiteButtonSkin, active:false, name:"onToggle",
			Behavior: ProjectTileButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:projectTileFooterStyle, string:"Checking..." }),
			],
		}),
	],
}));





