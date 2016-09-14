<!-- Version: 160812-CR / Last reviewed: August 2016

JavaScript 6th Edition (often referred to as ES6) brings many new features to the JavaScript language. We have updated KinomaJS to use some of these new features, including classes, generators, and promises. This Tech Note describes how you can use these new capabilities in KinomaJS.
-->

<img alt="" src="img/xs6-and-kinomajs.png" class="technoteIllus" >

# Using JavaScript 6th Edition Features in KinomaJS

**Patrick Soquet, Kinoma Software Architect**  
October 15, 2015

JavaScript 6th Edition (often referred to as ES6, reflecting the newer name "ECMAScript") brings many new features to the JavaScript language. We have updated KinomaJS to use some of these new features, including classes, generators, and promises. This Tech Note describes how you can use these new capabilities in KinomaJS.

KinomaJS is just beginning to take advantage of the many features in JavaScript 6th Edition. We welcome your ideas on how we can make KinomaJS even better through the use of new JavaScript 6th Edition features.

## Creating Templates with Arrow Functions

Templates are a powerful way to define the constructors you use to build your containment hierarchies. As described in the Tech Note [Introducing KinomaJS Dictionary-Based Constructors and Templates](../introducing-kinomajs-dictionary-based-constructors-and-templates/), the `Content` and `Container` constructors provide a `template` function that takes one argument, an anonymous callback function. [Arrow functions](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/Arrow_functions) are a feature of JavaScript 6th Edition that eliminates the `function` keyword in creating anonymous callback functions, simplifying your source code. So instead of this

```
var MyLabel = Label.template(function($) { return {
	left:0, right:0, string:$ 
}});
```

you can do this:

```
let MyLabel = Label.template($ => ({
	left:0, right:0, height:40, string:$
}));
```

You can also use arrow functions to map data into contents.

```
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
```

## Defining Behaviors with Classes

Behaviors are ordinary objects with functions that correspond to events triggered by contents or handlers. Typically frameworks like KinomaJS use inheritance to define increasingly specialized behaviors. You have always been able to define KinomaJS behaviors using `Object.create` and property descriptors; however, that syntax is so intricate that KinomaJS introduced dictionary-based constructors, as described in the Tech Note [Using Dictionary-Based Constructors for Kinoma-JS Behaviors](../using-dictionary-based-constructors-for-kinomajs-behaviors/). Dictionary-based constructors are simpler, but calling inherited functions is still cumbersome.

With JavaScript 6th Edition, you can now use [classes](http://exploringjs.com/es6/ch_classes.html) to define behaviors.

```
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
```

Behaviors can then use `super` to call inherited functions.

```
class URLButtonBehavior extends ButtonBehavior {
	onTouchEnded(content) {
		super.onTouchEnded(content);
		if (this.data.url)
			content.invoke(new Message(this.data.url));
	}
};
```

Note that classes define only functions, whereas dictionary-based constructors can also define property values, including numbers, booleans, and strings. If you need to define other properties, do so by overriding the constructor of the class.

```
class MyBehavior extends Behavior {
	constructor(...args) {
		super(...args);
		this.wow = "wow";
	}
};
```

### Binding a Behavior to Content

Templates now support classes to bind behaviors to contents.

```
let URLButton = Content.template($ => ({
	Behavior: URLButtonBehavior
}));
```

When the template is instantiated, KinomaJS calls the class constructor to create the behavior.

> **Note:** Use `Behavior` (capitalized) to bind a content to a class, and `behavior` (lowercase) to bind a content to an object. 

It is often useful to specialize a behavior inline, as part of the template itself. To do that, use an anonymous class.

```
let BackButton = Content.template($ => ({
	Behavior: class extends ButtonBehavior {
		onTouchEnded(content) {
			super.onTouchEnded(content);
			content.invoke(new Message("/back"));
		}
	}
}));
```
 
### Binding a Path to a Behavior

The `Handler.Bind` function now accepts a class to bind a path to a behavior.

```
Handler.Bind("/wow", class extends Behavior {
	onInvoke: function(handler, message) {
		debugger;
	}
});
```
	
The `Handler.Bind` function calls the class constructor to create the behavior.

> **Note:** Use `Handler.Bind` (with `Bind` capitalized) to bind a path to a class, and `Handler.bind` (lowercase `bind`) to bind a path to an object. 

## Defining Transitions with Classes

KinomaJS does not implement dictionary-based constructors to define transitions; you have had to use `Object.create` and property descriptors when creating transitions. However, KinomaJS now supports the use of classes to define transitions.

```
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
```

## Generators and Iterators

[Generator functions](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Iterators_and_Generators) are a new feature of JavaScript 6th Edition. A common use of generators is to implement iterators, and a common iterator in KinomaJS is to loop over a container's contents. You can define a content iterator with a generator function.

```
function* contentIterator(container) {
	let content = container.first;
	while (content) {
		yield content;
		content = content.next;
	}
}
let iterator = contentIterator(container);
for (let content of iterator)
	trace(content.constructor.name + "\n");
```

But you do not need to include this generator function in your code, because KinomaJS now has a built-in content iterator that works with a simple `for...of` loop. The content iterator is now a property of `Container.prototype`. The symbol of the content iterator property is the well-known JavaScript 6th Edition [symbol](http://ponyfoo.com/articles/es6-symbols-in-depth) referenced by `Symbol.iterator`.

```
for (let content of container)
	trace(content.constructor.name + "\n");
```

## Promises from Messages

KinomaJS applications use messages to communicate with each other, with the shell, and with internet services. Messages are always asynchronous. To date, only contents and handlers have been able to invoke messages, receiving the message result in the `onComplete` event of their behavior.

```
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
```

KinomaJS has extended messages so that they can be invoked directly, returning a JavaScript 6th Edition [promise](http://www.datchley.name/es6-promises/) to receive the message result. `Message.prototype.invoke` returns a promise.

```
let message = new Message("http://www.kinoma.com");
let promise = message.invoke(Message.TEXT);
promise.then(text => { trace(text) });
```
	
The argument of `Message.prototype.invoke` is the type of result you want. If `undefined`, the promise will resolve with the message itself.

```
let message = new Message("http://www.kinoma.com");
message.invoke().then(message => { trace(message.responseText) });
```

You can chain messages with `then`. For example, the first message below retrieves the latitude and longitude, and the second message retrieves the weather forecast.

```
let message = new Message("http://k3.cloud.kinoma.com/api?extAction=GeoIP&extMethod=getRecord")
message.invoke(Message.JSON).then(json => {
	let record = json.result.record;
	let message = new YahooWeatherMessage(record.latitude, record.longitude);
	return message.invoke(Message.JSON);
}).then(json => {
	let day = json.query.results.location.forecast.day[0];
	trace(day.temp.high + " " + day.temp.low + "\n");
});
```
