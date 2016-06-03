//@module
/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
var PinsSimulators = require ("PinsSimulators");
var SCROLLER = require ("mobile/scroller");
var CONTROL = require ("mobile/control");
var THEME = require('themes/flat/theme');
var BUTTONS = require('controls/buttons');

// So we can run in Studio
var MIXED = require('creations/mixedInput');
//var MIXED = require('creationsFOO/mixedInput');


// BLL
exports.pins = {
	serial: { type: "Serial", tx: 31, rx: 33, baud: 9600 }
};

var PinsSimulators = require ("PinsSimulators");

// styles
var receiveStyle = new Style( { font:"18px", color: "gray", horizontal:"left" } );
receiveStyle.margins = { left: 10, right: 10 };

var buttonStyle = new Style( { font: "22px", color: "white", horizontal: "center" } );
var displayStyle = new Style( { font:"bold 22px", color: "black", horizontal: "left" } );

var consoleStyle = new Style( { font: "18px Fira Mono", color: "black", align: "left", lines: "all" } );
consoleStyle.margins = { left: 10, right: 10 };

// skins
var displaySkin = new Skin({ fill: "white" } );

var SerialBehavior = function(column, data) {
	Behavior.call(this, column, data);
}
SerialBehavior.prototype = Object.create(Behavior.prototype, {
	onCreate: { value: function(column, data) {
        column.partContentsContainer.add(new SerialColumn(data)); 
	}},
});

var graySkin = new Skin( { fill: "#EEEEEE" } );


        	var gConsoleLines = [];
        	
        	var consoleClear = function() {
        		gConsoleLines = [];
        	}
        	var consoleAppendOutputLine = function(line) {
        		gConsoleLines.push( { dir : "out", descriptions : line } );
        	}
        	var consoleAppendInputLine = function(line) {
        		gConsoleLines.push( { dir : "in", descriptions : line } );
        	}



var SerialColumn = Column.template(function($) { return {
	left:0, right:0,
	behavior: Object.create(Behavior.prototype, {
		onCreate: { value: function(column, data) {
		}}
	}),
	contents: [
		Container($, {
			left: 20, right: 0, top: 0, bottom: 0,
			style: displayStyle,
			anchor: "DISPLAY",
			skin: displaySkin,
			contents: [
				Column($, {
					left: 0, right: 0, top: 10,
					contents: [
						Line($, {
							left: 0, top: 0,
							contents: [
								BUTTONS.LabeledButton( { name : "Transmit" }, { 
									width: 100, left: 0, height: 30, style: buttonStyle,
									behavior: BUTTONS.LabeledButtonBehavior({
										onTap: function(button) {
											var label = button.container.scroller.inputLabel;
											gStringToRead = label.string;
											label.string = "";
										}
									})
								}),
								Scroller($, {
									left: 10, width: 180, height: 30, name: "scroller", clip: true, skin: graySkin,
									contents: [
										Label($, {
											name: "inputLabel", left: 0, height: 30, active: true, editable: true, style: receiveStyle, string: "",
									
											behavior: CONTROL.FieldLabelBehavior({
												onDisplayed: function(label) {
													label.focus();
													label.start();
												},
												onTouchBegan: function(label, id, x, y, ticks) {
													label.focus();
													CONTROL.FieldLabelBehavior.prototype.onTouchBegan.call(this, label, id, x, y, ticks);
												},
												onTimeChanged: function(label) {
								//					label.select(0, label.length + 1);
													label.container.reveal(label.selectionBounds);
												}
											})
										}),
									]
								}),
							]
						}),
						
						Label($, {
							left: -10, width: 130, height: 30, style: receiveStyle, string: "Received"
						}),

						
						MIXED.MixedConsole($, { 
							left: 0, right: 0, height: 100, clip: true, anchor: "CONSOLE",
							behavior: MIXED.MixedConsoleBehavior({
								writeArgs: function(scroller, args) {
									var description = this.argsToDescription(scroller, args);
									this.addOutputLine(scroller, description);
								},
								argsToDescription: function(scroller, args) {
									var description = [];
									for (var i=0, c=args.length; i < c; i++) {
										var arg = args[i];										
										if (typeof arg == "string") {					// ascii
											description.push( { type: "ascii", chars: arg } );
										}
										else if (arg instanceof Chunk) {				// numeric value -> hex chars
											var hexChars = this.toHexChars(scroller, arg);
											description.push( { type: "hex", chars: hexChars } );
										}
										else if (arg instanceof Array) {				// ending				
											var chars = "";
											for (var n=0, c=arg.length; n < c; n++)
												chars += arg[n];
											description.push( { type: "ending", chars: chars } );
										}
									}
									return description;
								},
								toHexChars: function(scroller, valuesChunk) {
									var hexChars = "";
									for (var i=0, c=valuesChunk.length; i < c; i++) {
										var aByte = valuesChunk.peek(i);
										var hexStr = aByte.toString(16);
										if (aByte < 16)
											hexStr = "0" + hexStr;
										hexChars += hexStr;
									}
									return hexChars;
								}
							})
						}),
						
						Content($, { height: 10 })
					]
				})
			]
		})
	]
}});

exports.configure = function(configuration) {
	this.data = {
		id: 'Serial',
		behavior: SerialBehavior,
		header : { 
			label : "Serial", 
			name : "Tx 31, Rx 33", 
			iconVariant : PinsSimulators.SENSOR_SERIAL
		},
	};
	this.pinsSimulator = shell.delegate("addSimulatorPart", this.data);
}

exports.close = function() {
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
}

exports.setBaud = function(baud) {
	this.baud = baud;
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
	this.serial.close();
	this.serial = PINS.create( { type: "Serial", tx: 31, rx: 33, baud: this.baud } );
	this.serial.init();
	this.pinsSimulator = shell.delegate("addSimulatorPart", this.data);
}

exports.write = function(args) {
	var str = "";	
	for (var i=0; i < args.length; i++)
		str += args[i]
	this.data.CONSOLE.delegate("writeArgs", args);
}

var gStringToRead = "";

exports.readAsIntegerArray = function() {
	var str = gStringToRead;
	
	if (str.length > 0)
		trace("\n readAsIntegerArray: " + str);
		
	var charCodes = [];
	for (var i=0, c=str.length; i < c; i++)
		charCodes.push(str.charAt(i).charCodeAt(0));
	gStringToRead = "";
	return charCodes;
}
