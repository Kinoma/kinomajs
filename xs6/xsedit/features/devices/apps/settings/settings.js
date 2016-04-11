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

import { 
	model,
} from "shell/main";

class DeviceNameSetting {
	constructor($) {
		this.device = $.device;
		this.title = "Name";
		this.Template = DeviceNameContent;
	}
	get value() {
		return this.device.name;
	}
	set value(it) {
		this.device.name = it;
	}
	put(line) {
		this.device.setName(this.value);
	}
}

var DeviceNameContent = Column.template($ => ({
	left:0, right:0, top:0, height:40,
	Behavior: class extends Behavior {
		onCreate(container, data) {
			this.data = data;
		}
		onEnter(column) {
			let data = this.data;
			data.device.setName(data.FIELD.string);
			column.bubble("onClose");
			return true;
		}
	},
	contents: [
		Line($, {
			left:0, right:0, height:40,
			contents: [
				Label($, { width:160, style:settingNameStyle, string:$.title }),
				Scroller($, {
					left:3, right:3, top:5, bottom:5, skin: fieldScrollerSkin, clip:true, active:true,
					Behavior: FieldScrollerBehavior,
					contents: [
						Label($, {
							anchor:"FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:fieldLabelStyle, string:$.value, editable:true,
							Behavior: class extends FieldLabelBehavior {
								onDisplaying(label) {
									label.focus();
								}
							}
						}),
					],
				}),
				Container($, {
					width:80, skin:whiteButtonSkin, active:true, name:"onClose", Behavior: ButtonBehavior,
					contents: [
						Label($, { left:0, right:0, style:whiteButtonStyle, string:"Cancel" }),
					],
				}),
				Container($, {
					width:80, skin:orangeButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
					contents: [
						Label($, { left:0, right:0, style:orangeButtonStyle, string:"Rename" }),
					],
				}),
			],
		})
	]
}));

class MACAddressSetting {
	constructor($) {
		this.device = $.device;
		this.title = "MAC Address";
	}
	load(line) {
		this.device.getNetworkMAC().then(mac => {
			this.value = mac;
			if (!line.container) return;
			line.container.replace(line, new ValueSettingLine(this));
		});
	}
}

class UpdateSetting {
	constructor($) {
		this.device = $.device;
	}
	doUpdate() {
		debugger
	}
	get status() {
		debugger
	}
	get value() {
		return this.device[this.version];
	}
	get update() {
		return this.device[this.updateVersion];
	}
	put(line) {
		let device = this.device;
		system.alert({ 
			type:"stop",
			prompt:"Do you want to update \"" + device.name + "\" to " + this.title + " " + device[this.updateVersion] + "?",
			info:"Please keep the device connected and powered during the update.",
			buttons:["OK", "Cancel"]
		}, ok => {
			if (ok) {
				this.doUpdate();
			}
		});
	}
}

class UpdateSoftwareSetting extends UpdateSetting {
	constructor($) {
		super($);
		this.title = "Kinoma Software";
		this.version = "softwareVersion";
		this.updateVersion = "softwareUpdateVersion";
		this.updateAvailable = "softwareUpdate";
	}
	doUpdate() {
		this.device.updateSoftware(this);
	}
	doUpdateError(status) {
		this.device.updateSoftwareError(this);
	}
	doUpdateFinished(status) {
		this.device.updateSoftwareFinished(this);
	}
	get status() {
		return this.device.updateSoftwareStatus();
	}
}

class UpdateSystemSetting extends UpdateSetting {
	constructor($) {
		super($);
		this.title = "Operating System";
		this.version = "systemVersion";
		this.updateVersion = "systemUpdateVersion";
		this.updateAvailable = "systemUpdate";
	}
	doUpdate() {
		this.device.updateSystem(this);
	}
	doUpdateError() {
		this.device.updateSystemError(this);
	}
	doUpdateFinished() {
		this.device.updateSystemFinished(this);
	}
	get status() {
		return this.device.updateSystemStatus();
	}
}

class ClearCacheSetting {
	constructor($) {
		this.device = $.device;
		this.id = $.id;
		this.title = $.title;
	}
	put(line) {
		if (this.id in this.device)
			this.device[this.id]();
	}
}

class StartupAppSetting {
	constructor($) {
		this.device = $.device;
		this.title = "Startup App";
	}
	load(line) {
		let device = this.device;
		device.getStartupApp().then(json => {
			this.value = json;
			return device.getStartupAppList();
		}).then(json => {
			this.items = json.map(item => {
				let result = new StartupAppItem(item);
				if (this.value.id == result.id)
					this.value = result;
				return result;
			});
			if (!line.container) return;
			line.container.replace(line, new PopupSettingLine(this));
		});
	}
	put() {
		let message = new Message(mergeURI(this.device.url, "settings/startup-app"));
		message.requestText = JSON.stringify(this.value);
		message.method = "PUT"
		message.invoke();
	}
}

class StartupAppItem {
	constructor(item) {
		this.id = item.id;
		this.name = item.title;
	}
	toString() {
		return this.name;
	}
}

class TimezoneSetting {
	constructor($) {
		this.device = $.device;
		this.title = "Timezone";
		this.Template = TimezoneContent;
	}
	get value() {
		return getTimezone(this.device.TIMEZONE.zone);
	}
	load(line) {
		var device = this.device;
		device.getTimezone().then(json => {
			device.TIMEZONE = json;
			if (!line.container) return;
			line.container.replace(line, new DialogSettingLine(this));
		});
	}
}

class CacheSetting {
	constructor($) {
		this.device = $.device;
		this.title = "Data";
		this.Template = CacheContent;
	}
	get value() {
		return "Clear Caches";
	}
}

class CacheContentLineBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onEnter(line) {
		let data = this.data;
		data.put(line);
		shell.focus();
	}
}

var CacheContentLine = Line.template($ => ({
	left:0, right:0, height:40,
	Behavior: CacheContentLineBehavior,
	contents: [
		Label($, { left:0, right:0, style:settingNameStyle, string:$.title }),
		Container($, {
			anchor:"BUTTON", width:80, skin:orangeButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:orangeButtonStyle, string:"Clear" }),
			],
		}),
		Content($, { width:10 }),
	]
}));

var CacheContent = Column.template($ => ({
	left:0, right:0, top:0, height:200,
	contents: [
		CacheContentLine(new ClearCacheSetting({ device:$.device, title: "Kinoma Studio App Cache", id: "clearApps" }), {}),
		CacheContentLine(new ClearCacheSetting({ device:$.device, title: "Apps Preferences", id: "clearAppsPrefs" }), {}),
		CacheContentLine(new ClearCacheSetting({ device:$.device, title: "Cookies", id: "clearCookies" }), {}),
		CacheContentLine(new ClearCacheSetting({ device:$.device, title: "HTTP Cache", id: "clearHTTPCache" }), {}),
		CacheContentLine(new ClearCacheSetting({ device:$.device, title: "Known Wi-Fi Networks", id: "clearKnownNetworks" }), {}),
	]
}));

// ASSETS

import {
	orangeButtonSkin,
	orangeButtonStyle,
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";

import {
	BOLD_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BLACK,
	ORANGE,
	WHITE,
	PASTEL_GRAY,
	PASTEL_ORANGE,
	orangeHeaderSkin,
} from "shell/assets";

import {
	fieldLabelSkin,
	fieldScrollerSkin,
	whiteSkin,
} from "features/devices/assets";

import {
	orangeTileSkin, 
	tileStyle,
	infoStyle,
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

const DEVICE_NAME = "DEVICE_NAME";
const TIMEZONE = "TIMEZONE";
const STARTUP_APP = "STARTUP_APP";
const DEBUGGING = "DEBUGGING";
const UPDATE_APPLICATION = "UPDATE_APPLICATION";
const UPDATE_FIRMWARE = "UPDATE_FIRMWARE";
const CLEAR_CACHES = "CLEAR_CACHES";
const MAC_ADDRESS = "MAC_ADDRESS";
const ABOUT = "ABOUT";
const UPDATE = "UPDATE";

const appTitle = "Settings";

const bodySkin = new Skin({ fill:WHITE, stroke:PASTEL_ORANGE, borders:{ left:1, right:1, top:1, bottom:1 } });

const settingLineSkin = new Skin({ fill: [WHITE, PASTEL_ORANGE, PASTEL_ORANGE, PASTEL_ORANGE], stroke:PASTEL_ORANGE, borders:{ bottom:1 } });
const settingNameStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left", left:10 });
const settingValueStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left", left:10 });
const settingAlertStyle = new Style({ font:NORMAL_FONT, size:14, color:ORANGE, horizontal:"right", right:10 });

const fieldLabelStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"right", left:7 });

const headerTitleStyle = new Style({ font:BOLD_FONT, size:14, color:WHITE, horizontal:"left", left:10 });
const headerSubtitleStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"left", left:10 });

const iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60 });

// BEHAVIORS

import { 
	ButtonBehavior, 
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

import { 
	DropDialog,
	PopupDialog,
	PopupItem,
	popupArrowsSkin,
} from "common/menu";

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	LineBehavior,
} from "shell/behaviors";

import {
	HelperBehavior,
	WaitContent,
} from "features/devices/behaviors";

import { 
	getTimezone,
	TimezoneContent
} from "timezone";

class SettingsTileBehavior extends TileBehavior {
	onDisplaying(container) {
		this.onDeviceChanged(container);
		super.onDisplaying(container);
	}
	onDeviceChanged(container) {
		container.height = 105;
		let content = container.last.previous;
		content.empty();
		if (this.data.device.softwareUpdate || this.data.device.systemUpdate) {
			let banner = new UpdateTileBanner("Update available");
			content.add(banner);
			container.height += banner.height - 5;
		}
		container.container.adjust();
	}
}

class SettingsViewBehavior extends HelperBehavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDeviceHelperUp(container) {
		super.onDeviceHelperUp(container);
		let data = this.data;
		let spinner = data.SPINNER;
		spinner.visible = false;
		spinner.stop();
		let template;
		switch (this.data.device.constructor.tag) {
			case "Element":
				template = ElementSettingsScroller;
				break;
			case "ElementFactory":
				template = ElementFactorySettingsScroller;
				break;
			case "Create":
				template = CreateSettingsScroller;
				break;
			default:
				template = EmbedSettingsScroller;
				break;
		}
		let scroller = new template(this.data);
		container.insert(scroller, spinner);
	}
}

class SettingBehavior extends LineBehavior {
	onFocused(line) {
		this.select(line, true);
	}
	onUnfocused(line) {
		this.select(line, false);
	}
}

class ButtonSettingBehavior extends SettingBehavior {
	changeState(line) {
		super.changeState(line);
		let data = this.data;
		let button = data.BUTTON;
		button.active = button.visible = this.flags != 0;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
	}
	onFocused(line) {
		super.onFocused(line);
		let data = this.data;
		let button = data.BUTTON;
		button.active = button.visible = true;
	}
	onEnter(line) {
		let data = this.data;
		data.put(line);
		shell.focus();
	}
	onTap(line) {
		this.data.BUTTON.focus();
	}
	onUnfocused(line) {
		debugger
		super.onUnfocused(line);
		let data = this.data;
		let button = data.BUTTON;
		button.active = button.visible = false;
	}
}

class DialogSettingBehavior extends SettingBehavior {
	changeState(line) {
		super.changeState(line);
		let arrow =	line.first.next.last;
		arrow.visible = this.flags != 0;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
		this.data.LABEL.string = this.data.value;
	}
	onDialogClosed(line, item) {
		this.dialog = null;
		this.data.LABEL.string = this.data.value;
		shell.focus();
	}
	onTap(line) {
		let data = {
			Template:this.data.Template,
			button:line.first.next,
			device:this.data.device,
			variant:0,
		};
		this.dialog = new DropDialog(data);
		shell.add(this.dialog);
		line.focus();
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
}

class FieldSettingBehavior extends SettingBehavior {
	onCreate(line, data) {
		super.onCreate(line, data);
		data.FIELD.string = data.value;
	}
	onDialogClosed(line, item) {
		this.dialog = null;
		this.data.FIELD.string = this.data.value;
		shell.focus();
	}
	onTap(line) {
		let data = this.data;
		let description = {
			Template:data.Template,
			button:line,
			device:data.device,
			title:data.title,
			value:data.value,
			variant:1,
		};
		this.dialog = new DropDialog(description);
		shell.add(this.dialog);
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
}

class FieldSettingLabelBehavior extends FieldLabelBehavior {
	onFocused(label) {
		super.onFocused(label);
		let setting = label.container.container;
		setting.behavior.onFocused(setting);
	}
	onUnfocused(label) {
		let setting = label.container.container;
		setting.behavior.onUnfocused(setting);
	}
}

class PopupSettingBehavior extends SettingBehavior {
	changeState(line) {
		super.changeState(line);
		let arrow =	line.first.next.first.next;
		arrow.visible = arrow.next.visible = this.flags != 0;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
		data.LABEL.string = data.value;
	}
	onMenuSelected(line, item) {
		let data = this.data;
		if (item) {
			data.value = item;
			data.put();
			data.LABEL.string = data.value;
		}
		shell.focus();
	}
	onTap(line) {
		this.changeState(line, 1);
		let data = this.data;
		let description = {
			ItemTemplate:PopupItem,
			button:line.first.next,
			items:data.items,
			value:data.value,
			context:shell,
		};
		shell.add(new PopupDialog(description));
		line.focus();
	}
}

class URLSettingBehavior extends SettingBehavior {
	onCreate(line, data) {
		super.onCreate(line, data);
	}
	onDisplaying(line) {
		this.data.load(line);
	}
}

class UpdateSettingBehavior extends ButtonSettingBehavior {
	onCreate(line, data) {
		super.onCreate(line, data);
		this.onDeviceChanged(line, data.device);
	}
	onDeviceChanged(line, device) {
		let data = this.data;
		data.BUTTON.visible = false;
		var active = device[this.data.updateAvailable] && !device.isSimulator();
		data.LABEL.string = data.value;
		line.active = active;
		data.UPDATE.string = active ? "UPDATE" : "";
	}
}

class ValueSettingBehavior extends SettingBehavior {
	onCreate(line, data) {
		super.onCreate(line, data);
		data.LABEL.string = data.value;
	}
}

// TEMPLATES

import {
	SpinnerContent,
} from "features/devices/behaviors";

import {
	AppViewHeader,
} from "features/devices/viewer";

const testSkin = new Skin({ fill:"red" });
const bannerSkin = new Skin({ fill:ORANGE });
const bannerStyle = new Style({size:"14", color:WHITE });

const UpdateTileBanner = Container.template($ => ({
	left:0, right:0, height:30, bottom:0,
	contents: [
		Content($, { left:10, right:10, top:0, height:1, skin:whiteSkin }),
		Label($, { left:1, right:1, bottom:6, style:bannerStyle, string:$})
	]
}));

export const SettingsTile = Container.template($ => ({
	left:0, top:0, height:105, skin:orangeTileSkin, style:tileStyle,
	Behavior: SettingsTileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:70, string:appTitle }),
		Container($, { left:0, right:0, top:0, bottom:0 }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin, variant:1 }),
	]
}));

export const SettingsView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:bodySkin,
	Behavior: SettingsViewBehavior,
	contents: [
		SpinnerContent($, { anchor:"SPINNER" }),
		AppViewHeader({ skin:orangeHeaderSkin, title:appTitle, device:$.device }),
	],
}));

var CreateSettingsScroller = Scroller.template($ => ({
	left:1, right:1, top:60, bottom:0, clip:true, active:true,
	Behavior:ScrollerBehavior,
	contents: [
		Column($, {
			width:640, top:0,
			contents: [
				Separator($, {}),
				FieldSettingLine(new DeviceNameSetting($), {}),
				URLSettingLine(new TimezoneSetting($), {}),
				URLSettingLine(new StartupAppSetting($), {}),
// 				Separator($, {}),
				UpdateSettingLine(new UpdateSoftwareSetting($), {}),
				UpdateSettingLine(new UpdateSystemSetting($), {}),
				URLSettingLine(new MACAddressSetting($), {}),
// 				Separator($, {}),
				DialogSettingLine(new CacheSetting($), {}),
			]
		}),
		HorizontalScrollbar($, { bottom:-10 }),
		VerticalScrollbar($, { right:-10 }),
	],
}));

var ElementSettingsScroller = Scroller.template($ => ({
	left:1, right:1, top:60, bottom:0, clip:true, active:true,
	Behavior:ScrollerBehavior,
	contents: [
		Column($, {
			width:640, top:0,
			contents: [
				Separator($, {}),
				FieldSettingLine(new DeviceNameSetting($), {}),
				URLSettingLine(new TimezoneSetting($), {}),
// 				Separator($, {}),
				UpdateSettingLine(new UpdateSoftwareSetting($), {}),
				UpdateSettingLine(new UpdateSystemSetting($), {}),
				URLSettingLine(new MACAddressSetting($), {}),
			]
		}),
		HorizontalScrollbar($, { bottom:-10 }),
		VerticalScrollbar($, { right:-10 }),
	],
}));

var ElementFactorySettingsScroller = Scroller.template($ => ({
	left:1, right:1, top:60, bottom:0, clip:true, active:true,
	Behavior:ScrollerBehavior,
	contents: [
		Column($, {
			width:640, top:0,
			contents: [
				Separator($, {}),
				FieldSettingLine(new DeviceNameSetting($), {}),
// 				Separator($, {}),
				UpdateSettingLine(new UpdateSoftwareSetting($), {}),
			]
		}),
		HorizontalScrollbar($, { bottom:-10 }),
		VerticalScrollbar($, { right:-10 }),
	],
}));

var EmbedSettingsScroller = Scroller.template($ => ({
	left:1, right:1, top:60, bottom:0, clip:true, active:true,
	Behavior:ScrollerBehavior,
	contents: [
		Column($, {
			width:640, top:0,
			contents: [
				Separator($, {}),
				FieldSettingLine(new DeviceNameSetting($), {}),
				URLSettingLine(new TimezoneSetting($), {}),
				URLSettingLine(new StartupAppSetting($), {}),
// 				Separator($, {}),
				UpdateSettingLine(new UpdateSoftwareSetting($), {}),
				URLSettingLine(new MACAddressSetting($), {}),
				DialogSettingLine(new CacheSetting($), {}),
			]
		}),
		HorizontalScrollbar($, { bottom:-10 }),
		VerticalScrollbar($, { right:-10 }),
	],
}));

var Separator = Content.template($ => ({ left:0, right:0, height:10 }));

var ButtonSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin, active:true,
	Behavior: ButtonSettingBehavior,
	contents: [
		Label($, { left:0, right:0, style:settingNameStyle, string:$.title }),
		Container($, {
			anchor:"BUTTON", width:80, skin:whiteButtonSkin, active:false, visible:false, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Clear" }),
			],
		}),
		Content($, { width:80 }),
	],
}));

var DialogSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin, active:true,
	Behavior: DialogSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Container($, { left:0, right:0, skin:whiteButtonSkin, state:1,
			contents: [
				Label($, { left:0, right:0, anchor:"LABEL", style:settingValueStyle }),
				Content($, { right:5, skin:popupArrowsSkin, state:1, visible:false }),
			]
		}),
		Content($, { width:160 }),
	],
}));

var FieldSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin, active:true,
	Behavior: FieldSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Scroller($, {
			left:3, right:3, top:5, bottom:5, skin: fieldScrollerSkin, clip:true,
			contents: [
				Label($, {
					anchor:"FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:fieldLabelStyle,
				}),
			],
		}),
		Content($, { width:160 }),
	],
}));

var PopupSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin, active:true,
	Behavior: PopupSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Container($, { left:0, right:0, skin:whiteButtonSkin, state:1,
			contents: [
				Label($, { left:0, right:0, anchor:"LABEL", style:settingValueStyle }),
				Content($, { right:5, top:5, skin:popupArrowsSkin, state:0, visible:false }),
				Content($, { right:5, bottom:5, skin:popupArrowsSkin, state:1, visible:false }),
			]
		}),
		Content($, { width:160 }),
	],
}));

var URLSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin,
	Behavior: URLSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Content($, { width:10 }),
		WaitContent($, { }),
	],
}));

var UpdateSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin,
	Behavior: UpdateSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Label($, { anchor:"LABEL", left:0, right:0, style:settingValueStyle }),
		Container($, {
			anchor:"BUTTON", width:80, skin:whiteButtonSkin, active:false, visible:false, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Apply" }),
			],
		}),
		Content($, { width:30 }),
		Label($, { anchor:"UPDATE", style:settingAlertStyle, string:"" }),
	],
}));

var ValueSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin,
	Behavior: ValueSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Label($, { anchor:"LABEL", left:0, right:0, style:settingValueStyle }),
		Container($, {
			anchor:"BUTTON", width:80, skin:whiteButtonSkin, active:false, visible:false, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Apply" }),
			],
		}),
		Content($, { width:80 }),
	],
}));
