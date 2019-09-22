# Transforming Content at Runtime

KinomaJS enables developers to efficiently transform content in various ways at runtime by using the `layer` object, a `container` object that caches its contents in a bitmap. Layers are commonly used by transitions to temporarily cache and animate parts of the containment hierarchy. If you want a container that you can rotate, scale, skew, translate, adjust the opacity of, and so on, you will need to use a layer. In this tutorial you will learn to create `layer` objects and behaviors that change their appearance.

A full description of the `layer` object (and other objects) can be found in the [*KinomaJS JavaScript Reference*](../../../../../xs6/xsedit/features/documentation/docs/javascript/javascript.md) document.

## Getting Started

Start a new application project and add the following templates to your `main.js` file. You will use these templates to create your `layer` objects.

```
var ContentSample = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: $.fillColor }),
	contents: [
		new Label({ top:0, bottom:0, right:0, left:0, style: smallText, 
		   string: $.string })
	]
}));
	
var LayerExample = Layer.template($ => ({ 
	top: $.top, height:100, left: $.left, width:100,
	behavior: $.behavior,
	contents: [
		new ContentSample({ string: $.string, fillColor: $.fillColor })
	]
}));
```
	
## Example 1: Changing Opacity

This example is a layer that changes opacity. Create a behavior that increases the opacity until it reaches 1 (the maximum) and then sets it to 0 and starts again.

```
var opacityLayerBehavior = Behavior({
	onCreate: function(layer, data) {
		layer.opacity = 0;
		layer.interval = 50;
		layer.start();
	},
	onTimeChanged: function(layer) {
		layer.opacity = (layer.opacity + 0.05)%1;
	}
});
```
	
Create a container to fill the whole screen; then put a layer in it and add it to the application.

```
var mainContainer = new Container({
	left: 0, right: 0, top: 0, bottom: 0, skin: whiteSkin, style: smallText,
	contents: [
		new LayerExample({ top: 100, left: 200, fillColor: "#CC7E1A", 
		   string: "opacity", behavior: opacityLayerBehavior }),
	]
});
application.add(mainContainer);
```
	
Figure 1 shows what you will see if you run this application on the Kinoma Create simulator.

**F1gure 1.** Running the Opacity Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/Lhcbq-LUFaw?rel=0&amp;vq=hd1080" frameborder="0" allowfullscreen><a href="https://www.youtube.com/embed/Lhcbq-LUFaw?rel=0&amp;vq=hd1080">Watch Video</a></iframe>
	
## Example 2: Changing Rotation and Skew

You can follow the same process with other attributes, such as `rotation` and `skew`. Add these two behaviors to your `main.js` file:

```
var rotationLayerBehavior = Behavior({
	onCreate: function(layer, data) {
		layer.rotation = 0;
		layer.origin = {x: Math.floor(layer.width / 2), y: Math.floor(layer.height / 2)};  // This will make the object spin in place; you can comment out this entire line to see the difference.
		layer.start();
	},
	onTimeChanged: function(layer) {
		layer.rotation = (layer.rotation+1)%360
	}
});
	
var skewLayerBehavior = Behavior({
	onCreate: function(layer, data) {
		this.x = this.y = 0;
		layer.skew = {x: this.x, y: this.y};
		layer.start();
	},
	onTimeChanged: function(layer) {
		if (this.x < 30) {
			this.x++;
		} else if (this.y < 30) {
			this.y++;
		} else {
			this.x = 0;
			this.y = 0;
		}
		layer.skew = {x: this.x, y: this.y};
	}
});
```
	
Replace your code for `mainContainer` from Example 1 with the following to add two more layers with your newly defined behaviors.

```
var mainContainer = new Container({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin, style: smallText,
	contents: [
		new LayerExample({ top: 100, left: 200, fillColor: "#CC7E1A", 
		   string: "opacity", behavior: opacityLayerBehavior }),
		new LayerExample({ top: 100, left: 20, fillColor: "#FF6F3A", 
		   string: "rotation", behavior: rotationLayerBehavior }),
		new LayerExample({ top: 20, left: 75, fillColor: "#79FFBF", 
		   string: "skew", behavior: skewLayerBehavior })
	]
});
application.add(mainContainer);
```

Figure 2 shows what you will see if you run this application on the Kinoma Create simulator.

**F1gure 2.** Running the Rotation and Skew Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/1-hRYKKAnng?rel=0&amp;vq=hd1080" frameborder="0" allowfullscreen><a href="https://www.youtube.com/embed/1-hRYKKAnng?rel=0&amp;vq=hd1080">Watch Video</a></iframe>
