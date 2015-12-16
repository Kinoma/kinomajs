//@module
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
var Point = function(x, y) {
	this.x = x;
	this.y = y;
}
Point.prototype = Object.create(Object.prototype, {
	x: { configurable: true, enumerable: true, writable: true, value: 0  },
	y: { configurable: true, enumerable: true, writable: true, value: 0  }
});

var shortStraw = Object.create(Object.prototype, {
	lineThreshold: {configurable: true, enumerable: true, writable: true, value: 0.95 },
	W: {configurable: true, enumerable: true, writable: true, value: 3 },
	scaleFactor: {configurable: true, enumerable: true, writable: true, value: 10 },
	resampleAndGetCorners: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var S = this.determineResampleSpacing(points)
				var resampled = this.resamplePoints(points, S)
				var corners = this.getCorners(resampled)
				return corners
			}
	},
	resample: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var S = this.determineResampleSpacing(points)
				return this.resamplePoints(points, S)
			}
	},
	resamplePoints: {configurable: true, enumerable: true, writable: true, value: 
		function(points, S) {
				var D = 0
				var resampled = new Array(points[0])
				for (var i=1; i < points.length; i++) {
					var d = this.distance(points, i-1, i)
					if ((D + d) >= S) {
						var q = new Point
						q.x = points[i - 1].x + ((S - D) / d) * (points[i].x - points[i - 1].x)
						q.y = points[i - 1].y + ((S - D) / d) * (points[i].y - points[i - 1].y)
						resampled.push(q)
						points.splice(i, 0, q)
						D = 0
					}
					else
						D = D + d
				}
				return resampled
			}
	},
	determineResampleSpacing: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var topLeft = new Point(this.minX(points), this.minY(points))
				var bottomRight = new Point(this.maxX(points), this.maxY(points))
				var points = [topLeft, bottomRight]
				var diagonal = this.distance(points, 0, 1)
				var s = diagonal / this.scaleFactor
				return s
			}
	},
	getCorners: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var straws = []
				var corners = [0]
				var W = this.W
				
				for (var i=0; i < W; i++)
					straws.push(Infinity)
					
				for (var i=W; i < points.length - W; i++) {
					var strawDistance = this.distance(points, i - W, i + W)
					straws.push(strawDistance)
				}

				for (var i=0; i < W; i++)
					straws.push(Infinity)
				
				var t = this.median(straws) * this.lineThreshold
				
				for (var i=W; i < points.length - W; i++) {
					if (straws[i] < t) {
						var localMin = Infinity
						var localMinIndex = i
						while (i < straws.length && straws[i] < t) {
							if (straws[i] < localMin) {
								localMin = straws[i]
								localMinIndex = i
							}
							i = i + 1
						}
						corners.push(localMinIndex)
					}	
				}
				corners.push(points.length - 1)

				corners = this.postProcessCorners(points, corners, straws)
				return corners
			}
	},
	pathDistance: {configurable: true, enumerable: true, writable: true, value: 
		function(points, a, b) {
				var d = 0
				for (var i=a; i < b; i++)
					d = d + this.distance(points, i, i + 1)
				return d
			}
	},
	distance: {configurable: true, enumerable: true, writable: true, value: 
		function(points, a, b) {
				var dx = points[b].x - points[a].x
				var dy = points[b].y - points[a].y
				return Math.sqrt((dx * dx) + (dy * dy))
			}
	},
	postProcessCorners: {configurable: true, enumerable: true, writable: true, value: 
		function(points, corners, straws) {
				var finished = false
				while (! finished) {
					finished = true
					for (var i=1; i < corners.length; i++) {
						var c1 = corners[i - 1]
						var c2 = corners[i]
						if (false == this.isLine(points, c1, c2)) {
							var newCorner = this.halfwayCorner(straws, c1, c2)
							if (newCorner != -1) {
								corners.splice(i, 0, newCorner)
								finished = false
							}
						}
					}
				}
			
				for (var i=1; i < corners.length - 1; i++) {
					var c1 = corners[i - 1]
					var c2 = corners[i + 1]
					if (this.isLine(points, c1, c2)) {
						corners.splice(i, 1)
						i = i - 1
					}
				}
				return corners
			}
	},
	halfwayCorner: {configurable: true, enumerable: true, writable: true, value: 
		function(straws, a, b) {
				var quarter = Math.round((b - a) / 4)
				if (quarter < 1)
					return -1
				var minValue = Infinity
				var minIndex = -1
				for (var i = a + quarter; i < b - quarter; i++) {
					if (straws[i] < minValue) {
						minValue = straws[i]
						minIndex = i
					}
				}
				return minIndex
			}
	},
	isLine: {configurable: true, enumerable: true, writable: true, value: 
		function(points, a, b) {
				var distance = this.distance(points, a, b)
				var pathDistance = this.pathDistance(points, a, b)
				return (distance / pathDistance) > this.lineThreshold
			}
	},
	minX: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var minX = Infinity
				for (var i=0; i < points.length; i++) {
					if (points[i].x < minX)
						minX = points[i].x
				}
				return minX
			}
	},
	minY: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var minY = Infinity
				for (var i=0; i < points.length; i++) {
					if (points[i].y < minY)
						minY = points[i].y
				}
				return minY
			}
	},
	maxX: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var maxX = -Infinity
				for (var i=0; i < points.length; i++) {
					if (points[i].x > maxX)
						maxX = points[i].x
				}
				return maxX
			}
	},
	maxY: {configurable: true, enumerable: true, writable: true, value: 
		function(points) {
				var maxY = -Infinity
				for (var i=0; i < points.length; i++) {
					if (points[i].y > maxY)
						maxY = points[i].y
				}
				return maxY
			}
	},
	median: {configurable: true, enumerable: true, writable: true, value: 
		function(a) {
				var numbers = new Array(a.length)
				for (var i=0; i < a.length; i++)
					numbers[i] = a[i]
				var n = numbers.length
				var k = (n & 1) ? (n - 1) / 2 : (n / 2)
				return this.kthSmallest(numbers, n, k)
			}
	},
	kthSmallest: {configurable: true, enumerable: true, writable: true, value: 
		function(a, n, k) {
				var i, j, l, m, x
				l = 0
				m = n - 1
				while (l < m) {
					x = a[k]
					i = l
					j = m
					do {
						while (a[i] < x)
							i++
						while (x < a[j])
							j--
						if (i <= j) {
							this.swap(a, i, j)
							i++
							j--
						}
					} while (i <= j)
					if (j < k)
						l = i
					if (k < i)
						m = j			
				}
				return a[k]
			}
	},
	swap: {configurable: true, enumerable: true, writable: true, value: 
		function(a, i, j) {
				var temp = a[i]
				a[i] = a[j]
				a[j] = temp
			}
	},
});

var Recognizer = exports.Recognizer = function(behavior, content) {
	this.onCreate(behavior, content);
}
Recognizer.prototype = Object.create(Object.prototype, {
	state: { configurable: true, enumerable: true, writable: true, value: 0  },
	calculateAngle: { 
		value: function(fromPoint, toPoint) {
			var dy = toPoint.y - fromPoint.y 
			var dx = toPoint.x - fromPoint.x 
			var anAngle = Math.atan2(dy, dx)
			var inDegrees = (anAngle / Math.PI) *  180 + 180
			return inDegrees
		}
	},
	onCreate: { 
		value: function(behavior, content) {
		}
	},
	onTouchBegan: { 
		value: function(behavior, content, id, x, y, ticks) {
		}
	},
	onTouchCancelled: { 
		value: function(behavior, content, id, x, y, ticks) {
		}
	},
	onTouchEnded: { 
		value: function(behavior, content, id, x, y, ticks) {
		}
	},
	onTouchMoved: { 
		value: function(behavior, content, id, x, y, ticks) {
		}
	},
});

var TapRecognizer = exports.TapRecognizer = function(behavior, content) {
	Recognizer.call(this, behavior, content);
}
TapRecognizer.prototype = Object.create(Recognizer.prototype, {
	taps: { configurable: true, enumerable: true, writable: true, value: null  },
	TAP_X: { configurable: true, enumerable: true, writable: true, value: 8  },
	TAP_Y: { configurable: true, enumerable: true, writable: true, value: 8  },
	TAP_TICKS: { configurable: true, enumerable: true, writable: true, value: 500  },
	onCreate: { 
		value: function(behavior, content) {
			this.taps = [];
		}
	},
	onTouchEnded: { 
		value: function(behavior, content, id, x, y, ticks) {
			var corners = behavior.corners[id];
			if (corners.length == 1) {
				var point = behavior.resampledPoints[id][0];
				var tap;
				if ((id in this.taps) && (tap = this.taps[id]) 
						&& (Math.abs(point.x - tap.x) < this.TAP_X) 
						&& (Math.abs(point.y - tap.y) < this.TAP_Y) 
						&& (Math.abs(point.ticks - tap.ticks) < this.TAP_TICKS)) {
					tap.count++;
				}
				else {
					point.count = 1;
					this.taps[id] = tap = point;
				}
				content.delegate("onTap", tap.count);
			}
		}
	},
});

var SwipeRecognizer = exports.SwipeRecognizer = function(behavior, content) {
	Recognizer.call(this, behavior, content);
}
SwipeRecognizer.prototype = Object.create(Recognizer.prototype, {
	onTouchEnded: { 
		value: function(behavior, content, id, x, y, ticks) {
			var corners = behavior.corners[id];	
			var resampled = behavior.resampledPoints[id];	
			var maxSwipeAngle = 30	
			if (corners.length == 2) {
				var c1 = resampled[corners[0]]
				var c2 = resampled[corners[1]]
				var angle = this.calculateAngle(c1, c2)
				var dx = Math.abs(c2.x - c1.x)
				var dy = Math.abs(c2.y - c1.y)
				if (dx > dy) {
					if (c2.x > c1.x) {
						if (angle <= (180 + maxSwipeAngle) && angle >= (180 - maxSwipeAngle))
							content.delegate("onSwipe", 1, 0);
					}
					else {
						if ((angle <= maxSwipeAngle && angle >= 0) || (angle >= (360 -  maxSwipeAngle) && angle < 360))
							content.delegate("onSwipe", -1, 0);
					}
				}
				else {
					if (c2.y > c1.y) {
						if (angle <= (270 + maxSwipeAngle) && angle >= (270 - maxSwipeAngle))
							content.delegate("onSwipe", 0, 1);
					}
					else {
						if (angle <= (90 + maxSwipeAngle) && angle >= (90 - maxSwipeAngle))
							content.delegate("onSwipe", 0, -1);
					}
				}
			}
		}
	},
});

var Behavior = exports.Behavior = function(content, data, context) {
	Behavior.call(this, content, data, context);
};
Behavior.prototype = Object.create(Behavior.prototype, {
	corners: { configurable: true, enumerable: true, writable: true, value: null  },
	recognizers: { configurable: true, enumerable: true, writable: true, value: null  },
	sampledPoints: { configurable: true, enumerable: true, writable: true, value: null  },
	resampledPoints: { configurable: true, enumerable: true, writable: true, value: null  },
	addRecognizer: { 
		value: function(content, recognizer) {
			this.recognizers.push(recognizer);
		}
	},
	onCreate: { 
		value: function(content, data) {
			this.corners = [];
			this.recognizers = [];
			this.sampledPoints = [];
			this.resampledPoints = [];
		}
	},
	onTouchBegan: { 
		value: function(content, id, x, y, ticks) {
			this.sampledPoints[id] = touches[id];
			var recognizers = this.recognizers;
			var c = recognizers.length;
			for (var i = 0; i < c; i++)
				recognizers[i].onTouchBegan(this, content, id, x, y, ticks);
		}
	},
	onTouchCancelled: { 
		value: function(content, id, x, y, ticks) {
		}
	},
	onTouchEnded: { 
		value: function(content, id, x, y, ticks) {
			this.sampledPoints[id] = this.sampledPoints[id].concat(touches[id]);
			var sampledPoints = this.sampledPoints[id];
			var spacing = shortStraw.determineResampleSpacing(sampledPoints);
			if (spacing) {
				var resampledPoints = this.resampledPoints[id] = shortStraw.resamplePoints(sampledPoints, spacing);
				this.corners[id] = shortStraw.getCorners(resampledPoints);
			}
			else {
				this.resampledPoints[id] = [ sampledPoints[0] ];
				this.corners[id] = [ 0 ];
			}
			var recognizers = this.recognizers;
			var c = recognizers.length;
			for (var i = 0; i < c; i++)
				recognizers[i].onTouchEnded(this, content, id, x, y, ticks);
			//delete this.resampledPoints[id];
			//delete this.sampledPoints[id];
		}
	},
	onTouchMoved: { 
		value: function(content, id, x, y, ticks) {
			this.sampledPoints[id] = this.sampledPoints[id].concat(touches[id]);
			var recognizers = this.recognizers;
			var c = recognizers.length;
			for (var i = 0; i < c; i++)
				recognizers[i].onTouchMoved(this, content, id, x, y, ticks);
		}
	},
});
