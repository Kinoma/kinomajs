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

import {
	Feature
} from "shell/feature";

import {
	Viewer
} from "shell/viewer";

export default class extends Feature {
	constructor(model, url) {
		super(model, "Net Scanner");
		this.Template = NetScannerPane;
		this.iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60, states:60 });
		this.viewers = [
			new NetScannerViewer(this),
		];
		let map = Files.readJSON(mergeURI(url, "schemas.json"));
		this.browsers = [];
		this.categories = {};
		this.ssdp = {
			schemas: map.ssdp
		};
		this.zeroconf = {
			browserServiceType: undefined,
			schemas: map.zeroconf
		};
		var ssdp = this.ssdp;
		if (ssdp) {
			this.client = new SSDP.Client();
			this.client.behavior = this;
			this.client.start();
		}
		var zeroconf = this.zeroconf;
		if (zeroconf) {
			this.browser = new Zeroconf.Browser(zeroconf.browserServiceType);
			this.browser.behavior = this;
			this.browser.start();
		}
		this.menu = {
			data: {
				action: "/command?sort=",
				items: [
					{ title: "Device Type",         value: "onBrowserCompareDeviceType" },
					{ title: "IP Address",          value: "onBrowserCompareIPAddress"  },
				],
				selection: 1 // IP Address default selection
			}
		}
	}
	idle(now) {
		// ping
	}
	notify() {
		this.dirty = true;
		shell.distribute("onFeatureChanged", this);
	}
	getKind(schema, schemas, unknown) {
		if (schema in schemas) {
			return schemas[schema] || schema;
		}
		var schemaLowerCase = schema.toLowerCase();
		for (var key in schemas) {
			if (key.toLowerCase() == schemaLowerCase) {
				return schemas[key] || schema;
			}
		}
		trace("schema not mapped: " + schema + "\n");
		return unknown || "Unknown Device";
	}
	getSSDPKind(schema) {
		return this.getKind(schema, this.ssdp.schemas, "Unknown SSDP Device");
	}
	getZeroconfKind(schema) {
		return this.getKind(schema, this.zeroconf.schemas, "Unknown Zeroconf Device");
	}
	onBrowserCompareDeviceType(a, b) {
		var value = a.$temp.kindSort ? a.$temp.kindSort.compare(b.$temp.kindSort) : 0;
		return (value == 0) ? a.$temp.ipAddressSort.compare(b.$temp.ipAddressSort) : value;
	}
	onBrowserCompareIPAddress(a, b) {
		var value = a.$temp.ipAddressSort.compare(b.$temp.ipAddressSort);
		return (value == 0) ? a.$temp.kindSort ? a.$temp.kindSort.compare(b.$temp.kindSort) : value : value;
	}
	onSortBrowserChanged(value) {
		var data = this.menu.data;
		if (value) {
			var selection = data.selection;
			data.items.forEach(function (item, index) {
				if (item.value == value) {
					selection = index;
				}
			});
			data.selection = selection;
		}
		var browsers = this.browsers;
		let selector;
		switch (data.selection) {
			case 0:
				browsers.sort(this.onBrowserCompareDeviceType);
				selector = "kindSort";
				break;
			case 1:
				browsers.sort(this.onBrowserCompareIPAddress);
				selector = "ipAddressSort";
				break;
			default:
				debugger
				break;
		}
		var categories = this.categories;
		for (let key in categories) {
			categories[key].items = [];
		}
		browsers.forEach(function (browser) {
			let key = browser.$temp[selector];
			if (!(key in categories)) {
				categories[key] = {
					expanded: true,
					items: [],
					selection: data.selection
				};
			}
			categories[key].items.push(browser);
		});
		for (let key in categories) {
			if (!categories[key].items.length)
				delete categories[key];
		}
		shell.distribute("onBrowserChanged", categories);
	}
	onSSDPServerDown(server) {
//			trace("onSSDPServerDown: " + JSON.stringify(server) + "\n");
		var browser = Object.create(server);
		browser.$id = browser.uuid + "/" + browser.interface;
		var browsers = this.browsers;
		for (var i = browsers.length; i--;) {
			if (browsers[i].$id == browser.$id) {
				browsers.splice(i, 1);
				this.onSortBrowserChanged();
				break;
			}
		}
	}
	onSSDPServerUp(server) {
//			trace("onSSDPServerUp: " + JSON.stringify(server) + "\n");
		var browser = Object.create(server);
		browser.$id = browser.uuid + "/" + browser.interface;
		var browsers = this.browsers;
		for (var i = browsers.length; i--;) {
			if (browsers[i].$id == browser.$id) {
				browser.$temp = browsers[i].$temp;
				browsers[i] = browser;
				return;
			}
		}
		var ipAddress = parseURI(browser.url).authority.split(":")[0];
		var ipAddressSort = ipAddress.split(".").map(function (value) {
			return ("00" + parseInt(value, 10)).slice(-3);
		}).join(".");
		var kind = this.getSSDPKind(browser.type);
		var kindSort = kind ? kind.toLowerCase() : undefined;
		browser.$temp = {
			discovered: Date.now(),
			friendlyName: "",
			ipAddress: ipAddress,
			ipAddressSort: ipAddressSort,
			kind: kind,
			kindSort: kindSort,
			ssdp: true,
			thumbnailURL: null,
			when: null
		};
		browsers.push(browser);
		this.onSortBrowserChanged();
	}
	onZeroconfServiceDown(service) {
//			trace("onZeroconfServiceDown: " + JSON.stringify(service) + "\n");
		var browser = Object.create(service);
		browser.$id = browser.type + "/" + browser.name;
		var browsers = this.browsers;
		for (var i = browsers.length; i--;) {
			if (browsers[i].$id == browser.$id) {
				browsers.splice(i, 1);
				this.onSortBrowserChanged();
				break;
			}
		}
	}
	onZeroconfServiceUp(service) {
//			trace("onZeroconfServiceUp: " + JSON.stringify(service) + "\n");
		var browser = Object.create(service);
		browser.$id = browser.type + "/" + encodeURIComponent(browser.name);
		var browsers = this.browsers;
		for (var i = browsers.length; i--;) {
			if (browsers[i].$id == browser.$id) {
				browser.$temp = browsers[i].$temp;
				browsers[i] = browser;
				return;
			}
		}
		var ipAddress = browser.ip;
		var ipAddressSort = ipAddress.split(".").map(function (value) {
			return ("00" + parseInt(value, 10)).slice(-3);
		}).join(".");
		var kind = this.getZeroconfKind(browser.type);
		var kindSort = kind ? kind.toLowerCase() : undefined;
		browser.$temp = {
			discovered: Date.now(),
			friendlyName: browser.name,
			ipAddress: ipAddress,
			ipAddressSort: ipAddressSort,
			kind: kind,
			kindSort: kindSort,
			thumbnailURL: null,
			when: null,
			zeroconf: true
		};
		browsers.push(browser);
		this.onSortBrowserChanged();
	}
}

class NetScannerViewer extends Viewer {
	constructor(feature) {
		super(feature);
		this.Template = NetScannerView;
		this.scheme = "netscanner";
	}
	accept(parts, url) {
		return parts.scheme == this.scheme;
	}
	create(parts, url) {
		let id = parts.authority + parts.path;
		for (let browser of this.feature.browsers) {
			if (browser.$id == id)
				return new this.Template(browser);
		}
		return new ErrorView({ url, error:"Not found!" });
	}
};

// ASSETS

import {
	BOLD_FONT,
	BUTTON_DISABLED,
	BUTTON_ENABLED,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	WHITE,
	fileGlyphsSkin,
	grayBorderSkin,
	grayFooterSkin,
	grayHeaderSkin,
	greenHeaderSkin,
	greenLineSkin,
	tableLineStyle,
	tableHeaderStyle,
	whiteButtonSkin,
	whiteButtonsSkin,
} from "shell/assets";

// BEHAVIORS

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import { 
	HolderColumnBehavior,
	HolderContainerBehavior,
	LineBehavior,
	HeaderBehavior,
	TableBehavior,
} from "shell/behaviors";

import { 
	FeaturePaneBehavior,
} from "shell/feature";

class NetScannerPaneBehavior extends FeaturePaneBehavior {
	onDisplaying(container) {
		let data = this.data;
		container.distribute("onBrowserChanged", data.categories);
	}
};

// TEMPLATES

class BrowserHeaderBehavior extends HeaderBehavior {
};

class BrowserColumnBehavior extends HolderColumnBehavior {
	onBrowserChanged(column, categories) {
		column.empty(0);
		let keys = Object.keys(categories);
		keys.sort();
		for (let key of keys) {
			let category = categories[key];
			let temp = category.items[0].$temp;
			key = category.selection ? temp.ipAddress : temp.kind;
			let table = new BrowserTable({ key, category });
			column.add(table);
		}
	}
};

class BrowserTableBehavior extends TableBehavior {
	addLines(column) {
		let header = column.first;
		let category = this.data.category;
		column.empty(1);
		if (category && category.expanded) {
			header.behavior.expand(header, true);
			category.items.forEach(data => column.add(new (this.lineTemplate)(data)));
// 			if (view.lineIndex >= 0) {
// 				let line = column.content(view.lineIndex + 1);
// 				line.behavior.select(line, true);
// 			}
		}
		else
			header.behavior.expand(header, false);
		column.add(new this.footerTemplate(data));
	}
	hold(column) {
		let header = column.first;
		let result = BrowserHeader(this.data, {left:0, right:0, top:0, height:header.height, skin:header.skin});
		let category = this.data.category;
		result.behavior.expand(result, category && category.expanded);
		result.last.string = header.last.string;
		return result;
	}
	onCreate(column, data) {
		this.data = data;
		this.footerTemplate = BrowserFooter;
		this.lineTemplate = data.category.selection ? BrowserKindLine : BrowserIPLine;
		this.addLines(column);
		column.HEADER.last.string = data.key;
	}
	onBrowserChanged(column) {
		this.addLines(column);
	}
	toggle(column) {
		var category = this.data.category;
		if (category)
			category.expanded = !category.expanded;
		this.addLines(column);
	}
	trigger(column, line) {
	}
};

class BrowserLineBehavior extends LineBehavior {
	onComplete(line, message, document) {
		var data = this.data, $temp = data.$temp;
		var friendlyName = $temp.friendlyName || "";
		var thumbnailURL = $temp.thumbnailURL || "";
		if (document) {
			var device = document.getElementsByTagName("device").item(0);
			if (device) {
				friendlyName = device.getElementsByTagName("friendlyName").item(0).firstChild.nodeValue;
				var iconList = device.getElementsByTagName("iconList").item(0);
				if (iconList) {
					var icons = iconList.getElementsByTagName("icon");
					for (var i = 0; i < icons.length; i++) {
						var icon = icons.item(i);
						var mime = icon.getElementsByTagName("mimetype").item(0).firstChild.nodeValue;
						if (("image/jpeg" == mime) || ("image/png" == mime)) {
							var thumbnailURL = mergeURI(message.url, icon.getElementsByTagName("url").item(0).firstChild.nodeValue);
							break;
						}
					}
				}
			}
		}
		$temp.text = document ? DOM.serialize(document) : undefined; // not available
		line.first.last.string = $temp.friendlyName = friendlyName;
 		line.last.url = $temp.thumbnailURL = thumbnailURL;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
		if (("url" in data) && data.url) {
//@@					if (data.url.indexOf("rootDesc.xml") < 0)
				let message = new Message(data.url);
				message.setRequestHeader("Connection", "Close");
				line.invoke(message, Message.DOM);
		}
	}
	onTap(line) {
		shell.delegate("doOpenURL", "netscanner://" + this.data.$id);
	}
};

class NetScannerColumnBehavior extends Behavior {
	onCreate(column, browser) {
		super.onCreate(column, browser);
		if ("friendlyName" in browser.$temp)
			column.add(new KeyValueLine({ key:"Name", value:browser.$temp.friendlyName }));
		column.add(new KeyValueLine({ key:"Type", value:browser.type }));
		if ("uuid" in browser)
			column.add(new KeyValueLine({ key:"UUID", value:browser.uuid }));
		if ("host" in browser)
			column.add(new KeyValueLine({ key:"Host", value:browser.host }));
		if ("url" in browser)
			column.add(new KeyValueLine({ key:"URL", value:browser.url }));
		else
			column.add(new KeyValueLine({ key:"URL", value:"//" + browser.ip + ":" + browser.port }));
		if ("services" in browser) {
			column.add(new DividerLine());
			column.add(new KeyLine("Services"));
			if (browser.services.length) {
				for (let service of browser.services) {
					column.add(new ValueLine(service));
				}
			}
			else {
				column.add(new ValueLine("none"));
			}
		}
		if ("text" in browser.$temp) {
			column.add(new DividerLine());
			column.add(new KeyLine("Device Description"));
			if (browser.$temp.text) {
				for (let text of browser.$temp.text.split("\n")) {
					column.add(new ValueLine(text));
				}
			}
			else {
				column.add(new ValueLine("not available"));
			}
		}
	}
}

var NetScannerPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true, style:tableLineStyle,
	Behavior: NetScannerPaneBehavior,
	contents: [
		NetScannerHeader($),
		Scroller($, {
			left:0, right:0, top:65, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				Column($, {
					left:10, right:10, top:0, Behavior:BrowserColumnBehavior, 
					contents: [
					]
				}),
				VerticalScrollbar($, {}),
			]
		}),
		Container($, {
			left:10, right:10, top:65, height:40, clip:true, Behavior:HolderContainerBehavior,
		}),
	],
}));

import {
	ErrorView
} from "shell/viewer";

var BrowserTable = Column.template($ => ({
	left:0, right:0, active:true, clip:true, Behavior:BrowserTableBehavior,
	contents: [
		BrowserHeader($, { name:"HEADER" }),
	],
}));

import { 
	ButtonBehavior, 
} from "common/control";

const headerLineStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:WHITE, horizontal:"left" });
const browserTitleStyle = new Style({ font:BOLD_FONT, size:14, color:[ "#cdcdcd", "white", "white", "white" ] })
const whiteButtonStyle = new Style({ font:NORMAL_FONT, size:14, color:[BUTTON_DISABLED, WHITE, WHITE, BUTTON_ENABLED ], horizontal:"center" });
var menuHeaderAddressStyle = new Style({ font:NORMAL_FONT, size:12, color:WHITE, horizontal:"left", right:10 })

var NetScannerHeader = Line.template(function($) { return {
	left:10, right:10, top:0, height:60, active:false, skin:grayHeaderSkin,
	contents: [
		Container($, {
			width:60, height:60,
			contents: [
				Picture($, {
					width:60, height:60, url:"../devices/assets/searching.png",
					Behavior: class extends Behavior {
						onCreate(picture) {
							picture.opacity = 0.66;
							picture.origin = { x:30, y:30 };
						}
						onDisplayed(picture) {
							picture.start();
						}
						onTimeChanged(picture) {
							var rotation = picture.rotation;
							rotation += 1;
							if (rotation > 360) rotation = 0;
							picture.rotation = rotation;
						}
					},
				}),
			],
		}),
		Content($, { width:10 }),
		Column($, {
			left:0, right:0, height:40,
			contents: [
				Label($, { left:0, right:0, style:tableHeaderStyle, string:"NET SCANNER" }),
				Line($, {
					left:0, right:0, height:20,
					contents: [
						Label($, { left:0, right:0, style:menuHeaderAddressStyle, string:model.SSID }),
						Container($, {
							width:110, skin:whiteButtonSkin, active:true, visible:true,
							Behavior: class extends ButtonBehavior {
								onCreate(button, $) {
									super.onCreate(button, $);
									this.update(button);
								}
								onTap(button) {
									let data = this.data;
									data.onSortBrowserChanged(data.menu.data.selection ? "onBrowserCompareDeviceType" : "onBrowserCompareIPAddress");
									this.update(button);
								}
								update(button) {
									button.first.string = $.menu.data.selection ? "Sort by Type" : "Sort by IP";
								}
							},
							contents: [
								Label($, { left:0, right:0, style:whiteButtonStyle, string:"Sort by IP" }),
							],
						}),
					],
				}),
			],
		}),
	],
}});

var BrowserHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:greenHeaderSkin, active:true,
	Behavior: BrowserHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, variant:1 }),
		Label($, { left:0, right:0, style:headerLineStyle }),
	],
}});

var BrowserFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:grayFooterSkin,
}});

var lineHeight = 42;
var kindStyle = new Style({ font:SEMIBOLD_FONT, size:12, color: "black" });
var nameStyle = new Style({ font:NORMAL_FONT, size:12, color: "black" });

var BrowserIPLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:greenLineSkin, active:true,
	Behavior:BrowserLineBehavior,
	contents: [
		Column($, {
			left:5, right:5, top:5, bottom:5, 
			contents: [
				Label($, { left:0, style:kindStyle, string:$.$temp.ipAddress }),
				Label($, { left:0, style:nameStyle, string:$.$temp.friendlyName }),
			],
		}),
		Thumbnail($, { right:5, width:32, height:32, url:$.$temp.thumbnailURL, aspect:"fit" }),
	]
}});

var BrowserKindLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:greenLineSkin, active:true, 
	Behavior:BrowserLineBehavior,
	contents: [
		Column($, {
			left:5, right:5, top:5, bottom:5, 
			contents: [
				Label($, { left:0, style:kindStyle, string:$.$temp.kind }),
				Label($, { left:0, style:nameStyle, string:$.$temp.friendlyName }),
			],
		}),
		Thumbnail($, { right:5, width:32, height:32, url:$.$temp.thumbnailURL, aspect:"fit" }),
	]
}});

var NetScannerView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:grayBorderSkin,
	contents: [
		Scroller($, {
			left:1, right:1, top:30, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				Column($, { left:0, top:0, Behavior:NetScannerColumnBehavior }),
				HorizontalScrollbar($, { bottom:-10 }),
				VerticalScrollbar($, { right:-10 }),
			],
		}),
		Line($, {
			left:0, right:0, top:0, height:30, skin:greenHeaderSkin,
			contents: [
				Label($, { left:10, right:0, height:30, style: tableHeaderStyle, string:$.$temp.ssdp ? "SSDP Details" : "Zeroconf Details" }),
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

var DividerLine = Container.template($ => ({
	left:0, right:0, height:10,
	contents: [
		Content($, { left:0, right:0, top:5, height:1, skin:grayBorderSkin }),
	]
}));

var infoHeight = 24;
var KeyValueLine = Line.template($ => ({
	left:10, right:10, height:infoHeight,
	contents: [
		Label($, { style:kindStyle, string:$.key + ': ' }),
		Label($, { style:nameStyle, string:$.value }),
	]
}));

var KeyLine = Label.template($ => ({
	left:10, height:18, style:kindStyle, string:$ + ': ',
}));

var ValueLine = Label.template($ => ({
	left:20, height:16, style:nameStyle, string:$,
}));
