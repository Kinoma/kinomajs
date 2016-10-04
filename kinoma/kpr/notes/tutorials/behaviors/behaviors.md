# Application Logic in Behaviors

KinomaJS delivers events to the containment hierarchy of applications and shells, and the `content` and `container` objects contain functions in their `behavior` object to handle those events. KinomaJS uses events extensively. User actions like touching or typing, and system callbacks like activating or focusing, are dispatched into the containment hierarchy. From the application and shell point of view, the events are triggered by `content` and `container` objects. Programs and modules implement `behavior` objects to respond to such events.

This tutorial provides sample code that uses `behavior` objects. It assumes familiarity with building grid-based interfaces and the containment hierarchy in KinomaJS. If you are not familiar with these topics, please refer to the tutorial [The KinomaJS Containment Hierarchy](../containment-hierarchy/containment-hierarchy.md) and the Tech Note [Using Dictionary-Based Constructors for KinomaJS Behaviors](../../tech-notes/using-dictionary-based-constructors-for-kinomajs-behaviors/using-dictionary-based-constructors-for-kinomajs-behaviors.md).

##Behaviors and Events

The `behavior` object contains functions corresponding to events triggered by a `content` or `handler` object. A `content` or `handler` object checks whether its behavior (which can be any object) owns or inherits a function property with the name of the event, and if so calls that function, passing itself as the first argument.

The following sections focus on functions related to the behavior of `content` objects.

###Built-In Behavior Functions

KinomaJS defines low-level events, such as `onTouchBegan` and `onTouchEnded`, that are useful on a wide range of target devices with touch screens, including Kinoma Create and Android/iOS smartphones. 

Applications and shells can define their own low-level events to support the unique features of a particular kind of device. Events defined by applications and shells are implemented in the same way as events defined by KinomaJS.

The following examples use a subset of the built-in behavior functions. See the [*KinomaJS JavaScript Reference*](../../../../../xs6/xsedit/features/documentation/docs/javascript/javascript.md) document for the full lists of events that are triggered by `content` and `handler` objects; they can be found in the "Events" subsection for each object.

####Example 1: Touch

This example makes use of two built-in behavior functions for the `application` object: `onLaunch` and `onTouchEnded`. Open a new application project and put the following code in your `main.js` file.

```
var redSquare = new Content({ 
	left: 0, height: 10, top: 0, width: 10, skin: new Skin({ fill: "red" }), 
});
		
application.behavior = Behavior({
	onLaunch: function(application) {
		application.skin = whiteSkin;
		application.active = true;
		application.add(redSquare);
	},
	onTouchEnded: function(content, id, x, y, ticks) {
		application.first.position = {x, y}  // ES6 shorthand; the equivalent of {x:x, y:y}
	}
})
```
	
The `onLaunch` event is triggered when the shell calls the `launch` function of the `host` object that hosts the application. You can read more about `host` objects in [*KinomaJS JavaScript Reference*](../../../../../xs6/xsedit/features/documentation/docs/overview/overview.md), but all you really need to know to understand this code is that `onLaunch` is called when you run the program.

The `onTouchEnded` event is automatically called for any `content` object whose `active` property is set to `true`. The arguments `x` and `y` are numbers that represent the global coordinate of the event; in this example they are used to determine where the red square goes.

The video in Figure 1 shows what running this application on the Kinoma Create simulator will look like.

**Figure 1.** Running the Touch Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/ccM-PTNwyBk?rel=0&amp;vq=hd720" frameborder="0" allowfullscreen></iframe>

<br>For more examples of using built-in touch functions, see the [`canvas`](https://github.com/Kinoma/KPR-examples/tree/master/canvas) and [`multitouch-picture`](https://github.com/Kinoma/KPR-examples/tree/master/multitouch-picture) sample apps.

####Example 2: Duration

This example makes use of five built-in behavior functions for `container` objects: `onCreate`, `onDisplayed`, `onTimeChanged`, `onFinished`,  and `onTouchEnded`. It demonstrates how to use the clock built into every `content` object. Keep in mind, however, that a content's clock should be used only for animation, not for other periodic events. When a clock is running, the screen is typically updated many times a second.

Open a new application project and put the following code in your `main.js` file.

```
var stringStyle = new Style( { font: "bold 30px", color: "black" } );
var graySquare = new Container({ 
	left: 115, height: 90, top: 65, width: 90, 
	skin: new Skin({ fill: "#d0d0d0" }), 
	contents: [
		new Label({ left: 0, right: 0, top: 0, bottom: 0, style: stringStyle, 
		   string: "3" })
	],
	behavior: Behavior({
		onCreate: function(container, data) {
			//trace("onCreate called\n");
			this.count = this.startCount = Number(container.first.string);
			container.interval = 1000;
			container.duration = this.startCount*1000;
			container.active = false;
		},
		onDisplayed: function(container, data) {
			//trace("onDisplayed called\n");
			container.start();
		},
		onTimeChanged: function(container) {
			this.count = this.count - 1;
			container.first.string = (this.count > 0) ? this.count : "Done!"
		},
		onFinished: function(container) {
			container.active = true;
		},
		onTouchEnded: function(container) {
			container.active = false;
			this.count = this.startCount+1;  // +1 to cancel out the -1 in onTimeChanged
			container.time = 0;
			container.start();
			
		}
	})
});
		
application.behavior = Behavior({
	onLaunch: function(application) {
		application.skin = whiteSkin;
		application.active = true;
	},
	onTouchEnded: function(application) {
		application.active = false;
		application.add(graySquare);  // Will trigger onDisplayed in redSquare's behavior
	}
})
```

Here is when the events are triggered:

* `onCreate` is triggered when the behavior is constructed.

* `onDisplayed` is triggered when the specified `content` object becomes visible. In this example it is called when you touch the screen for the first time and `graySquare` is added to the application.

* `onTimeChanged` is triggered when the time of the specified `content` object changes. The time changes at the interval specified; in this example, the interval is set to 1000 ms in the `onCreate` function.

* `onFinished` is triggered when the specified `content` object is running and its time equals its duration. You can see that the duration is set on the `onCreate` function in this example.

The video in Figure 2 shows what running this application on the Kinoma Create simulator will look like.

**Figure 2.** Running the Duration Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/C3cLbxeVwSY?rel=0&amp;vq=hd720" frameborder="0" allowfullscreen></iframe>

<br>
For more examples of using the clock built into `content` objects, see the [`timers`](https://github.com/Kinoma/KPR-examples/tree/master/timers) and [`kangaroo-disco`](https://github.com/Kinoma/KPR-examples/tree/master/kangaroo-disco) sample apps.

###Custom Behavior Functions

In addition to using the built-in behavior functions, you can also create your own and trigger events that will call them. This is most commonly done using the `delegate`, `bubble`, and `distribute` functions. The next section covers these functions in more detail.


##Delegate, Bubble, Distribute

When an event is triggered, the application or an application framework usually traverses some portion of the containment hierarchy to identify the contents or containers to receive the event. To help efficiently implement common event propagation patterns, contents have `delegate` and `bubble` functions, and containers additionally have a `distribute` function.

> **Note:** Much of the information in this section comes directly from the discussion of delegation in the section on behaviors and events in the [*KinomaJS Overview*](../../../../../xs6/xsedit/features/documentation/docs/overview/overview.md) document.

Here we will use a simple example to demonstrate the difference between `delegate`, `bubble`, and `distribute`. Start a new application project and add the following code to your `main.js` file. It creates a simple white skin, with light-gray borders, that will be used later.

```
var whiteSkin = new Skin({ 
	fill: "white",
	borders: {left: 2, right: 2, top: 2, bottom: 2}, 
	stroke: "#909090"	
});
```

###Delegate

The `delegate` function takes the name of a function and its parameters and calls the corresponding function of the content's behavior with the content and those parameters.

Add to your `main.js` file the following template for a container, with two behavior functions, that is slightly smaller than its parent container.

```
var smallerContainer = Container.template($ => ({
	top: 20, bottom: 20, right: 20, left: 20, skin: whiteSkin,
	behavior: Behavior({
		onBackgroundChange: function(container, data) {
			container.skin = data.newSkin;
		},
		onTouchEnded: function(container, data) {
			var num = Math.floor(Math.random() * Math.pow(2, 24));
			var hexVal = "#" + ('00000' + num.toString(16)).substr(-6);
			let newSkin = new Skin({
				fill: hexVal, 
				borders: {left: 2, right: 2, top: 2, bottom: 2}, 
				stroke: "#909090"
			})
			container.delegate("onBackgroundChange", {newSkin});  // ES6 shorthand
		}
	})
}));
```
	
Note that this template has an `onTouchEnded` function in its behavior. `onTouchEnded` will be called only on containers whose `active` property is set to `true`. This is not done upon instantiation; it will be done later as necessary.
 
Now add some containers to the application when it launches. The following code creates three containers nested within each other. The `active` property of the medium-sized one is set to `true`. They are all added to the application, so there are four levels of containment hierarchy involved: the application container contains `biggestContainer`, which contains `mediumContainer`, which contains `smallestContainer`.
 
```
application.behavior = Behavior({
	onLaunch: function(application) {
		application.skin = whiteSkin;
		var biggestContainer = new smallerContainer();
		var mediumContainer = new smallerContainer();
		var smallestContainer = new smallerContainer();
		
		mediumContainer.active = true;
		
		mediumContainer.add(smallestContainer);
		biggestContainer.add(mediumContainer);
		application.add(biggestContainer);
	}
})
```

Because `mediumContainer` is the only active container, it is the only one that will call `onTouchEnded` and, in turn, `onBackgroundChange`. The video in Figure 3 shows what running this application and clicking `mediumContainer` will look like.

**Figure 3.** Running the Delegate Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/Oi9jebCvx2U?rel=0&amp;vq=hd720" frameborder="0" allowfullscreen><a href="https://www.youtube.com/embed/Oi9jebCvx2U?rel=0&amp;vq=hd720">Watch Video</a></iframe>

###Distribute

The `distribute` function works like the `delegate` function but also calls all the behaviors downward in the containment hierarchy. The order of traversal is depth first.

To test the `distribute` function, you only have to make one change to the code from before. Change the following line in `onTouchEnded`

```
container.delegate("onBackgroundChange", {newSkin});
```

to

```
container.distribute("onBackgroundChange", {newSkin});
```
	
Because `smallContainer` is inside `mediumContainer`, its `onBackgroundChange` function will be called even though its `active` property is not set to `true`. Its `onTouchEnded` function is not called. 

Because the application container and `bigContainer` are higher in the containment hierarchy, their behaviors are *not* searched for an `onBackgroundChange` function.

The video in Figure 4 shows what running the application and clicking `mediumContainer` will now look like.

**Figure 4.** Running the Distribute Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/zkfATafAPqc?rel=0&amp;vq=hd720" frameborder="0" allowfullscreen><a href="https://www.youtube.com/embed/zkfATafAPqc?rel=0&amp;vq=hd720">Watch Video</a></iframe>

###Bubble

The `bubble` function works like the `delegate` function but also calls all the behaviors upward in the containment hierarchy, from the content to the application.

Again, you have to change only one line. Change the following line in `onTouchEnded`

```
container.distribute("onBackgroundChange", {newSkin});
```

to

```
container.bubble("onBackgroundChange", {newSkin});
```

Because `bigContainer` is higher in the containment hierarchy, its `onBackgroundChange` function will be called even though its `active` property is not set to `true`. Its `onTouchEnded` function is not called. If the application container had an `onBackgroundChange` function in its behavior, it would also be called.
 
Because `smallContainer` is inside `mediumContainer`, its behavior is *not* searched for an `onBackgroundChange` function.

The video in Figure 5 shows what running the application and clicking `mediumContainer` will now look like.

**Figure 5.** Running the Bubble Example

<iframe width="100%" height="500" src="https://www.youtube.com/embed/Flg2mRQvFK0?rel=0&amp;vq=hd720" frameborder="0" allowfullscreen><a href="https://www.youtube.com/embed/Flg2mRQvFK0?rel=0&amp;vq=hd720">Watch Video</a></iframe>

##Where to Go Next

Now that you know how to embed application logic within your user interface elements and how to trigger events, the next step is to learn how to manage other types of asynchronous application flow, as discussed in the tutorial [Interacting with Web Services Asynchronously](../flow/flow.md). This includes interacting with web services in the background to dynamically populate your UI.

To see some more complicated uses of behaviors to modify the UI, check out the tutorial [Scrolling Content and Programmatically Adjusting Layout](../advanced-layouts/advanced-layouts.md).