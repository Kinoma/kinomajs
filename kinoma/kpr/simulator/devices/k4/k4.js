//@module
/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
var blackSkin = new Skin("black")
var blueSkin = new Skin("blue")
var redSkin = new Skin("red")
var greenSkin = new Skin("green")

var verticalTexture = new Texture("k4v.png")
var verticalSkin = new Skin(verticalTexture, {x:0, y:0, width:400, height:500})
var Vertical = function(data) {
	Container.call(this, null, verticalSkin)
	var screen = new Container({left:80, width:240, top:90, height:320}, blackSkin)
	screen.clip = true
	data.screen = screen
	this.add(screen)
}
Vertical.prototype = Container.prototype
exports.Vertical = Vertical

var gShrunkLayer = null
var gDeviceMockup = null

var sizeToggleTexture = new Texture("simulator-size-toggle01.png")
var sizeToggleSkin = new Skin(sizeToggleTexture, {x:0, y:0, width:43, height:74})

var proximityToggleTexture = new Texture("simulator-size-toggle02.png")
var proximityToggleSkin = new Skin(proximityToggleTexture, {x:0, y:0, width:43, height:39})

var horizontalTexture = new Texture("k4h.png")
var horizontalSkin = new Skin(horizontalTexture, {x:0, y:0, width:500, height:400})
var Horizontal = function(data) {
	Container.call(this, null)

	var deviceMockup = gDeviceMockup = new Container({width:500, height:400}, horizontalSkin)
	this.add(deviceMockup)

	var leftButton = new Content({left:30, top:90, width:50, height:240})
	leftButton.active = true
	var leftButtonBehavior = {}
	leftButtonBehavior.onTouchBegan = function(button, id, x, y, ticks) {
		shell.invoke(new Message("xkpr://k2pencilTest.kinoma.com/handleEvent?eventID=leftButtonDown"))
	}
	leftButton.behavior = leftButtonBehavior
	this.add(leftButton)

	var rightButton = new Content({left:410, top:90, width:50, height:240})
	rightButton.active = true
	var rightButtonBehavior = {}
	rightButtonBehavior.onTouchBegan = function(button, id, x, y, ticks) {
		shell.invoke(new Message("xkpr://k2pencilTest.kinoma.com/handleEvent?eventID=rightButtonDown"))
	}
	rightButton.behavior = rightButtonBehavior
	this.add(rightButton)

	var background = new Container({left:0, top:0, width:1000, height:1000})
	this.add(background)

	var zoomButton = new Content({right : 0, top : 180, height : 74 }, sizeToggleSkin)
	zoomButton.active = true
	zoomButtonBehavior = {}
	zoomButtonBehavior.onTouchBegan = function(button, id, x, y, ticks) {
		this.toggleZoom()
	}
	zoomButtonBehavior.toggleZoom = function() {
		shell.invoke(new Message("xkpr://k2pencilTest.kinoma.com/handleEvent?eventID=zoom"))
/*		if (gShrunkLayer == null) {
			var layer = new Layer
			layer.attach(gDeviceMockup)
			layer.origin = { x :235, y : 360 }
			layer.scale = { x : 0.75, y : 0.75 }
			gShrunkLayer = layer
		}
		else {
			gShrunkLayer.detach()
			gShrunkLayer = null
		}
*/
	}
	zoomButton.behavior = zoomButtonBehavior
	background.add(zoomButton)
	
	var proximityButton = new Content({right : 0, top : 280, height : 74 }, proximityToggleSkin)
	proximityButton.active = true
	proximityButtonBehavior = {}
	proximityButtonBehavior.onTouchBegan = function(button, id, x, y, ticks) {
		shell.invoke(new Message("xkpr://k2pencilTest.kinoma.com/handleEvent?eventID=proximity"))
	}
	proximityButton.behavior = proximityButtonBehavior
	background.add(proximityButton)
	
	var blackBackground = new Content( { width : 320, height : 240 }, blackSkin)
	this.add(blackBackground)
	
	var screen = new Container( { width : 320, height : 240 } )
	screen.clip = true
	data.screen = screen
	
	var screenBehavior = {}
	screenBehavior.onKeyDown = function(content, key, modifiers, count, ticks) {
		switch (key) {
			case "p":
				shell.invoke(new Message("xkpr://k2pencilTest.kinoma.com/handleEvent?eventID=proximity"))
			break
			case "z":
				zoomButtonBehavior.toggleZoom()
			break
		}
	}
	screen.behavior = screenBehavior
	this.add(screen)

}
Horizontal.prototype = Container.prototype
exports.Horizontal = Horizontal