<!--
|     Copyright (C) 2010-2016 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<img alt="" src="http://kinoma.com/develop/documentation/technotes/images/xs6-and-kinomajs.png" class="technoteIllus" >

# Using JavaScript 6th Edition Features in KinomaJS
**Patrick Soquet, Kinoma Software Architect**  
October 15, 2015

JavaScript 6th Edition, often referred to by its technical abbreviation ES6, edition brings many new features to the JavaScript language. We've updated KinomaJS to use some of these new features, including Classes, Promises, and Generators. This Tech Note describes how you can use these news capabilities in KinomaJS.

KinomaJS is just beginning to take advantage of the many features in JavaScript 6th Edition. We welcome your ideas on how we can make KinomaJS even better through the use of new JavaScript 6th Edition features.

## Templates love arrows
Templates are a powerful way to define the constructors you use to build your containment hierarchies. Templates are introduced in the "[Contemplate This](http://kinoma.com/develop/documentation/technotes/introducing-kinomajs-dictionary-based-constructors-and-templates.php)" Tech Note. [Arrow functions](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/Arrow_functions) are a feature of JavaScript 6th Edition that eliminates the `function` keyword when creating anonymous callback functions, simplifying your source code.

	let MyLabel = Label.template($ => ({
	  left:0, right:0, height:40, string:$ 
	}));

You can also use arrow functions to map data into contents.
	
	let MyScroller = Scroller.template($ => ({
	  left:0, right:0, top:0, bottom:0, 
	  contents: [
		Column($, {
		  left:0, right: 0, top:0,
		  contents: $.map($$ => MyLabel($$, {})) 
		})
	  ]
	}));
	application.add(new MyScroller(["one","two","three","four"]));

## Behaviors with class
Behaviors are ordinary objects with functions that correspond to events triggered by contents or handlers. Typically frameworks like KinomaJS use inheritance to define increasingly specialized behaviors.

You have always been able to define KinomaJS behaviors using `Object.create` and property descriptors. However, that syntax is so intricate that KinomaJS introduced dictionary-based constructors as described in the "[Good Behavior](http://kinoma.com/develop/documentation/technotes/using-dictionary-based-constructors-for-kinomajs-behaviors.php)" Tech Note. Dictionary-based constructors are simpler, but calling inherited functions is still cumbersome.

With JavaScript 6th Edition, you can now use [classes](http://www.2ality.com/2015/02/es6-classes-final.html) to define behaviors.

	class ButtonBehavior extends Behavior {
	  onCreate(content, data) {
		this.data = data;
	  }
	  onTouchBegan(content) {
		content.state = 1;
	  }
	  onTouchEnded(content) {
		content.state = 0;
	  }
	};

Behaviors can then use `super` to call inherited functions.

	class URLButtonBehavior extends ButtonBehavior {
	  onTouchEnded(content) {
		super.onTouchEnded(content);
		if (this.data.url)
			content.invoke(new Message(this.data.url));
	  }
	};

Unlike dictionary-based constructors, classes only define functions. Dictionary-based constructors can also define property values, including numbers, booleans, and strings. If you need to define other properties, do so by overriding the constructor of the class.

	class MyBehavior extends Behavior {
		constructor(...args) {
			super(...args);
			this.wow = "wow";
		}
	};

	
### Containment hierarchy

Templates now support classes to bind behaviors to contents.

	let URLButton = Content.template($ => ({
		Behavior: URLButtonBehavior
	}));

When the template is instantiated, KinomaJS calls the class constructor to create the behavior.

> Notice the uppercase `B`. Use `Behavior` to bind a content to a class, use `behavior` to bind a content to an object. 

It is often useful to specialize a behavior inline, as part of the template itself. To do that, use an anonymous class.

	let BackButton = Content.template($ => ({
		Behavior: class extends ButtonBehavior {
			onTouchEnded(content) {
				super.onTouchEnded(content);
				content.invoke(new Message("/back"));
			}
		}
	}));
 
### Handlers

The `Handler.Bind` function now accepts a class to bind a path to a behavior.

	Handler.Bind("/wow", class extends Behavior {
		onInvoke: function(handler, message) {
			debugger;
		}
	});
	
The `Handler.Bind` function calls the class constructor to create the behavior.

> Notice the uppercase `B`. Use `Handler.Bind` to bind a path to a class, use `Handler.bind` to bind a path to an object. 

## Transitions too
KinomaJS does not implement dictionary-based constructors to define transitions. You have had to use `Object.create` and property descriptors when creating transitions.

KinomaJS now supports the use of classes to define transitions.

	class FadeInTransition extends Transition {
		onBegin(container, content) {
			container.add(content);
			this.layer = new Layer();
			this.layer.attach(content);
		}
		onEnd(container, content) {
			this.layer.detach();
		}
		onStep(fraction) {
			this.layer.opacity = Math.quadEaseOut(fraction);
		}
	}

## Contents iterator

[Generator functions](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Iterators_and_Generators) are a new feature of JavaScript 6th Edition. A common use of generator functions is to implement iterators. A common iterator in KinomaJS is to loop over a container's contents. You can define a contents iterator with a generator function.

	function* contentsIterator(container) {
		let content = container.first;
		while (content) {
			yield content;
			content = content.next;
		}
	}
	let iterator = contentsIterator(container);
	for (let content of iterator)
		trace(content.constructor.name + "\n");

But you don't need to include this generator function in your code, because KinomaJS now has a built-in content iterator that works with a simple `for of` loop. The contents iterator is now a property of `Container.prototype`. The symbol of the content iterator property is the [well-known symbol](http://ponyfoo.com/articles/es6-symbols-in-depth) referenced by `Symbol.iterator`.

	for (let content of container)
		trace(content.constructor.name + "\n");

## Promises from Message

Applications written in KinomaJS use messages to communicate with each other, with the shell, and with Internet services. Messages are always asynchronous. To date only contents and handlers could invoke messages, receiving the message result in the `onComplete` event of their behavior.

	let GetURL = Content.template($ => ({
		Behavior: class extents Behavior {
			onCreate(content, data) {
				content.invoke(new Message(data), Message.TEXT);
			}
			onComplete(content, message, text) {
				trace(text);
			}
		}
	}));
	let getURL = new GetURL("http://www.kinoma.com");

KinomaJS has extended messages, so that they can be invoked directly, returning a JavaScript 6th Edition [Promise](http://www.datchley.name/es6-promises/) to receive the message result. `Message.prototype.invoke` returns a promise.

	let message = new Message("http://www.kinoma.com");
	let promise = message.invoke(Message.TEXT);
	promise.then(text => { trace(text) });
	
The argument of `Message.prototype.invoke` is the type of result you want. If `undefined`, the promise will resolve with the message itself.
 
	let message = new Message("http://www.kinoma.com");
	message.invoke().then(message =>
			{ trace(message.responseText) });

You can chain messages with `then`. For example, here the first message retrieves the latitude and longitude, and then the second message retrieves the weather forecast.

	let message = new Message("http://k3.cloud.kinoma.com/api?extAction=GeoIP&extMethod=getRecord")
	message.invoke(Message.JSON).then(json => {
		let record = json.result.record;
		let message = new YahooWeatherMessage(record.latitude, record.longitude);
		return message.invoke(Message.JSON);
	}).then(json => {
		let day = json.query.results.location.forecast.day[0];
		trace(day.temp.high + " " + day.temp.low + "\n");
	});
	
<!-- -->
