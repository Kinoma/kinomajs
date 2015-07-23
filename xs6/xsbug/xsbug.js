/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
import * as CONTROL from "control";
import * as SCROLLER from "scroller";

shell.menus = [
	{ 
		title: "Debug",
		items: [
			{ title: "Kill", key: "K", command: "Kill" },
			null,
			{ title: "Run", key: "R", command: "Run" },
			{ title: "Step", key: "S", command: "Step" },
			{ title: "Step In", key: "I", command: "StepIn" },
			{ title: "Step Out", key: "O", command: "StepOut" },
			null,
			{ title: "Set Breakpoint", key: "B", command: "SetBreakpoint" },
			{ title: "Clear Breakpoint", key: "Shift+B", command: "ClearBreakpoint" },
			{ title: "Clear All Breakpoints", key: "Alt+Shift+B", command: "ClearAllBreakpoints" },
		],
	},
	{ 
		title: "Edit",
		items: [
			{ title: "Undo", key: "Z", command: "Undo" },
			null,
			{ title: "Cut", key: "X", command: "Cut" },
			{ title: "Copy", key: "C", command: "Copy" },
			{ title: "Paste", key: "V", command: "Paste" },
			null,
			{ title: "Find", key: "F", command: "Find" },
			{ title: "Find Next", key: "G", command: "FindNext" },
			{ title: "Find Previous", key: "Shift+G", command: "FindPrevious" },
		],
	},
	{ 
		title: "Machine",
		items: [
		],
	},
	{
		title: "View",
		items: [
			{ title: "Calls", key: "Alt+C", command: "Calls" },
			{ title: "Globals", key: "Alt+G", command: "Globals" },
			{ title: "Modules", key: "Alt+M", command: "Modules" },
			{ title: "Files", key: "Alt+F", command: "Files" },
			{ title: "Breakpoints", key: "Alt+B", command: "Breakpoints" },
		],
	}
];

var lineHeight = 16;
var dividerSize = 22;
var dividerMin = 200;
var halfWidth;
var halfHeight;

var screenSkin = new Skin({ fill: "white" });
var screenStyle = new Style({ font: "14px Helvetica", horizontal:"left", vertical:"middle" })

var di = "com.kinoma.xsbug";

var toolbarTexture = new Texture("./assets/toolbar.png", 2);
var toolbarSkin = new Skin({ texture: toolbarTexture, x:0, y:0, width:20, height:20, variants:20, states:20 });

var cancellerSkin = new Skin({ fill:"#20000000" });
var dialogBoxTexture = new Texture("./assets/dialogBox.png", 1);
var dialogBoxSkin = new Skin({ texture: dialogBoxTexture, x:0, y:0, width:70, height:70, 
	tiles: { left:30, right: 30, top:30, bottom: 30 },
	margins: { left:20, right: 20, top:20, bottom: 20 }
});
var menuButtonStyle = new Style({ font: "bold", color:[ "gray", "black", "#76b321", "#76b321" ] })
var menuButtonAddressStyle = new Style({ color:[ "gray", "gray", "#76b321" ] })
var menuTexture = new Texture("./assets/menu.png", 2);
var menuCheckSkin = new Skin({ texture: menuTexture, x:0, y:0, width:20, height:20, states:20 });
var menuArrowSkin = new Skin({ texture: menuTexture, x:0, y:60, width:20, height:10, });
var menuLineSkin = new Skin({ fill:[ "transparent", "transparent", "#76b321"], stroke: "white", borders: { bottom:1 } });
var menuLineStyle = new Style({ font: "bold", color: ["gray", "black", "white"] })
var menuLineAddressStyle = new Style({ color: ["gray", "gray", "white"] })

var tabTexture = new Texture("./assets/tab.png", 1);
var tabButtonSkin = new Skin({ texture:tabTexture, x:0, y:0, width:48, height:32, tiles: { left:16, right: 16, }, states:32 });
var tabButtonStyle = new Style({ font: "bold 15px", color:["white", "black", "#76b321"], left:16, right:16 })

var headerSkin = new Skin({ fill:"#e8e8e8", stroke: "gray", borders: { bottom:1 } });
var headerStyle = new Style({ font: "bold 14px", color:"black", left:5, vertical:"middle" });
var borderSkin = new Skin({ fill:"#f8f8f8" });
var bodyStyle = new Style({ font: "16px Menlo", horizontal:"left" });
var lineSkin = new Skin({ fill: [ "transparent", "#4076b321", "#40b37621" ] });
var lineStyle = new Style({ color: ["black", "black", "black"] });
var nameStyle = new Style({ font: "bold 16px Menlo", color: "black" });
var valueStyle = new Style({ font: "16px Menlo", color: "black" });
var flagsStyle = new Style({ font: "15px Menlo", color: "white", horizontal:"center", vertical:"middle", right:2, top: 3 });

var flagsSkin = new Skin({ fill: "#76b321" });
var lineNumberTexture = new Texture("./assets/flags.png", 2);
var lineNumberSkin = new Skin({ texture:lineNumberTexture, x:0, y:0, width:24, height:16, tiles: { left:8, right: 8, }, states:16 });
var lineNumberStyle = new Style({ font: "11px Helvetica", color: ["gray", "white"], horizontal:"right", vertical:"bottom", right:10 })

var lineFlagsTexture = new Texture("./assets/line.png", 2);
var lineFlagsSkin = new Skin({ texture:lineFlagsTexture, x:0, y:0, width:16, height:16, states:16 });

var fileSkin = new Skin({ fill:["transparent", "red", "green", "#76b321"] })
var fileStyle = new Style({ font: "15px Menlo", color: [ "black", "green", "blue", "red" ], leading: lineHeight })

var findSkin = new Skin({ fill: "white" });
var findStyle = new Style({ font: "16px Menlo", color:"black", horizontal:"left", left:5, right:5});
var findHintStyle = new Style({ font: "16px Menlo", color:"silver", horizontal:"left", left:5, right:5});

var horizontalDividerSkin = new Skin({ stroke: "gray", borders: { bottom:1 }});

var verticalScrollbarSkin = new Skin({ texture:new Texture("./assets/verticalScrollbar.png", 1), x:0, y:0, width:10, height:40, tiles: { top:10, bottom: 10, } });
var fieldLabelSkin = new Skin({ fill: [ "transparent","transparent","#C0C0C0","#acd473" ] });
var localhostStyle = new Style({ font: "bold 14px", color:"black", right:5, horizontal:"right" });

var pad = function(value, size) {
  var s = String(value);
  while (s.length < (size || 2)) {s = "0" + s;}
  return s;
}

var ToolButton = Container.template(function($) { return {
	width:32, height:32, active:false,
	behavior: CONTROL.ButtonBehavior({
		doActivate: function(content, machine) {
			switch (content.index) {
				case 0: content.active = machine != null; break;
				default: content.active = machine && machine.broken; break;
			}
		},
		onTap: function(content) {
			content.bubble("onMachineCommand", this.data.command);
		}
	}),
	contents: [
		Content($, {skin:toolbarSkin, variant:$.variant}),
	]
}});

var MachineMenuButton = Line.template(function($) { return {
	anchor: "MACHINES", left:0, right:0, height:32, active:false,
	skin: new Skin({ stroke: "gray", borders: { left:1 }}),
	behavior: CONTROL.ButtonBehavior({
//		changeState: function(container, state) {
//			container.state = state;
//			var content = container.first;
//			while (content) {
//				if (state == 2)
//					content.state |= state;
//				else
//					content.state &= state;
//				content = content.next;
//			}
//		},
		onMachinesChanged: function(container, machines) {
			container.active = machines.length > 0;
			container.first.delegate("onMachinesChanged", machines);
		},
		onTap: function(container) {
			this.data.button = container;
			shell.add(new MachineMenuDialog(this.data));
		}
	}),
	contents: [
		Content($, { left:0, width:32, top:0, height:32, skin:toolbarSkin, variant:8,
			behavior: Behavior({
				onMachinesChanged: function(content, machines) {
					if (machines.length > 0)
						content.state |= 1;
					else
						content.state &= ~1;
				},
			}),
		}),
		Label($, {
			left:0, height:32, style:menuButtonStyle, state:1,
			behavior: Behavior({
				onMachineSelected: function(label, machine) {
					label.string = machine && machine.title ? machine.title : "";
//					label.state = machine && machine.broken ? 1 : 0;
				},
			}),
		}),
		Label($, {
			left:0, right:0, height:32, style:menuButtonAddressStyle,
			behavior: Behavior({
				onMachineSelected: function(label, machine) {
					var string = (machine && machine.title) ? " @ " : "";
					if (machine) string += machine.address;
					label.string = string;
				},
			}),
		}),
	]
}});
var MachineMenuDialog = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, active:true,
	behavior:Behavior({
		onTouchEnded: function(container, id, x, y, ticks) {
			var layout = container.first;
			if (!layout.hit(x, y))
				layout.behavior.onCancel(layout);
		}
	}),
	contents: [
		Layout($, {
			left:162, top:41, skin: dialogBoxSkin, style: screenStyle,
			behavior: Behavior({
				onCancel: function(layout) {
					shell.remove(shell.last);
					model.dialog = null;
				},
				onCreate: function(layout, data) {
					this.data = data;
				},
				onMeasureHorizontally: function(layout) {
					//var size = layout.first.first.measure();
					return shell.width - 172;
				},
				onMeasureVertically: function(layout) {
					var size = layout.first.first.measure();
					return Math.min(size.height, shell.height - 46);
				},
			}),
			contents: [
				MachineMenuScroller($),
				Content($, { left:6, top:-12, skin:menuArrowSkin }),
			],
		}),
	],

}});
var MachineMenuScroller = SCROLLER.VerticalScroller.template(function($) { return {
	left:0, right:0, top:0, bottom:0, clip: true,
	contents: [
		Column($, {
			left:0, right:0, top:0,
			contents: $.machines.map(function(machine) {
				return new MachineMenuLine(machine);
			}),
		})
	]
}});
var MachineMenuLine = Line.template(function($) { return {
	left:0, right:0, height:32, skin: menuLineSkin, active:true,
	behavior: Behavior({
		changeState: function(line, state) {
			line.state = state;
			line.first.state = state;
			line.first.next.state = state;
			line.last.state = state;
		},
		onCreate: function(line, data) {
			this.data = data;
			this.onStateChanged(line);
		},
		onStateChanged: function(line) {
			this.changeState(line, this.data.broken ? 1 : 0);
		},
		onTap: function(line) {
			model.selectMachine(this.data);
			line.bubble("onCancel");
		},
		onTouchBegan: function(container, id, x, y, ticks) {
			this.changeState(container, 2);
		},
		onTouchCancelled: function(container, id, x, y, ticks) {
			this.changeState(container, this.data.broken ? 1 : 0);
		},
		onTouchEnded: function(container, id, x, y, ticks) {
			this.changeState(container, this.data.broken ? 1 : 0);
			this.onTap(container);
		}
	}),
	contents: [
		Content($, { width:32, height:32, skin: menuCheckSkin, visible: model.data.currentMachine == $, }),
		Label($, {left:0, height:32, style:menuLineStyle, string:$.title}),
		Label($, {left:0, right:0, height:32, style:menuLineAddressStyle, string:($.title ? " @ " : "") + $.address}),
	]
}});

var DialogButton = Container.template(function($) { return {
	width:80, height:32, active:true, skin:tabButtonSkin,
	behavior: CONTROL.ButtonBehavior({
		onTouchEnded(container) {
			container.bubble(this.data.event);
		}
	}),
	contents: [
		Label($, { style:tabButtonStyle, string:$.title }),
	]
}});

var SettingsButton = Container.template(function($) { return {
	width:32, height:32, active:true,
	behavior: CONTROL.ButtonBehavior({
		onTap: function(container) {
			this.data.button = container;
			this.data.port = model.preferences.port;
			shell.add(new SettingsDialog(this.data));
		}
	}),
	contents: [
		Content($, {skin:toolbarSkin, variant:14}),
	]
}});
var SettingsDialog = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, active:true, skin:cancellerSkin,
	behavior:Behavior({
		onTouchEnded: function(container, id, x, y, ticks) {
			var layout = container.first;
			layout.behavior.onCancel(layout);
		}
	}),
	contents: [
		Layout($, {
			right:5, top:41, skin: dialogBoxSkin, style: screenStyle, active:true,
			behavior: Behavior({
				onCancel: function(layout) {
					shell.remove(shell.last);
					model.dialog = null;
				},
				onCreate: function(layout, data) {
					this.data = data;
				},
				onKeyDown(layout, key, repeat, ticks) {
					var code = key.charCodeAt(0);
           			if ((Event.FunctionKeyPower == code) || (0xF000C == code))
                		return false;
					return true;
				},
				onKeyUp(layout, key, repeat, ticks) {
					var code = key.charCodeAt(0);
					if ((code == 27) || (code == 0xF0001))
						this.onCancel(layout);
					else if ((code == 3) || (code == 13)) {
						if (this.data.OK.active)
							this.onOK(layout);
					}
           			if ((Event.FunctionKeyPower == code) || (0xF000C == code))
                		return false;
					return true;
				},
				onMeasureHorizontally: function(layout) {
					return 128;
				},
				onMeasureVertically: function(layout) {
					var size = layout.first.measure();
					return size.height;
				},
				onOK: function(layout) {
					shell.remove(shell.last);
					model.dialog = null;
					model.doChangePort(shell, parseInt(this.data.PORT.string));
				},
			}),
			contents: [
				Column($, {
					left:0, right:0, top:0,
					contents: [
						//DIALOG.Field({ label:"Port", labelWidth:80, value:"5002" }),
						//DIALOG.Field({ label:"Home", labelWidth:80, value:"/Users/ps/dev/fsk/" }),
						//DIALOG.Field({ label:"Remote", labelWidth:80, value:"Y:\\fsk\\" }),
						
						Line($, {
							left:0, right:0, height:36,
							contents: [
								Label($, { width:48, style:localhostStyle, string:"Port:" }),
								Scroller($, {
									left:0, right:0, top:5, bottom:5,
									skin: findSkin,
									clip:true,
									behavior: CONTROL.FieldScrollerBehavior,
									contents: [
										Label($, {
											anchor: "PORT", left: 0, top:0, bottom:0,
											skin:fieldLabelSkin,
											style:findStyle,
											string:$.port,
											active:true,
											editable: true,
											selectable: true,
											behavior: CONTROL.FieldLabelBehavior({
												onDisplayed: function(label, view) {
													label.focus();
												},
												onEdited: function(label) {
													var string = label.string;
													var port = parseInt(string);
													this.data.OK.active = (string == port.toString());
													//label.bubble("doSearch", string);
												},
												onFocused: function(label) {
													CONTROL.FieldLabelBehavior.prototype.onFocused.call(this, label);
												},
												onUnfocused: function(label) {
													CONTROL.FieldLabelBehavior.prototype.onUnfocused.call(this, label);
												},
											}),
										}),
									],
								}),
								Container($, {
									anchor: "OK", width:32, height:32, active:true,
									behavior: CONTROL.ButtonBehavior({
										onTap: function(content) {
											content.bubble("onOK");
										}
									}),
									contents: [
										Content($, {skin:toolbarSkin, variant:15}),
									]
								})
							],
						}),
					],
				}),
				Content($, { right:1, top:-12, skin:menuArrowSkin }),
			],
		}),
	],
}});


var tabStrings = [
// 	"Calls", "Globals", "Modules", "Files", "Breakpoints", "Kinoma"
	"Calls", "Globals", "Modules", "Files", "Breakpoints"
];

var TabButton = Container.template(function($) { return {
	height:32, active:true, skin:tabButtonSkin,
	behavior: CONTROL.ButtonBehavior({
		onCreate: function(container, data) {
			this.data = data;
		},
		onDisplaying: function(container) {
			container.first.string = tabStrings[container.index];
			this.onTabSelected(container);
		},
		onTabSelected: function(container) {
			if (this.data.currentTab == container.index) {
				this.changeState(container, 0);
				container.active = false;
			}
			else {
				this.changeState(container, 1);
				container.active = true;
			}
			model.updateViewMenu();
		},
		onTap: function(container) {
			this.data.currentTab = container.index;
			shell.first.distribute("onTabSelected", this.data.currentTab);
		},
	}),
	contents: [
		Label($, { style:tabButtonStyle }),
	]
}});

var dividerLayoutBehavior = Behavior({
	onAdapt: function(layout) {
		var divider = layout.last;
		divider.behavior.divide(divider);
	}
});

var VerticalDivider = Content.template(function($) { return {
	left:halfWidth - (dividerSize / 2), width:dividerSize, top:0, bottom:0, active:true,
	behavior: Behavior({
		divide: function(divider) {
			var layout = divider.container;
			var left = divider.x - layout.x + (dividerSize / 2);
			var right = layout.width - left;
			if (left < dividerMin) {
				left = dividerMin;
				right = layout.width - left;
			}
			else if (right < dividerMin) {
				right = dividerMin;
				left = layout.width - right;
			}
			divider.x = layout.x + left - (dividerSize / 2);
			divider.previous.previous.width = left;
			divider.previous.width = right;
		},
	
		onTouchBegan: function(divider, id, x, y, ticks) {
			this.anchor = x - divider.x;
		},
		onTouchMoved: function(divider, id, x, y, ticks) {
			divider.x = x - this.anchor;
			this.divide(divider);
		},
	}),
	
}});

var HorizontalDivider = Content.template(function($) { return {
	left:0, right:dividerSize, top:halfHeight, height:dividerSize, active:true,
	behavior: Behavior({
		divide: function(divider) {
			var layout = divider.container;
			var top = divider.y - layout.y;
			var bottom = layout.height - top;
			if (top < dividerMin) {
				top = dividerMin;
				bottom = layout.height - top;
			}
			else if (bottom < dividerMin) {
				bottom = dividerMin;
				top = layout.height - bottom;
			}
			divider.y = layout.y + top;
			divider.previous.previous.height = top;
			divider.previous.height = bottom;
		},
	
		onTouchBegan: function(divider, id, x, y, ticks) {
			this.anchor = y - divider.y;
		},
		onTouchMoved: function(divider, id, x, y, ticks) {
			divider.y = y - this.anchor;
			this.divide(divider);
		},
	}),
}});

var Screen = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0,
	skin: screenSkin, style: screenStyle,
	contents: [
		Line($, {
			anchor:"BUTTONS",
			left:0, right:0, top:0, height:40, skin: headerSkin,
			contents: [
				ToolButton({ command:"abort", variant:0 }),
				ToolButton({ command:"go", variant:1 }),
				ToolButton({ command:"step", variant:2 }),
				ToolButton({ command:"stepIn", variant:3 }),
				ToolButton({ command:"stepOut", variant:4 }),
				MachineMenuButton($),
				SettingsButton($),
			],
		}),
		Container($, {
			left:0, right:0, top:40, height:40, skin: headerSkin,
			contents: [
				Line($, {
					height:40,
					contents: [
						TabButton($),
						TabButton($),
						TabButton($),
						TabButton($),
						TabButton($),
						TabButton($),
					],
				}),
			]
		}),
		Layout($, {
			left:0, right:0, top:80, bottom:0,
			behavior: dividerLayoutBehavior,
			contents: [
				Container($, {
					left:0, width:$.verticalDivider, top:0, bottom:0,
					behavior: Behavior({
						onTabSelected: function(container, tabIndex) {
							var content = container.first;
							while (content) {
								content.visible = content.index == tabIndex;
								content = content.next;
							}
						}
					}),
					contents: [
						Layout($, {
							left:0, right:0, top:0, bottom:0,
							behavior: dividerLayoutBehavior,
							contents: [
								Container($, {
									anchor: "CALLS",
									left:1, right:0, top:0, height:$.horizontalDividerLeft, skin: horizontalDividerSkin,
									contents: [
										ViewHeader("Calls"),
										ViewBody({ command: "doFile", viewIndex: 0, keepLine: true, keepState: 2 }, {
											left: 0,
											behavior: Behavior({
												createLine: function(item, index) {
													item.index = index + 1;
													if (item.name == "?")
														item.map = model.mapPath(item.path);
													return new ViewFrameLine(item);
												},
											}),
										})
									]
								}),
								Container($, {
									anchor: "LOCALS",
									left:1, right:0, height:$.height - $.horizontalDividerLeft, bottom:0,
									contents: [
										ViewHeader("Locals"),
										ViewBody({ command: "doToggle", viewIndex: 1, keepLine: false, keepState: 1 }, {
											left: 0,
											behavior: Behavior({
												createLine: function(item) {
													return new ViewPropertyLine(item);
												},
											}),
										})
									]
								}),
								HorizontalDivider($, {
									top:$.horizontalDividerLeft,
								}),
							]
						}),
						Container($, {
							anchor: "GLOBALS",
							left:0, right:0, top:0, bottom:0, visible:false,
							contents: [
								ViewHeader("Globals"),
								ViewBody({ command: "doToggle", viewIndex: 2, keepLine: false, keepState: 1 }, {
									left: 0,
									behavior: Behavior({
										createLine: function(item) {
											return new ViewPropertyLine(item);
										},
									}),
								})
							]
						}),
						Container($, {
							anchor: "MODULES",
							left:0, right:0, top:0, bottom:0, visible:false,
							contents: [
								ViewHeader("Modules"),
								ViewBody({ command: "doToggle", viewIndex: 5, keepLine: false, keepState: 1 }, {
									left: 0,
									behavior: Behavior({
										createLine: function(item) {
											return new ViewPropertyLine(item);
										},
									}),
								})
							]
						}),
						Container($, {
							anchor: "FILES",
							left:0, right:0, top:0, bottom:0, visible:false,
							contents: [
								ViewHeader("Files"),
								ViewBody({ command: "doFile", viewIndex: 3, keepLine: true, keepState: 1 }, {
									behavior: Behavior({
										createLine: function(item) {
											item.map = model.mapPath(item.path);
											return new ViewFileLine(item);
										},
									}),
								})
							]
						}),
						Container($, {
							anchor: "BREAKPOINTS",
							left:0, right:0, top:0, bottom:0, visible:false,
							contents: [
								BreakpointsHeader("Breakpoints"),
								ViewBody({ command: "doFile", viewIndex: 4, keepLine: false, keepState: 1 }, {
									behavior: Behavior({
										createLine: function(item) {
											item.map = model.mapPath(item.path);
											return new ViewBreakpointLine(item);
										},
									}),
								})
							]
						}),
// 						Container($, {
// 							anchor: "KINOMA",
// 							left:0, right:0, top:0, bottom:0, visible:false,
// 							contents: [
// 								BrowserHeader("Kinoma"),
// 								BrowserBody({ url: "http://www.kinoma.com/search/" })
// 							]
// 						}),
					]
				}),
				Layout($, {
					width:$.width - $.verticalDivider, right:0, top:0, bottom:0,
					skin: new Skin({ stroke: "gray", borders: { left:1 }}),
					behavior: dividerLayoutBehavior,
					contents: [
						Column($, {
							anchor: "FILE",
							left:1, right:0, top:0, height:$.horizontalDividerRight, skin: horizontalDividerSkin,
							contents: [
								FileHeader("File"),
								FileBody({ command: "doSelection", viewIndex: 6 }, { left: 0 })
							]
						}),
						Container($, {
							anchor: "LOG",
							left:1, right:0, height:$.height - $.horizontalDividerRight, bottom:0,
							contents: [
								LogHeader("Log"),
								ViewBody({ command: "doLog", viewIndex: 7 })
							]
						}),
						HorizontalDivider($, {
							top:$.horizontalDividerRight,
						}),
					]
				}),
				VerticalDivider($, {
					left:$.verticalDivider - (dividerSize / 2),
				}),
			]
		}),
	]
}});

var ViewHeader = Line.template(function($) { return {
	left:0, right:0, top:0, height:dividerSize, skin: headerSkin,
	contents: [
		Label($, {left:0, right:0, top:0, bottom:1, style:headerStyle, string:$ }),
	]
}});

var BreakpointsHeader = Line.template(function($) { return {
	left:0, right:0, top:0, height:dividerSize, skin: headerSkin,
	contents: [
		Label($, {left:0, right:0, top:0, bottom:1, style:headerStyle, string:$ }),
		ToolButton({ variant:7 }, {
			width:32, right:0,
			behavior: CONTROL.ButtonBehavior({
				onTap: function(button) {
					model.onMachineCommand(shell, "resetBreakpoints");
				},
				onViewChanged: function(button, view) {
					button.active = model.data.currentMachine && view && (view.lines.length > 0);
				},
			}),
		}),
	]
}});

var LogHeader = Line.template(function($) { return {
	left:0, right:0, top:0, height:dividerSize, skin: headerSkin,
	contents: [
		Label($, {left:0, right:0, top:0, bottom:1, style:headerStyle, string:$ }),
		ToolButton({ variant:7 }, {
			width:32, right:0,
			behavior: CONTROL.ButtonBehavior({
				onTap: function(button) {
					button.container.next.first.first.empty();
					button.active = false;
				},
				onPrint: function(button, string, newLine) {
					button.active = true;
				},
			}),
		}),
	]
}});

var ViewBody = Container.template(function($) { return {
	anchor: "BODY",
	left:5, right:5, top:dividerSize, bottom:1, style: bodyStyle,
	behavior: Behavior({
		createLine: function(item) {
			return new ViewLine(item.name);
		},
	}),
	contents: [
		SCROLLER.VerticalScroller($, {
			clip: true,
			contents: [
				Container($, {
					left:0, right:0, top:0, active:true, backgroundTouch:true,
					behavior: Behavior({
						moveSelection(container, index) {
							var selection = this.data.SELECTION;
							var tag = this.data.TAG;
							if (index < 0) {
								selection.visible = false;
								selection.state = 0;
								tag.visible = false;
							}
							else {
								var line = this.view.lines[index];
								if ("flags" in line) {
									tag.string = line.flags;
									tag.visible = true;
								}
								else {
									tag.string = "";
									tag.visible = false;
								}
								selection.visible = true;
								selection.state = this.data.keepState;
								selection.offset = { x:0, y: index * lineHeight };
								tag.y = container.y + index * lineHeight;
							}
							this.touchIndex = index;
						},
						onCreate: function(container, data) {
							this.data = data;
							this.touchIndex = -1;
						},
						onTouchBegan: function(container, id, x, y, ticks) {
							this.moveSelection(container, Math.floor((y - container.y) / lineHeight));
						},
						onTouchCancelled: function(container, id, x, y, ticks) {
							this.moveSelection(container, -1);
						},
						onTouchEnded: function(container, id, x, y, ticks) {
							container.bubble(this.data.command, this.data.viewIndex, this.touchIndex);
							if (!this.data.keepLine)
								this.moveSelection(container, -1);
						},
						onSelectionChanged: function(container, view) {
							var touchIndex = -1;
							if (view) {
								var lines = view.lines;
								if (lines) {
									var c = lines.length
									if ((c > 0) && this.data.keepLine) {
										if (view.lineIndex >= c)
											touchIndex = c - 1;
										else
											touchIndex = view.lineIndex;
									}
								}
							}
							this.view = view;
							this.moveSelection(container, touchIndex);
						},
						onViewChanged: function(container, view, title) {
							this.onSelectionChanged(container, view);
						},
					}),
					contents: [
						Content($, { anchor: "SELECTION", left:0, right:0, top:0, height:lineHeight, skin:lineSkin, state:0, visible:false }),
						SCROLLER.HorizontalScroller($, {
							bottom:undefined, clip: true,
							contents: [
								Column($, {
									anchor: "LINES",
									left:0, top:0,
									behavior: Behavior({
										onCreate: function(column, data) {
											this.data = data;
										},
										onPrint: function(column, string, newLine) {
											// only for LOG
											if (newLine || !column.last) {
												column.add(new ViewLine(string));
											}
											else {
												column.replace(column.last, new ViewLine(string))
											}
											column.container.container.container.scrollTo(0, 0x7FFFFFFF);
										},
										onViewChanged: function(column, view, title) {
											column.empty();
											if (!view) return;
											var items = view.lines;
											if (!items) return;
											var body = this.data.BODY; //column.container.container.container;
											items.forEach(function(item, index) {
												column.add(body.behavior.createLine(item, index));
											});
										},
									}),
								}),
							]
						}),
						Label($, { anchor: "TAG", width:32, right:0, top:0, height:lineHeight, skin:flagsSkin, style:flagsStyle, visible:false }),
						
					]
				}),
				SCROLLER.VerticalScrollbar($, { skin:verticalScrollbarSkin })
			]
		}),
	]
}});

var ViewLine = Line.template(function($) { return {
	left:0, height:lineHeight, skin:lineSkin,
	contents: [
		Label($, { style:lineStyle, string:$, skin:lineSkin}),
	]
}});

var ViewBreakpointLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:lineSkin,
	contents: [
		Label($, {width:40, height:lineHeight, style:lineNumberStyle, string:$.line, skin:lineNumberSkin, variant:0, state:1 }),
		Label($, {left:0, style:lineStyle, string:"key" in $.map ? "$" + $.map.key : "", skin:lineSkin}),
		Label($, {left:0, style:lineStyle, string:$.map.path, skin:lineSkin}),
	]
}});

var ViewFileLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:lineSkin,
	contents: [
		Label($, {left:0, style:lineStyle, string:"key" in $.map ? "$" + $.map.key : "", skin:lineSkin}),
		Label($, {left:0, style:lineStyle, string:$.map.path, skin:lineSkin}),
	]
}});

var ViewFrameLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:lineSkin,
	contents: [
		Content($, { width:lineHeight, }),
		Label($, { style:nameStyle, string:$.name, skin:lineSkin }),
		/*
		Label($, { left:5, style:lineStyle, string:$.map ? ("key" in $.map ? "$" + $.map.key : "" ) : $.name, skin:lineSkin }),
		$.map ? Label($, { left:0, style:valueStyle, string:$.map.path, skin:lineSkin }) : undefined,
		*/
	]
}});

var ViewPropertyLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:lineSkin,
	contents: [
		Content($, { left:$.column*lineHeight, width:lineHeight, top:0, height:lineHeight, skin:lineFlagsSkin, state:$.state }),
		Label($, { style:nameStyle, string:$.name, skin:lineSkin }),
		Label($, { style:valueStyle, string:$.state == 0 ? (" = " + $.value) : "", skin:lineSkin }),
	]
}});

var ViewNodeLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:lineSkin,
	contents: [
		Content($, { left:$.column*lineHeight, width:lineHeight, top:0, height:lineHeight, skin:lineFlagsSkin, state:$.state }),
		Label($, { left:0, style:nameStyle, string:$.name, skin:lineSkin }),
		Label($, { left:0, style:valueStyle, string:$.data ? (" = " + $.data) : "", skin:lineSkin })
	]
}});

var FileHeader = Line.template(function($) { return {
	left:0, right:0, top:0, height:dividerSize, skin: headerSkin,
	contents: [
		Label($, {left:0, width: 40, top:0, bottom:1, style:headerStyle, string:$ }),
		Label($, {left:0, top:0, bottom:1, style:valueStyle, string:"",
			behavior: Behavior({
				onViewChanged: function(label, view) {
					label.string = view && view.title && view.title.key ? "$" + view.title.key : "";
				},
			}),
		}),
		Label($, {left:0, right:0, top:0, bottom:1, style:valueStyle, string:"",
			behavior: Behavior({
				onViewChanged: function(label, view) {
					label.string = view && view.title && view.title.path ? view.title.path : "";
				},
			}),
		}),
		ToolButton({ variant:9 }, {
			width:32, right:0,
			behavior: CONTROL.ButtonBehavior({
				doFind: function(button) {
					var container = button.container.container;
					if (container.length == 2) {
						container.insert(new FileFind({ string: "", indices: [], index: -1 }), container.last);
					}
				},
				hideFind: function(button) {
					var container = button.container.container;
					if (container.length == 3) {
						container.remove(container.first.next);
						button.bubble("doSearch", null);
					}
				},
				onTap: function(button) {
					var container = button.container.container;
					if (container.length == 2) {
						container.insert(new FileFind({ string: "", indices: [], index: -1 }), container.last);
					}
					else {
						container.remove(container.first.next);
						button.bubble("doSearch", null);
					}
				},
				onViewChanged: function(button, view) {
					var active = view && view.text;
					var container = button.container.container;
					button.active = active;
					if (!active && (container.length == 3)) {
						container.remove(container.first.next);
					}
				},
			}),
		}),
	]
}});

var FileFind = Line.template(function($) { return {
	left:0, right:0, top:0, height:dividerSize, skin: headerSkin,
	contents: [
		Label($, {
			left:0, width:40, style:headerStyle,
			string:"Find",
		}),
		Scroller($, {
			left:0, right:0, top:0, bottom:1,
			skin: findSkin,
			clip:true,
			behavior: CONTROL.FieldScrollerBehavior,
			contents: [
				Label($, {
					left: 0, top:0, bottom:0,
					skin:fieldLabelSkin,
					style:findStyle,
					string:$.string,
					active:true,
					editable: true,
					selectable: true,
					behavior: CONTROL.FieldLabelBehavior({
						onDisplayed: function(label, view) {
							label.focus();
						},
						onEdited: function(label) {
							var string = label.string;
							var visible = string.length > 0;
							label.bubble("doSearch", string);
						},
						onFocused: function(label) {
							CONTROL.FieldLabelBehavior.prototype.onFocused.call(this, label);
						},
						onUnfocused: function(label) {
							CONTROL.FieldLabelBehavior.prototype.onUnfocused.call(this, label);
						},
					}),
				}),
				Label($, {
					left: 0, top:0, bottom:0,
					style:findHintStyle,
					string:"String",
					behavior: Behavior({
						onSelect: function(label, view) {
							label.visible = !(view && view.string && (view.string.length > 0));
						}
					}),
				}),
			],
		}),
		Container($, {
			left:0, width:24, top:0, bottom:1,
			skin: findSkin,
			contents: [
				ToolButton({ variant:12 }, {
					left:0, width:24, visible:false,
					behavior: CONTROL.ButtonBehavior({
						onTap: function(button) {
							var label = button.container.previous.first;
							label.string = "";
							label.focus();
							label.behavior.onEdited(label);
						},
						onSelect: function(button, view) {
							button.active = button.visible = view && view.string && (view.string.length > 0);
						}
					}),
				}),
			],
		}),
		ToolButton({ variant:10 }, {
			width:24, left:0,
			behavior: CONTROL.ButtonBehavior({
				onFound: function(button, view) {
					this.data.view = view;
					this.updateState(button, view);
				},
				onSelect: function(button, view) {
					this.updateState(button, view);
				},
				onTap: function(button) {
					button.bubble("doSelectPrevious");
				},
				updateState: function(button, view) {
					button.active = (view.index > 0);
				},
			}),
		}),
		Label($, {
			left:0, width:50, style:headerStyle,
			string:"",
			behavior: CONTROL.ButtonBehavior({
				onFound: function(label, view) {
					this.updateState(label, view);
				},
				onSelect: function(label, view) {
					this.updateState(label, view);
				},
				updateState: function(label, view) {
					if (view.index + 1 > view.indices.length)
						label.string = "";
					else
						label.string = (view.index + 1) + "/" + view.indices.length;
				},
			}),
		}),
		ToolButton({ variant:11 }, {
			width:24, left:0,
			behavior: CONTROL.ButtonBehavior({
				onFound: function(button, view) {
					this.data.view = view;
					this.updateState(button, view);
				},
				onSelect: function(button, view) {
					this.updateState(button, view);
				},
				onTap: function(button) {
					button.bubble("doSelectNext");
				},
				updateState: function(button, view) {
					button.active = (view.index + 1 < view.indices.length);
				},
			}),
		}),
	]
}});

var FileBody = Container.template(function($) { return {
	left:5, right:5, top:0, bottom:1, style: bodyStyle,
	contents: [
		Content($, { left:0, width:32, top:0, bottom:0, skin: borderSkin }),
		SCROLLER.VerticalScroller($, {
			clip: true,
			contents: [
				Container($, {
					left:0, right:0, top:0, active: false,
					contents: [
						Column($, {
							anchor: "LINES",
							left:0, right:0, top:0, active: true, backgroundTouch: true,
							behavior: Behavior({
								onCreate: function(column, data) {
									this.data = data;
									this.touchIndex = -1;
									this.touchLine = null;
								},
								onBreakpointsChanged: function(column, breakpoints) {
									var line = column.first;
									if (!line) return;
									while (line) {
										line.first.state = 0;
										line = line.next;
									}
									if (breakpoints.length > 0) {
										breakpoints.map(function(item) {
											return item.line;
										}).forEach(function(index) {
											var line = column.content(index - 1);
											line.first.state = 1;
										});
									}
								},
								onCallsChanged: function(column, calls) {
									var line = column.first;
									if (!line) return;
									while (line) {
										line.first.variant = 0;
										line.state &= 1;
										line = line.next;
									}
									if (calls.length > 0) {
										calls.map(function(item) {
											return item.line;
										}).forEach(function(index) {
											var line = column.content(index - 1);
											line.first.variant = 1;
											line.state = 2;
										});
									}
								},
								onTouchBegan: function(column, id, x, y, ticks) {
									this.touchIndex = Math.floor((y - column.y) / lineHeight);
								},
								onTouchCancelled: function(column, id, x, y, ticks) {
									this.touchIndex = -1;
								},
								onTouchEnded: function(column, id, x, y, ticks) {
									column.bubble(this.data.command, this.data.viewIndex, this.touchIndex);
									if (x - column.x < column.first.first.width) {
										var line = column.content(this.touchIndex);
										if (line.first.state == 0)
											column.bubble("onMachineCommand", "addBreakpoint");
										else
											column.bubble("onMachineCommand", "removeBreakpoint");
									}
									var touchLine = (this.touchIndex >= 0) ? column.content(this.touchIndex) : null;
									if (this.touchLine != touchLine) {
										if (this.touchLine && (this.touchLine.state == 1))
											this.touchLine.state = 0;
										if (touchLine && (touchLine.state == 0))
											touchLine.state = 1;
										this.touchLine = touchLine;
									}
								},
								onLineChanged: function(column, view) {
									column.empty();
									if (!view || !view.text) return;
									var height = this.data.TEXT.height / lineHeight;
									if (height <= 0) return;
									for (var i = 1; i <= height; i++)
										column.add(new FlagLine(i));
									if (this.touchLine && (this.touchLine.state == 1))
										this.touchLine.state = 0;
									this.touchIndex = view && view.lineIndex ? view.lineIndex : -1;
									this.touchLine = (this.touchIndex >= 0) ? column.content(this.touchIndex) : null;
									if (this.touchLine && (this.touchLine.state == 0))
										this.touchLine.state = 1;
									// scroll
									var scroller = column.container.container;
									var y = (lineHeight * (this.touchIndex - (scroller.height / 32)));
									if (Math.abs(scroller.scroll.y - y) > scroller.container.height / 3)
										scroller.scrollTo(0, y);
								},
							}),
						}),
						SCROLLER.HorizontalScroller($, {
							clip: true, left:40, 
							contents: [
								Text($, {
									anchor: "TEXT",
									left:0, top:0, style:fileStyle, skin: fileSkin, active:true, selectable: true,
									behavior: Behavior({
										onCreate: function(text, data) {
											this.data = data;
										},
										onSelect: function(text, view) {
											if (!view || !view.string || (view.stringLength == 0) || (view.indices.length == 0)) {
												text.select(0, 0);
											}
											else {
												var lines = this.data.LINES;
												var behavior = lines.behavior;
												text.select(view.indices[view.index], view.stringLength);
												var selectionBounds = text.selectionBounds;
												var y = selectionBounds.y;
												behavior.touchIndex = Math.floor(y / lineHeight);
												if (behavior.touchLine && (behavior.touchLine.state == 1))
													behavior.touchLine.state = 0;
												behavior.touchLine = (behavior.touchIndex >= 0) ? lines.content(behavior.touchIndex) : null;
												if (behavior.touchLine && (behavior.touchLine.state == 0))
													behavior.touchLine.state = 1;
												view.lineIndex = behavior.touchIndex;
										
												// scroll
												var horizontalScroller = text.container;
												horizontalScroller.reveal(selectionBounds);
												
												var verticalScroller = text.container.container.container;
												//verticalScroller.reveal(selectionBounds);
												var y = (lineHeight * (behavior.touchIndex - (verticalScroller.height / 32)));
												if (Math.abs(verticalScroller.scroll.y - y) > verticalScroller.container.height / 3)
													verticalScroller.scrollTo(0, y);
											}
										},
										onTouchBegan: function(text, id, x, y, ticks) {
											this.data.LINES.delegate("onTouchBegan", id, x, y, ticks);
										},
										onTouchCancelled: function(text, id, x, y, ticks) {
											this.data.LINES.delegate("onTouchCancelled", id, x, y, ticks);
										},
										onTouchEnded: function(text, id, x, y, ticks) {
											this.data.LINES.delegate("onTouchEnded", id, x, y, ticks);
										},
										onViewChanged: function(text, view) {
										trace("onViewChanged \n");
											text.string = view && view.text ? view.text : "";
											this.data.LINES.delegate("onLineChanged", view);
										},
									}),
								}),
							],
						}),
					],
				}),
				SCROLLER.VerticalScrollbar($, { skin:verticalScrollbarSkin })
			]
		}),
	]
}});

var FileLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight,
	contents: [
		Label($, { width:40, height:lineHeight, style:lineNumberStyle, string:$.line, skin:lineNumberSkin }),
		Label($, { left:0, right:0, style:lineStyle, string:$.string, skin:lineSkin }),
	]
}});

var FlagLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:lineSkin,
	contents: [
		Label($, { width:40, height:lineHeight, style:lineNumberStyle, string:$, skin:lineNumberSkin }),
		Label($, { left:0, right:0, style:lineStyle, skin:lineSkin }),
	]
}});

// var BrowserHeader = Line.template(function($) { return {
// 	left:0, right:0, top:0, height:dividerSize, skin: headerSkin,
// 	contents: [
// 		Label($, {left:0, right:0, top:0, bottom:1, style:headerStyle, string:$,
// 			behavior: Behavior({
// 				onCreate: function(label, data) {
// 					this.title = data;
// 				},
// 				onBrowserChanged: function(label, browser) {
// 					label.string = browser.evaluate("document.title");
// 				},
// 			}),
// 		}),
// 		ToolButton({ variant:10 }, {
// 			width:32, right:0,
// 			behavior: CONTROL.ButtonBehavior({
// 				onBrowserChanged: function(button, browser) {
// 					button.active = browser.canBack;
// 				},
// 				onTap: function(button) {
// 					var browser = button.container.next.first;
// 					browser.back();
// 				},
// 			}),
// 		}),
// 		ToolButton({ variant:11 }, {
// 			width:32, right:0,
// 			behavior: CONTROL.ButtonBehavior({
// 				onBrowserChanged: function(button, browser) {
// 					button.active = browser.canForward;
// 				},
// 				onTap: function(button) {
// 					var browser = button.container.next.first;
// 					browser.forward();
// 				},
// 			}),
// 		}),
// 		ToolButton({ variant:13 }, {
// 			width:32, right:0, active:true,
// 			behavior: CONTROL.ButtonBehavior({
// 				onBrowserChanged: function(button, browser) {
// 					button.active = browser.url;
// 				},
// 				onTap: function(button) {
// 					var browser = button.container.next.first;
// 					browser.reload();
// 				},
// 			}),
// 		}),
// 	]
// }});
// 
// var BrowserBehavior = Behavior.template({
// 	onCreate: function(browser, data) {
// 		this.url = data.url;
// 		browser.duration = 100;
// 		browser.start();
// 	},
// 	onFinished: function(browser) {
// 		browser.focus();
// 		browser.load(this.url);
// 	},
// 	onLoading: function(browser) {
// 		trace("onLoading: " + browser.url + "\n");
// 	},
// 	onLoaded: function(browser) {
// 		this.url = browser.url;
// 		browser.container.container.distribute("onBrowserChanged", browser);
// 	},
// 	onInvoke: function(browser, message) {
// 		trace("onInvoke: " + message.url + "\n");
// 	},
// });
// 
// var BrowserView = function($) {
// 	var browser = new Browser($.coordinates);
// 	browser.behavior = new BrowserBehavior(browser, $);
// 	return browser;
// }
// 
// var BrowserBody = Container.template(function($) { return {
// 	left:0, right:0, top:dividerSize, bottom:0, duration: 100,
// 	behavior: Behavior({
// 		onCreate: function(body, data) {
// 			this.data = data;
// 		},
// 		onBrowserChanged: function(button, browser) {
// 			this.data.url = browser.url;
// 		},
// 		onTabSelected: function(body, tabIndex) {
// 			var visible = body.container.index == tabIndex;
// 			if (visible) {
// 				if (!body.first) {
// 					var browser = BrowserView({
// 						coordinates: { left:0, right:0, top:0, bottom:0 },
// 						url: $.url,
// 					});
// 					body.add(browser);
// 				}
// 			}
// 		},
// 	}),
// 	contents: [
// //		BrowserView({
// //			coordinates: { left:0, right:0, top:0, bottom:0 },
// //			url: $.url,
// //		})
// 	]
// }});

var View = function(data) {
	this.path = null;
	this.title = null;
	this.lines = [];
	if (data)
		for (var property in data) {
			if (data.hasOwnProperty(property))
				this[property] = data[property];
		}
}
View.prototype = Object.create(Object.prototype, {
	lineIndex: { value: -1, writable: true },
	lines: { value: null, writable: true },
	title: { value: null, writable: true },
});

var mxFramesView = 0;
var mxLocalsView = 1;
var mxGlobalsView = 2;
var mxFilesView = 3;
var mxBreakpointsView = 4;
var mxGrammarsView = 5;
var mxFileView = 6;
var mxLogView = 7;


var Machine = function(address) {
	this.address = address;
	this.broken = false;
	this.check = false;
	this.command = "Machine";
	this.title = "";
	this.views = [
		new View(),
		new View(),
		new View(),
		new View(),
		new View(),
		new View(),
		new View({ indices:[], index:0, text:"", path:"" }),
		new View(),
	];
}
Machine.prototype = Object.create(Object.prototype, {
	address: { value: null, writable: true },
	broken: { value: false, writable: true },
	title: { value: null, writable: true },
	views: { value: null, writable: true },
	path: { value: null, writable: true },
	lineIndex: { value: -1, writable: true },
	framesView: { value: function() {
		return this.views[0];
	}},
	localsView: { value: function() {
		return this.views[1];
	}},
	globalsView: { value: function() {
		return this.views[2];
	}},
	filesView: { value: function() {
		return this.views[3];
	}},
	breakpointsView: { value: function() {
		return this.views[4];
	}},
	modulesView: { value: function() {
		return this.views[5];
	}},
	fileView: { value: function() {
		return this.views[6];
	}},
	logView: { value: function() {
		return this.views[7];
	}},
});

var model = shell.behavior = Behavior({
	doChangePort: function(shell, port) {
		this.debug.close();
		this.preferences.port = port;
  		this.debug = new KPR.Debug({ port });
  		this.debug.behavior = this;
		this.savePreferences();
		shell.windowTitle = "xsbug @ localhost:" + port;
	},
	doFile: function(shell, viewIndex, lineIndex) {
		trace("doFile " + viewIndex + " " + lineIndex + "\n");
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.views[viewIndex];
			view.lineIndex = lineIndex;
			var line = view.lines[lineIndex];
			//if (line.path)
				this.debug.file(machine.address, viewIndex, line.path, line.line, line.value);
			//else {
			//	this.data.FILE.distribute("onViewChanged", null);
			//}
		}
	},
	doLog: function(shell, viewIndex, lineIndex) {
		trace("doLog " + viewIndex + " " + lineIndex + "\n");
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.views[viewIndex];
			view.lineIndex = lineIndex;
			var item = view.lines[lineIndex];
			this.debug.file(machine.address, viewIndex, item.name, -1, item.value);
		}
	},
	doSearch: function(shell, string) {
		var indices = [];
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.fileView();
			var text = view.text;
			var offset, index = 0;
			if (view.indices && view.index && view.index >= 0) {
				offset = view.indices[view.index];
			}
			if (text && string && string.length) {
				var regex = new RegExp(string,"gi");
				var result;
				while ( (result = regex.exec(text)) ) {
					if (offset && (result.index == offset))
						index = indices.length;
					indices.push(result.index);
				}
			}
			view.indices = indices;
			view.index = index;
			view.string = string;
			view.stringLength = (string) ? string.length : 0;
			var container = this.data.FILE;
			if (container) {
				container.distribute("onFound", view);
				container.distribute("onSelect", view);
			}
		}
	},
	doSelectNext: function(shell) {
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.fileView();
			view.index += 1;
			var container = this.data.FILE;
			if (container)
				container.distribute("onSelect", view);
		}
	},
	doSelection: function(shell, viewIndex, lineIndex) {
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.views[viewIndex];
			view.lineIndex = lineIndex;
		}
	},
	doSelectPrevious: function(shell) {
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.fileView();
			view.index -= 1;
			var container = this.data.FILE;
			if (container)
				container.distribute("onSelect", view);
		}
	},
	doToggle: function(shell, viewIndex, lineIndex) {
		var machine = this.data.currentMachine;
		if (machine) {
			var view = machine.views[viewIndex];
			var line = view.lines[lineIndex];
			this.debug.toggle(machine.address, viewIndex, line.name, line.value);
		}
	},
	findMachineIndex: function(address) {
		var index = -1;
		this.data.machines.some(function(machine, i) {
			if (machine.address == address) {
				index = i;
				return true;
			}
		});
		return index;
	},
	getLocalPath: function(path) {
		var mapping = model.data.mapping;
		for (var c = mapping.length, i = c - 1; i >= 0; i--) {
			var map = mapping[i];
			var value = map.value;
			var remote;
			if ("remote" in map)
				remote = map.remote;
			if (remote && (path.indexOf(remote) == 0))
				return value + path.substring(remote.length);
		}
		return path;
	},
	gone: function(address) {
		var machineIndex = this.findMachineIndex(address);
		if (machineIndex >= 0) {
			var data = this.data;
			var machine = data.machines[machineIndex];
			machine.broken = false;
			data.BUTTONS.distribute("doActivate", machine);
		}
	},
	mapPath: function(path) {
		var mapping = this.data.mapping;
		for (var c = mapping.length, i = c - 1; i >= 0; i--) {
			var map = mapping[i];
			var value = map.value;
			var remote;
			if ("remote" in map)
				remote = map.remote;
			if (remote && path.indexOf(remote) == 0)
				return { key: map.key, path: path.substring(remote.length) };
			else if (path.indexOf(value) == 0)
				return { key: map.key, path: path.substring(value.length) };
		}
		return { path: path };
	},
	onLaunch: function(shell) {
  		halfWidth = (shell.width) / 2;
		halfHeight = (shell.height - 80) / 2;
		var mapping = [];
		[ "HOME", "F_HOME", "KPR_HOME", "KDT_HOME", "XS_HOME", "FSK", "KPR", "KDT", "XS", "XS6" ].forEach(function(variable, index) {
			var value = system.getenv(variable);
			if (!value)
				value = getEnvironmentVariable(variable);
			if (value) {
				value = value.replace("//", "/");
				if (value.charAt(value.length - 1) == '/')
					value = value.substring(0, value.length - 1);
				trace(variable + " = " + value + "\n");
				mapping.push({ key: variable, value: value, remote: null });
			}
		});
		var preferences = this.preferences = this.readPreferences(shell, "preferences", {
			port: 5003,
			currentTab: 0,
			width: 800,
			height: 600,
			horizontalDividerLeft: halfHeight,
			horizontalDividerRight: halfHeight,
			verticalDivider: halfWidth,
			mapping: mapping,
		});
		if (preferences.horizontalDividerLeft > shell.height - dividerMin - 80)
			preferences.horizontalDividerLeft = shell.height - dividerMin - 80;
		if (preferences.horizontalDividerRight > shell.height - dividerMin - 80)
			preferences.horizontalDividerRight = shell.height - dividerMin - 80;
		if (preferences.verticalDivider > shell.width - dividerMin)
			preferences.verticalDivider = shell.width - dividerMin;
		this.data = {
			currentMachine: null,
			currentTab: preferences.currentTab,
			machines: [],
			horizontalDividerLeft: preferences.horizontalDividerLeft,
			horizontalDividerRight: preferences.horizontalDividerRight,
			verticalDivider: preferences.verticalDivider,
			width: shell.width, //preferences.width,
			height: shell.height - 80, // preferences.height - 80,
			mapping: preferences.mapping,
		}
  		this.debug = new KPR.Debug({ port: preferences.port });
		shell.windowTitle = "xsbug @ localhost:" + preferences.port;
  		this.debug.behavior = this;
 		shell.add(new Screen(this.data));
		shell.first.distribute("onTabSelected", this.data.currentTab);
	},
	onQuit: function(shell) {
		//this.savePreferences();
		if (this.debug) {
			this.debug.close();
			delete this.debug
		}
	},
	onMachineCommand: function(shell, command) {
		var machine = this.data.currentMachine;
		if (machine) {
			switch (command) {
			case "go":
				var data = this.data;
				var container;
				machine.framesView().lines = lines;
				machine.framesView().lineIndex = -1;
				var lines = [];
				container = data.CALLS;
				[
					{ container: data.CALLS, index: 0 },
					{ container: data.LOCALS, index: 1 },
					{ container: data.GLOBALS, index: 2 },
					{ container: data.MODULES, index: 5 },
				].forEach(function(item, index) {
					var view = machine.views[item.index];
					view.lines = lines;
					view.lineIndex = -1;
					view.title = null;
					if (item.container)
						item.container.distribute("onViewChanged", view);
				});
				var view = machine.fileView();
				var container = data.FILE;
				var title = view.title;
				container.distribute("onCallsChanged", []);
				container.distribute("onBreakpointsChanged", machine.breakpointsView().lines.filter(function(item) {
					return item.path == view.path;
				}));
				machine.broken = false;
				this.selectMachine(machine);
				this.debug[command](machine.address);
				break;
			case "addBreakpoint":
				var view = machine.fileView();
				var broken = machine.broken;
				var path = view.path;
				this.debug.addBreakpoint(machine.address, path, view.lineIndex + 1);
// 				if (!broken)
// 					this.debug.go(machine.address);
				break;
			case "removeBreakpoint":
				var view = machine.fileView();
				var path = view.path;
				this.debug.removeBreakpoint(machine.address, path, view.lineIndex + 1);
				break;
			default:
				this.debug[command](machine.address);
				break;
			}
		}
	},
	onMachineFileChanged: function(address, viewIndex, lines, path, lineIndex) {
		var machineIndex = this.findMachineIndex(address);
		if (machineIndex >= 0) {
			var data = this.data;
			var machine = data.machines[machineIndex];
			var view = machine.views[viewIndex];
			var text = null;
			if (path) {
				var uri = "file://" + this.getLocalPath(path);
				if (Files.exists(uri)) {
					try {
						text = Files.readText(uri);
					}
					catch(e) {
					}
				}
			}
			if (text) {
				view.lineIndex = lineIndex - 1;
				view.path = path;
				view.text = text;
				view.title = this.mapPath(path);
			}
			else {
				view.lineIndex = -1;
				view.path = "";
				view.text = "";
				view.title = "";
			}
			view.indices = [];
			view.index = 0;
			if (data.currentMachine == machine) {
				var container = data.FILE;
				if (container) {
					container.distribute("onViewChanged", view);
					var calls = machine.framesView();
					if (calls.lines.length) {
						var line = calls.lines[calls.lineIndex].line;
						container.distribute("onCallsChanged", calls.lines.filter(function(item) {
							return (item.path == view.path) && (item.line == line);
						}));
					}
					container.distribute("onBreakpointsChanged", machine.breakpointsView().lines.filter(function(item) {
						return item.path == view.path;
					}));
				}
				if (view.string)
					this.doSearch(shell, view.string);
			}
			this.onMachineFilesSelectionUpdate(machine);
//			view = machine.filesView();
//			lines = view.lines;
//			view.lineIndex = -1;
//			for (var c = lines.length, i = c - 1; i >= 0; i--) {
//				var item = lines[i];
//				if (item.path == path) {
//					view.lineIndex = i;
//					break;
//				}
//			}
//			if (data.currentMachine == machine) {
//				container = data.FILES;
//				container.distribute("onSelectionChanged", view);
//			}
		}
	},
	onMachineFilesSelectionUpdate: function(machine) {
		var data = this.data;
		var path = machine.fileView().path;
		var view = machine.filesView();
		var lines = view.lines;
		view.lineIndex = -1;
		for (var c = lines.length, i = c - 1; i >= 0; i--) {
			var item = lines[i];
			if (item.path == path) {
				view.lineIndex = i;
				break;
			}
		}
		if (data.currentMachine == machine) {
			var container = data.FILES;
			container.distribute("onSelectionChanged", view);
		}
	},
	onMachineRegistered: function(address) {
		this.data.machines.push(new Machine(address));
		this.data.MACHINES.delegate("onMachinesChanged", this.data.machines);
		this.updateMachineMenu();
	},
	onMachineTitleChanged: function(address, title) {
		var index = this.findMachineIndex(address);
		if (index >= 0) {
			var machine = this.data.machines[index];
			if (title.indexOf("file://") == 0) {
				title = title.substring(7);
				var map = this.mapPath(title);
				title = "";
				if (map.key)
					title += "$" + map.key;
				title += map.path;
				machine.title = title;
			}
			else
				machine.title = title;
			this.data.MACHINES.delegate("onMachinesChanged", this.data.machines);
			this.updateMachineMenu();
		}
	},
	onMachineUnregistered: function(address) {
		var data = this.data;
		var currentMachine = data.currentMachine;
		var index = this.findMachineIndex(address);
		if (index >= 0) {
			data.machines.splice(index, 1);
			data.MACHINES.delegate("onMachinesChanged", this.data.machines);
			if (currentMachine && (currentMachine.address == address))
				this.selectMachine(null);
			this.updateMachineMenu();
		}
	},
	onMachineViewChanged: function(address, viewIndex, lines) {
		var machineIndex = this.findMachineIndex(address);
		trace("onMachineViewChanged " + machineIndex + " " + viewIndex + "\n");
		if (machineIndex >= 0) {
			var data = this.data;
			var machine = data.machines[machineIndex];
			var view = machine.views[viewIndex];
			view.lines = lines;
			if (lines.length) {
				if (view.lineIndex < 0) view.lineIndex = 0;
				machine.broken = true;
				this.selectMachine(machine);
			}
			else
				view.lineIndex = -1;
			var container;
			switch (viewIndex) {
			case 0:
				container = data.CALLS;
				machine.views[viewIndex].lineIndex = 0;
				this.selectTab(shell, 0);
				break;
			case 1: container = data.LOCALS; break;
			case 2: container = data.GLOBALS; break;
			case 3:
				container = data.FILES;
				view.lines.sort(function(a,b){
					return a.path.localeCompare(b.path);
				});
				break;
			case 4:
				container = data.BREAKPOINTS;
// @@ does not work (crash in c or js)
//				view.lines.sort(function(a,b) {
//					var value = a.path.localeCompare(b.path);
//					if (value == 0)
//						value = (a.line < b.line) ? -1 : 1;
//					return value;
//				});
				view.lines.sort(function(a,b) {
					return (a.path + ":" + pad(a.line, 5)).localeCompare((b.path + ":" + pad(b.line, 5)))
				});
				break;
			case 5: container = data.MODULES; break;
			}
			if (data.currentMachine == machine) {
				if (container)
					container.distribute("onViewChanged", view);
				if (viewIndex == 0) {
					container = data.FILE;
					if (container) {
						var path = machine.fileView().path;
						if (path) {
							var line = view.lines[view.lineIndex].line;
							container.distribute("onCallsChanged", view.lines.filter(function(item) {
								return (item.path == path) && (item.line == line);
							}));
						}
					}
				}
				if (viewIndex == 4) {
					container = data.FILE;
					if (container) {
						var path = machine.fileView().path;
						if (path) {
							container.distribute("onBreakpointsChanged", view.lines.filter(function(item) {
								return item.path == path;
							}));
						}
					}
				}
				if (viewIndex == 3) {
					this.onMachineFilesSelectionUpdate(machine);
				}
			}
		}
	},
	onMachineViewPrint: function(address, viewIndex, line) {
		var machineIndex = this.findMachineIndex(address);
		if (machineIndex >= 0) {
			var data = this.data;
			var machine = data.machines[machineIndex];
			var view = machine.views[viewIndex];
			var split = line.split("\n");
			split.forEach(function(string, i) {
				if (string.length) {
					if (view.lines.length) {
						var line = view.lines[view.lines.length - 1];
						line.name += string;
						string = line.name;
					}
					else
						view.lines.push({ name: string, value: null });
				}
				else
					view.lines.push({ name: "", value: null });
				if (data.currentMachine == machine)
					data.LOG.distribute("onPrint", string, i > 0);
			});
		}
	},
	onMachineViewTitle: function(address, viewIndex, title) {
		var machineIndex = this.findMachineIndex(address);
		if (machineIndex >= 0) {
			var data = this.data;
			var machine = data.machines[machineIndex];
			var view = machine.views[viewIndex];
			view.path = title;
			view.title = this.mapPath(title);
//			if (data.currentMachine == machine) {
//				var container;
//				switch (viewIndex) {
//				case 0: container = data.CALLS; break;
//				case 1:	container = data.LOCALS; break;
//				case 2: container = data.GLOBALS; break;
//				case 3: container = data.FILES; break;
//				case 4: container = data.BREAKPOINTS; break;
//				case 5: container = data.MODULES; break;
//				}
//				if (container)
//					container.distribute("onViewTitleChanged", view);
//			}
		}
	},
	onTouchMoved: function(shell, id, x, y, ticks) {
		this.touchX = x;
		this.touchY = y;
	},
	onTouchScrolled: function(shell, touched, dx, dy, ticks) {
		var content = shell.hit(this.touchX, this.touchY);
		while (content) {
			if (content instanceof Scroller)
				content.delegate("onTouchScrolled", touched, dx, dy, ticks);
			content = content.container;
		}
	},
	readPreferences: function(shell, name, preferences) {
		try {
			var url = mergeURI(Files.preferencesDirectory, di + "." + name + ".json");
			if (Files.exists(url))
				return JSON.parse(Files.readText(url));
		}
		catch(e) {
		}
		return preferences;
	},
	savePreferences: function() {
		var data = this.data;
		var preferences = this.preferences;
		preferences.currentTab = data.currentTab;
		preferences.horizontalDividerLeft = data.CALLS.height;
		preferences.horizontalDividerRight = data.FILE.height;
		preferences.verticalDivider = data.GLOBALS.container.width;
		preferences.width = shell.width;
		preferences.height = shell.height;
		this.writePreferences(shell, "preferences", preferences);
	},
	writePreferences: function(shell, name, preferences) {
		try {
			var url = mergeURI(Files.preferencesDirectory, di + "." + name + ".json");
			Files.writeText(url, JSON.stringify(preferences, null, '\t'));
		}
		catch(e) {
		}
	},
	selectMachine: function(machine) {
		var data = this.data;
		if (!machine) {
			// select next broken machine if any
			var first = null;
			var fallback = null;
			for (var index = data.machines.length - 1; index >= 0; index--) {
				fallback = data.machines[index];
				if (fallback.broken) break;
				else fallback = null;
				if (!first) first = fallback;
			}
			if (fallback)
				machine = fallback;
			else if (first)
				machine = first;
		}
		if (data.currentMachine != machine) {
			if (data.currentMachine) data.currentMachine.check = false;
			if (machine) machine.check = true;
			data.currentMachine = machine;
			shell.first.distribute("onMachineSelected", machine);
			data.CALLS.distribute("onViewChanged", machine ? machine.framesView() : null);
			data.LOCALS.distribute("onViewChanged", machine ? machine.localsView() : null);
			data.GLOBALS.distribute("onViewChanged", machine ? machine.globalsView() : null);
			data.FILES.distribute("onViewChanged", machine ? machine.filesView() : null);
			data.BREAKPOINTS.distribute("onViewChanged", machine ? machine.breakpointsView() : null);
			data.MODULES.distribute("onViewChanged", machine ? machine.modulesView() : null);
			data.FILE.distribute("onViewChanged", machine ? machine.fileView() : null);
			data.LOG.distribute("onViewChanged", machine ? machine.logView() : null);
			
//			if (machine)
//				this.onMachineFilesSelectionUpdate(machine);
		}
		data.BUTTONS.distribute("doActivate", machine);
	},
	selectTab: function(shell, tab) {
		this.data.currentTab = tab;
		shell.first.distribute("onTabSelected", tab);
	},
	
	// menus
	
	// Debug
	canKill: function(shell, item) {
		return this.data.currentMachine != null;
	},
	doKill: function(shell, item) {
		this.onMachineCommand(shell, "abort");
	},
	// --
	canRun: function(shell, item) {
		var machine = this.data.currentMachine;
		return machine && machine.broken;
	},
	canStep: function(shell, item) {
		var machine = this.data.currentMachine;
		return machine && machine.broken;
	},
	canStepIn: function(shell, item) {
		var machine = this.data.currentMachine;
		return machine && machine.broken;
	},
	canStepOut: function(shell, item) {
		var machine = this.data.currentMachine;
		return machine && machine.broken;
	},
	doRun: function(shell, item) {
		this.onMachineCommand(shell, "go");
	},
	doStep: function(shell, item) {
		this.onMachineCommand(shell, "step");
	},
	doStepIn: function(shell, item) {
		this.onMachineCommand(shell, "stepIn");
	},
	doStepOut: function(shell, item) {
		this.onMachineCommand(shell, "stepOut");
	},
	// --
	canSetBreakpoint: function(shell, item) {
		var machine = this.data.currentMachine;
		return machine && (machine.fileView().lineIndex > 0);
	},
	canClearBreakpoint: function(shell, item) {
		var machine = this.data.currentMachine;
		return machine && (machine.fileView().lineIndex > 0);
	},
	canClearAllBreakpoints: function(shell, item) {
		return this.data.currentMachine;
	},
	doSetBreakpoint: function(shell, item) {
		this.onMachineCommand(shell, "addBreakpoint");
	},
	doClearBreakpoint: function(shell, item) {
		this.onMachineCommand(shell, "removeBreakpoint");
	},
	doClearAllBreakpoints: function(shell, item) {
		this.onMachineCommand(shell, "resetBreakpoints");
	},
	// Edit
	canFind: function(shell, item) {
		var machine = model.data.currentMachine;
		var view = machine ? machine.fileView() : null;
		return view && view.text;
	},
	canFindNext: function(shell, item) {
		var machine = model.data.currentMachine;
		var view = machine ? machine.fileView() : null;
		return view && (view.index + 1 < view.indices.length);
	},
	canFindPrevious: function(shell, item) {
		var machine = model.data.currentMachine;
		var view = machine ? machine.fileView() : null;
		return view && (view.index > 1);
	},
	doFind: function(shell, item) {
		model.data.FILE.first.distribute("onTap");
	},
	doFindNext: function(shell, item) {
		model.doSelectNext(shell);
	},
	doFindPrevious: function(shell, item) {
		model.doSelectPrevious(shell);
	},
	// Machines
	canMachine: function(shell, item) {
		return true;
	},
	doMachine: function(shell, item) {
		this.selectMachine(item);
	},
	// View
	canCalls: function(shell, item) {
		return true;
	},
	canGlobals: function(shell, item) {
		return true;
	},
	canModules: function(shell, item) {
		return true;
	},
	canFiles: function(shell, item) {
		return true;
	},
	canBreakpoints: function(shell, item) {
		return true;
	},
	canKinoma: function(shell, item) {
		return true;
	},
	doCalls: function(shell, item) {
		this.selectTab(shell, 0);
	},
	doGlobals: function(shell, item) {
		this.selectTab(shell, 1);
	},
	doModules: function(shell, item) {
		this.selectTab(shell, 2);
	},
	doFiles: function(shell, item) {
		this.selectTab(shell, 3);
	},
	doBreakpoints: function(shell, item) {
		this.selectTab(shell, 4);
	},
	doKinoma: function(shell, item) {
		this.selectTab(shell, 5);
	},
	
	updateMachineMenu: function() {
		var items = this.data.machines;
		shell.menus[2].items = items;
		items.forEach(function(item, index) {
			item.key = (index + 1);
		});
		shell.updateMenus();
	},
	updateViewMenu: function() {
		var items = shell.menus[3].items;
		var currentTab = this.data.currentTab;
		items.forEach(function(item, index) {
			item.check = index == currentTab;
		});
		shell.updateMenus();
	},

});
shell.behavior.onLaunch(shell);
