//@module
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
THEME = require("themes/sample/theme");
var theme = require("themes/flat/theme");
for( var i in theme )
	THEME[i] = theme[i];
var PinsSimulators = require("PinsSimulators");

var SCROLLER = require("mobile/scroller");

var buttonTexture = new Texture("./assets/button.png", 1);
var buttonSkin = new Skin({ texture:buttonTexture, x:0, y:0, width:60, height:60, states:60, variants:60 });
var deviceTexture = new Texture("./assets/device.png", 2);
var deviceSkin = new Skin({ texture:deviceTexture, x:0, y:0, width:250, height:250 });

var darkGraySkin = new Skin({ fill:"#555555" });
var graySkin = new Skin({ fill:"gray" });
var kinomaSkin = new Skin({ fill:"#7bc042" });
var whiteSkin = new Skin({ fill:"white" });

var smallTitleStyle = new Style({ font:"bold", size:16, color:"white", horizontal:"left" });
var largeTitleStyle = new Style({ font:"bold", size:24, color:"white", horizontal:"left" });

var pinsTexture = new Texture("./assets/pins.png", 1);
var pinsSkins = [
	new Skin({ texture:pinsTexture, x:0, y:0, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:20, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:40, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:60, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:80, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:100, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:120, width:80, height:20, variants:80 }),
	new Skin({ texture:pinsTexture, x:0, y:140, width:80, height:20, variants:80 }),
];
var pinNumberStyle = new Style({ font:"bold", size:14, color:"white", top:2 });

class PinContainerBehavior extends Behavior {
	onConfigure(container, indexes) {
		container.skin = pinsSkins[indexes[this.number]];
	}
	onCreate(line, number) {
		this.number = number;
	}
};

var LeftPinContainer = Container.template($ => ({
	width:80, height:24, Behavior:PinContainerBehavior, skin:pinsSkins[0], variant:0,
	contents:[
		Label($, { width:20, right:0, height:20, style:pinNumberStyle, string:$ }),
	],
}));

var RightPinContainer = Container.template($ => ({
	width:80, height:24, Behavior:PinContainerBehavior, skin:pinsSkins[0], variant:1,
	contents:[
		Label($, { left:0, width:20, height:20, style:pinNumberStyle, string:$ }),
	],
}));

var Window = Line.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, { 
			width:360, top:0, bottom:0, skin:whiteSkin, clip:true,
			contents: [ 
				PinsSimulators.PartsScroller($, { top:60 }),
				Container($, { 
					left:0, width:360, top:0, height:60, skin:kinomaSkin,
					contents: [ 
						Column($, {
							left:10, right:0,
							contents:[
								Label($, { left:0, right:0, style:smallTitleStyle, string:"Element" }),
								Label($, { left:0, right:0, style:largeTitleStyle, string:"Hardware Pins Simulators" }),
							]
						}),
						Content($, {
							right:0, top:0, skin:buttonSkin, active:true,
							Behavior: class extends Behavior {
								onCreate(container, data) {
									this.data = data;
								}
								onTouchBegan(container) {
									container.state = 1;
								}
								onTouchEnded(container) {
									var log = this.data.LOG;
									if (log.visible)
										container.container.run(new CloseLogTransition, log, container);
									else
										container.container.run(new OpenLogTransition, log, container);
								}
							},
						}),
					] 
				}),
			] 
		}),
		MessageList($, { anchor:"LOG", width:240, top:0, bottom:0 }),
		Container($, { 
			left:0, right:0, top:0, bottom:0, skin:graySkin,
			contents: [ 
				Line($, {
					contents: [ 
						Column($, {
							width:80, 
							contents: [ 1, 2, 3, 4, 5, 6, 7, 8 ].map(number => new LeftPinContainer(number)),
						}),
						Content($, { skin:deviceSkin }),
						Column($, {
							width:80, 
							contents: [ 9, 10, 11, 12, 13, 14, 15, 16 ].map(number => new RightPinContainer(number)),
						}),
					],
				}),
			] 
		}),
	],
}));

class CloseLogTransition extends Transition {
	constructor() {
		super(250);
	}
	onBegin(container, log, button) {
		this.log = log;
		button.active = false;
	}
	onEnd(container, log, button) {
		log.visible = false;
		button.active = true;
		button.state = 0;
		button.variant = 1;
	}
	onStep(fraction) {
		this.log.width = 240 * (1 - Math.quadEaseIn(fraction));
	}
};

class OpenLogTransition extends Transition {
	constructor() {
		super(250);
	}
	onBegin(container, log, button) {
		this.log = log;
		log.visible = true;
		button.active = false;
	}
	onEnd(container, log, button) {
		button.active = true;
		button.state = 0;
		button.variant = 0;
	}
	onStep(fraction) {
		this.log.width = 240 * Math.quadEaseOut(fraction);
	}
};

var messageTexture = new Texture("./assets/bubbles.png", 1);
var messageErrorSkin = new Skin({ texture: messageTexture, x:200, y:0, width:100, height:64, 
	tiles:{ left:20, right:20, top:20, bottom:20 }, 
	margins:{ left:20, right:20, top:10, bottom:10 } 
});
var messageInSkin = new Skin({ texture: messageTexture, x:100, y:0, width:100, height:64, 
	tiles:{ left:20, right:20, top:20, bottom:20 }, 
	margins:{ left:20, right:20, top:10, bottom:10 } 
});
var messageOutSkin = new Skin({ texture: messageTexture, x:0, y:0, width:100, height:64, 
	tiles:{ left:20, right:20, top:20, bottom:20 }, 
	margins:{ left:20, right:20, top:10, bottom:10 } 
});
var messageListSkin = new Skin({ fill:"#e2e2e2" });
var messageListStyle = new Style({ font: "12px Menlo", horizontal:"left" });
var messagePathStyle = new Style({ font: "bold" });

var MessageList = Container.template($ =>  ({
	skin:messageListSkin, style:messageListStyle,
	Behavior: class extends Behavior {
		add(container, line) {
			var scroller = container.first;
			var column = scroller.first;
			var flag = scroller.scroll.y >= (column.height - scroller.height);
			if (column.length == 1024)
				column.remove(column.first);
			column.add(new Content({ height:10 }));
			column.add(line);
			column.add(new Content({ height:10 }));
			if (flag)
				scroller.scrollTo(0, 0x7FFFFFFF);
		}
		addInput(container, path, object) {
			var text = new Text({ right:20, skin:messageInSkin });
			var format = [{ style:messagePathStyle, string: path }];
			if (object)
				format.push({ string: JSON.stringify(object, null, 2) });
			text.format(format);
			this.add(container, text);
		}
		onCreate(container) {
			//this.log = "";
		}
		onPinsClose(container) {
			var scroller = container.first;
			var column = scroller.first;
			column.empty();
		}
		onPinsError(container, path, object, error) {
			this.addInput(container, path, object);
			this.add(container, new Text({ left:20, skin:messageErrorSkin, string:error }));
		}
		onPinsInvoke(container, path, object, result) {
			this.addInput(container, path, object);
			if (result !== undefined)
				this.add(container, new Text({ left:20, skin:messageOutSkin, string:JSON.stringify(result, null, 2) }));
		}
		onPinsRepeat(container, path, object, result) {
			this.addInput(container, path, object);
			this.add(container, new Text({ left:20, skin:messageOutSkin, string:JSON.stringify(result, null, 2) }));
		}
	},
	contents: [
		SCROLLER.VerticalScroller($, {
			left:0, right:0, top:0, bottom:0, clip:true, 
			contents: [
				Column($, {
					left:0, right:0, top:0,
				}),
			],
		}),
	],
}));

class ShellBehavior extends Behavior {
	addSimulatorPart(shell, data) {
		var scroller = shell.first.first.first;
		return scroller.delegate( "addPart", data );
	}
	removeSimulatorPart(shell, container) {
		var scroller = shell.first.first.first;
		scroller.partsContainer.remove(container);
	}
	canAbout() {
		return true; 
	}
	canBreakApplication() {
		return true; 
	}
	canBreakShell() {
		return true; 
	}
	canPurgeApplication() {
		return true; 
	}
	canPurgeShell() {
		return true; 
	}
	canQuit() {
		return true; 
	}
	doAbout() {
		shell.alert("about", "Kinoma Element Simulator", "Copyright © 2016 Marvell. All rights reserved. Kinoma is a registered trademark of Kinoma, Inc."); 
	}
	doBreakApplication() {
		this.host.debugger(); 
	}
	doBreakShell() {
		debugger; 
	}
	doPurgeApplication() {
		this.host.purge(); 
	}
	doPurgeShell() {
		shell.purge(); 
	}
	doQuit() {
		shell.quit(); 
	}
	onCreate() {
		this.preferences = "elementShell.json";
		var modulePath = getEnvironmentVariable("modulePath");
		modulePath += ";" + mergeURI(Files.documentsDirectory, "../tmp/mc/k0/");
		setEnvironmentVariable("modulePath", modulePath);
		this.host = new ElementHost(getEnvironmentVariable("archivePath"));
		shell.add(new Window(this));
		this.readPreferences();
		shell.updateMenus();
		this.host.launch();
		shell.interval = 25;
		shell.start();
	}
	onInvoke(shell, message) {
		if (message.name == "quit")
			shell.quit(); 
	}
	onPinsClose(shell) {
		var states = new Array(32);
		states.fill(0);
		shell.last.distribute("onConfigure", states);
		this.LOG.behavior.onPinsClose(this.LOG);
	}
	onPinsConfigure(shell, configurations) {
		var states = new Array(16);
		states.fill(0);
		for (var i in configurations) {
			var pins = configurations[i].pins;
			for (var j in pins) {
				var pin = pins[j];
				switch(pin.type) {
				case "A2D":
				case "Analog":
					if ("pin" in pin)
						states[pin.pin] = 3;
					break;
				case "Digital":
					if ("direction" in pin) {
						var direction = pin.direction;
						if (direction == "input") {
							if ("pin" in pin)
								states[pin.pin] = 4;
						}
						else if (direction == "output") {
							if ("pin" in pin)
								states[pin.pin] = 5;
						}
					}
					break;
				case "Ground":
					if ("pin" in pin)
						states[pin.pin] = 2;
					break;
				case "I2C":
					if ("clock" in pin)
						states[pin.clock] = 6;
					if ("sda" in pin)
						states[pin.sda] = 7;
					break;
				case "Power":
					if ("pin" in pin)
						states[pin.pin] = 1;
					break;
				case "PWM":
					if ("pin" in pin)
						states[pin.pin] = 10;
					break;
				}
			}
		}
		shell.last.distribute("onConfigure", states);
	}
	onPinsError(shell, path, object, error) {
		this.LOG.behavior.onPinsError(this.LOG, path, object, error);
	}
	onPinsInvoke(shell, path, object, result) {
		this.LOG.behavior.onPinsInvoke(this.LOG, path, object, result);
	}
	onPinsRepeat(shell, path, object, result) {
		this.LOG.behavior.onPinsRepeat(this.LOG, path, object, result);
	}
	onQuit() {
		this.host.quit();
		this.writePreferences();
	}
	onTimeChanged(target) {
		this.host.wake();
	}
	onTouchBegan(target, id, x, y) {
	}
	onTouchEnded(target, id, x, y) {
	}
	onTouchMoved(target, id, x, y) {
		this.touchX = x;
		this.touchY = y;
	}
	onTouchScrolled(shell, touched, dx, dy, ticks) {
		var content = shell.first.hit(this.touchX, this.touchY);
		while (content) {
			if (content instanceof Scroller)
				content.delegate("onTouchScrolled", touched, dx, dy, ticks);
			content = content.container;
		}
	}
	readPreferences() {
		try {
			var url = mergeURI(Files.preferencesDirectory, this.preferences);
			if (Files.exists(url)) {
				var preferences = JSON.parse(Files.readText(url));
				if ("windowState" in preferences)
					shell.windowState = preferences.windowState;
			}
		}
		catch(e) {
		}
	}
	writePreferences() {
		try {
			var url = mergeURI(Files.preferencesDirectory, this.preferences);
			var preferences = {
				windowState: shell.windowState,
			};
			Files.writeText(url, JSON.stringify(preferences));
		}
		catch(e) {
		}
	}
}

shell.menus = [
	{ 
		title: "Shell", 
		items: [
			{ title: "Break", command: "BreakShell"},
			{ title: "Purge", key: "", command: "PurgeShell"},
			null,
			{ title: "Exit", key: "Q", command: "Quit"},
		]
	},
	{ 
		title: "Application", 
		items: [
			{ title: "Break", key: "B", command: "BreakApplication"},
			{ title: "Purge", key: "", command: "PurgeApplication"},
		]
	},
	{
		title: "Help", 
		items: [
			{ title: "Kinoma Developer", command: "Support" },
			null,
			{ title: "About Kinoma Simulator", command: "About"},
		]
	}
];
shell.behavior = new ShellBehavior(shell, {});
