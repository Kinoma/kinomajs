# XS6 and KinomaJS
The 6th edition brings a lot of new features to JavaScript. This tech note explains how you can use some of them in KinomaJS...

## Templates love Arrows
Templates are a powerful way to define the constructors that build your containment hierarchies, see the "Contemplate This" tech note. Arrow functions allow to get rid of the `function` keyword, which simplifies further the notation.

	let MyLabel = Label.template($ => ({
	  left:0, right:0, height:40, string:$ 
	}));

And of course you can use arrow functions to map data into contents.
	
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

## Behaviors with Class
Behaviors are ordinary objects with functions corresponding to events triggered by contents or handlers. Typically frameworks use inheritance to define increasingly specialized behaviors.

You can define behaviors with `Object.create` and property descriptors but the syntax is so intricate that KinomaJS introduced dictionary-based constructors, see the "Good Behavior" tech note. It helps but re-calling inherited functions is still cumbersome.

Now you can use classes to define behaviors.

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

Behaviors can then use `super` to re-call inherited functions.

	class URLButtonBehavior extends ButtonBehavior {
	  onTouchEnded(content) {
		super.onTouchEnded(content);
		if (this.data.url)
			content.invoke(new Message(this.data.url));
	  }
	};

Contrarily to dictionary-based constructors, classes define only functions. If you need to define other properties, override the class constructor.

	class MyBehavior extends Behavior {
		constructor(...args) {
			super(...args);
			this.wow = "wow";
		}
	};

	
### Containment Hierarchy

Templates now support classes to bind contents and behaviors.

	let URLButton = Content.template($ => ({
		Behavior: URLButtonBehavior
	}));

When the template is instantiated, KinomaJS calls the class constructor to create the behavior.

> Notice the uppercase `B`. Use `Behavior` to bind a content to a class, use `behavior` to bind a content to an object. 

It is often useful to specialize a behavior inline, in the template itself. To do that, use an anonymous class.

	let BackButton = Content.template($ => ({
		Behavior: class extends ButtonBehavior {
			onTouchEnded(content) {
				super.onTouchEnded(content);
				content.invoke(new Message("/back"));
			}
		}
	}));
 
### Handlers

Similarly the `Handler.Bind` function accepts a class to bind a path to a behavior.

	Handler.Bind("/wow", class extends Behavior {
		onInvoke: function(handler, message) {
			debugger
		}
	});
	
The `Handler.Bind` function calls the class constructor to create the behavior.

> Notice the uppercase `B`. Use `Handler.Bind` to bind a path to a class, use `Handler.bind` to bind a path to an object. 

## Transitions too
There are no dictionary-based constructors to define transitions so you needed to use `Object.create` and property descriptors. 

Now you can use classes to define transitions too.

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

Of course you could define a contents iterator with a generator function.

	function* iterator(container) {
		let content = container.first;
		while (content) {
			yield content;
			content = content.next;
		}
	}

To make it easier and faster, the contents iterator is now a property of `Container.prototype`. The symbol of the property is the well-known symbol referenced by `Symbol.iterator`. So you can use a `for of` loop for instance.

	for (let content of container)
		trace(content.constructor.name + "\n");

## Promises from Message

Applications use messages to communicate with each other, with the shell, and with Internet services. Messages are asynchronous. So far only contents and handlers could invoke messages and process results in the `onComplete` event of their behavior.

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

Thanks to XS6, you can now invoke messages directly. `Message.prototype.invoke` returns a promise.

	let message = new Message("http://www.kinoma.com");
	let promise = message.invoke(Message.TEXT);
	promise.then(text => { trace(text) });
	
The argument of `Message.prototype.invoke` is the type of result you want. If `undefined`, the promise will resolve with the message itself.
 
	let message = new Message("http://www.kinoma.com");
	message.invoke().then(message => { trace(message.responseText) });

You can chain messages with `then`. For instance here the first message gets the latitude and longitude, then the second messages gets the weather forecast.

	let message = new Message("http://k3.cloud.kinoma.com/api?extAction=GeoIP&extMethod=getRecord")
	message.invoke(Message.JSON).then(json => {
		let record = json.result.record;
		let message = new YahooWeatherMessage(record.latitude, record.longitude);
		return message.invoke(Message.JSON);
	}).then(json => {
		let day = json.query.results.location.forecast.day[0];
		trace(day.temp.high + " " + day.temp.low + "\n");
		debugger;
	});

## To Be Continued

No doubt XS6 can bring more features to KinomaJS. Do not hesitate to share ideas...




