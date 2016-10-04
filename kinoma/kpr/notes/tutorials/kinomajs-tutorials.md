<!-- Version: 160629-KO / Primary authors: Lizzie Prader and Andy Carle / Last reviewed: May 2016

This collection of tutorials is designed to teach you about KinomaJS, our framework for developing JavaScript applications that run on Kinoma Create, iOS, Android, and desktops. If you are just getting started with KinomaJS, we suggest completing at least the four basic tutorials.
--> 

#KinomaJS Tutorials

This collection of tutorials is designed to teach you about KinomaJS, our framework for developing JavaScript applications that run on Kinoma Create, iOS, Android, and desktops. If you are just getting started with KinomaJS, we suggest completing the four basic tutorials before moving on to those that address more advanced topics. 

These tutorials are intended as a complement to the primary [KinomaJS documentation](../../../../xs6/xsedit/features/documentation/docs/index.md). For a conceptual overview of KinomaJS that will put these tutorials in context, check out the [*KinomaJS Overview*](../../../../xs6/xsedit/features/documentation/docs/overview/overview.md) document; for a thorough reference on KinomaJS, see the [*KinomaJS JavaScript Reference*](../../../../xs6/xsedit/features/documentation/docs/javascript/javascript.md) document. Essentially all features of KinomaJS are also demonstrated in our large collection of [KinomaJS samples](http://kinoma.com/develop/samples/).

>**Note:** These tutorials do not apply to scripting for Kinoma Element, which does not use KinomaJS.

##Basic Tutorials

* [The KinomaJS Containment Hierarchy](containment-hierarchy/) -- Teaches the basics of creating and styling user interface elements in KinomaJS.

* [Application Logic in Behaviors](behaviors/) -- Describes how JavaScript application logic is bound to objects within a KinomaJS UI and how events propagate through the containment hierarchy.

* [Interacting with Web Services Asynchronously](flow/) -- Walks through examples of how to work with web services to retrieve data asynchronously and then use that data to update the UI.

* [Using Modules for Screens and Transitions](multiple-screens-modules-transitions/) -- Shows how KinomaJS applications can be broken down into a collection of modules and how those modules are included in the main application; uses an example of breaking a multi-screen app into individual modules and shows how to include transitions between screens.

##Advanced UI Options

* [Transforming Content at Runtime](layers/layers/md) -- Describes how to use the `layer` object to dynamically control the appearance of content. Features include opacity, rotation, and scaling.

* [Scrolling Content and Programmatically Adjusting Layout](advanced-layouts/advanced-layouts.md) -- Teaches how to make large pieces of content scroll on the screen and how to build your own layout manager in JavaScript.

* [Displaying Images and Media](images-media/images-media.md) -- Shows how to incorporate images, sound, and video into a KinomaJS application. Loading of both local and remote media is covered.


##Working with Sensors and Audio

* [Building Your Own BLLs](building-a-bll/building-a-bll.md) -- The Pins module can interact with sensors through either built-in or custom BLLs; this tutorial defines and describes the rules for building your own custom BLLs.

* [Audio Input and Output](audio-io/audio-io.md) -- Shows how to send audio to speakers and capture audio from microphones using the Pins mechanism. This enables audio data to be treated like any other sensor data in KinomaJS.