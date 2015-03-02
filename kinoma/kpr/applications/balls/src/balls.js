//@program
/*
  Copyright 2011-2014 Marvell Semiconductor, Inc.

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

var BallBehavior = function (delta) {
	this.dx = delta;
	this.dy = delta
}
BallBehavior.prototype = Object.create(Object.prototype, {
	dx: { value: 5, writable: true },
	dy: { value: 5, writable: true },
	x: { value: 0, writable: true },
	y: { value: 0, writable: true },
	width: { value: 0, writable: true },
	height: { value: 0, writable: true },
	onDisplaying: {
		value: function(ball) {
			ball.start();
			this.width = ball.container.width - ball.width;
			this.height = ball.container.height - ball.height;
		}
	},
	onTimeChanged: {
		value: function(ball) {
			var dx = this.dx;
			var dy = this.dy;
			ball.moveBy(dx, dy);
			var x = ball.x - ball.container.x;
			var y = ball.y - ball.container.y;
			if ((x < 0) || (x > this.width)) dx = -dx;
			if ((y < 0) || (y > this.height)) dy = -dy;
			this.dx = dx;
			this.dy = dy;
		}
	},
});

var build = function(container) {
	container.skin = new Skin("white");
	var ballTexture = new Texture("balls.png");
	var ballSkin = new Skin(ballTexture, {x:0, y:0, width:30, height:30}, 30, 0);
	var ball = new Content({left:0, width: 30, top: 0, height: 30}, ballSkin);
	ball.behavior = new BallBehavior(6);
	ball.variant = 0;
	container.add(ball);
	var ball = new Content({right:0, width: 30, top: 0, height: 30}, ballSkin);
	ball.behavior = new BallBehavior(5);
	ball.variant = 1;
	container.add(ball);
	var ball = new Content({right:0, width: 30, bottom: 0, height: 30}, ballSkin);
	ball.behavior = new BallBehavior(4);
	ball.variant = 2;
	container.add(ball);
	var ball = new Content({left:0, width: 30, bottom: 0, height: 30}, ballSkin);
	ball.behavior = new BallBehavior(3);
	ball.variant = 3;
	container.add(ball);
}

application.behavior = {
	onAdapt: function(application) {
		application.empty();
		build(application);
	},
	onLaunch: function(application) {
		build(application);
	},
}
