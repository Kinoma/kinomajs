#Navigating the KinomaJS Application Framework Source Code and Documentation

<p align="center"> <img src="http://kinoma.com/media/press-images/img/KinomaJS-sm.jpg" height="200" alt=""/></p>

> Note: In this document, 'KinomaJS' refers to the JavaScript application framework for embedded devices created by Kinoma, not the entire Kinoma software stack.

The source code for the KinomaJS application framework is contained in [kinomajs/kinoma/kpr](../kinoma/kpr). This document is an overview of what's there, complete with links to source code and documentation.

***

##Extensions

The [extensions](../kinoma/kpr/extensions) folder contains the source code for KinomaJS features related to network connectivity, including Wi-Fi configuration and a variety of network protocols.

###Network Protocols

KinomaJS has its own implementations of the following network protocols. Links in the **Protocol** column lead to the source code. The samples are part of our collection of [KinomaJS sample apps](https://github.com/Kinoma/KPR-examples).

| Protocol | Documentation | Samples |
| :-- 	| :-- | :-- |
| [CoAP](../kinoma/kpr/extensions/coap) | [Creating a CoAP Client and Server in KinomaJS applications](../kinoma/kpr/notes/tech-notes/coap-client-and-server-in-kinomajs/coap-client-and-server-in-kinomajs.md): Provides an overview of CoAP and explains how to use it in KinomaJS applications | [coap-client](https://github.com/Kinoma/KPR-examples/tree/master/coap-client)<BR>[coap-server](https://github.com/Kinoma/KPR-examples/tree/master/coap-server)
| [HTTP](../kinoma/kpr/sources) | [Embedding an HTTP Server in your Application](../kinoma/kpr/notes/tech-notes/embedding-an-http-server-in-your-application/embedding-an-http-server-in-your-application.md): Describes how to start, stop, and customize your app's HTTP server | [basic-web-service-request](https://github.com/Kinoma/KPR-examples/tree/master/basic-web-service-request)
| [MQTT](../kinoma/kpr/extensions/mqtt) |  | 
| [SSDP](../kinoma/kpr/extensions/ssdp)  | [Net Scanner](../kinoma/kpr/notes/tech-notes/net-scanner/net-scanner.md): describes SSDP and how it's used on Kinoma Create | [ssdp](https://github.com/Kinoma/KPR-examples/tree/master/ssdp) 
| [WebSockets](../kinoma/kpr/extensions/websocket) |  | [websocket-client](https://github.com/Kinoma/KPR-examples/tree/master/websocket-client)<BR>[websocket-server](https://github.com/Kinoma/KPR-examples/tree/master/websocket-server)
| [Zeroconf](../kinoma/kpr/extensions/zeroconf) |  [Zeroconf troubleshooting guide](../kinoma/kpr/notes/tech-notes/zero-conf-note/zero-conf-troubleshooting.md): For developers who encounter problems when using Pins Sharing or other Zeroconf features | [zeroconf](https://github.com/Kinoma/KPR-examples/tree/master/zeroconf)

***

##Libraries

The [libraries](../kinoma/kpr/libraries) folder contains several JavaScript libraries. Some are built-in to the platform, others must be added to the build path. The table below provides a description of each and links to the corresponding source code, documentation, and (where applicable) to samples that showcase them from our collection of [KinomaJS sample apps](https://github.com/Kinoma/KPR-examples).

| Library | Description |Documentation | Samples |
| :-- 	| :-- | :-- | :-- | 
| [Controls](../kinoma/kpr/libraries/Controls) 		| Templates for basic on-screen buttons, radio buttons, checkboxes, sliders, switches, and input fields | [Adding User Interface Controls](../kinoma/kpr/notes/tutorials/controls/controls.md) : Describes how to use a selection of sample modules and assets to create onscreen input controls. | [basic-checkbox] (https://github.com/Kinoma/KPR-examples/tree/master/basic-checkbox)<BR>[controls-buttons](https://github.com/Kinoma/KPR-examples/tree/master/controls-buttons)
| [Creations](../kinoma/kpr/libraries/Creations) 		| Templates for frequently used UI elements, such as dialogs, headers, and graphs | - | [level-meter](https://github.com/Kinoma/KPR-examples/tree/master/level-meter)<BR><BR>This module is mainly used by the [built-in apps](https://github.com/Kinoma/kinomajs/tree/master/kinoma/kpr/projects/create/apps) on the home screen of Kinoma Create, rather than those in the samples repository
| [HID](../kinoma/kpr/libraries/HID) 				| Allows you to make Linux platforms (e.g. Kinoma Create) act as an HID keyboard, a mouse, and/or gamepad | [Prototyping Peripherals with Kinoma Create](../kinoma/kpr/notes/tech-notes/prototyping-peripherals/prototyping-peripherals.md): Explains how to build peripherals using your Kinoma Create Version 2 with the KinomaJS HID library | [hid-gamepad](https://github.com/Kinoma/KPR-examples/tree/master/hid-gamepad)<BR>[hid-keyboard](https://github.com/Kinoma/KPR-examples/tree/master/hid-keyboard)<BR>[hid-mouse](https://github.com/Kinoma/KPR-examples/tree/master/hid-mouse)
| [LowPAN](../kinoma/kpr/libraries/LowPAN) 			| BLLs for the KinomaJS BLE implementation; accessed through the Pins module. | [KinomaJS BLE V2 API](../kinoma/kpr/libraries/LowPAN/doc/kinomajs-ble-api.md) : Describes the class-based BLE V2 API, which supports both BLE central and peripheral roles. | [ble-ancs](https://github.com/Kinoma/KPR-examples/tree/master/ble-ancs/)<BR>[ble-clapper](https://github.com/Kinoma/KPR-examples/tree/master/ble-clapper)<BR>[ble-keyboard](https://github.com/Kinoma/KPR-examples/tree/master/ble-keyboard)<BR>+many more<BR><BR>Walkthrough tutorials available in our our [hardware tutorials](https://github.com/Kinoma/KPR-examples/tree/master/tutorials)
| [MobileFramework](../kinoma/kpr/libraries/MobileFramework) | A collection of templates for a variety of features useful for developing mobile apps, including scrollers, screen transitions, and headers/footers| - | [basic-scroller](https://github.com/Kinoma/KPR-examples/tree/master/basic-scroller)<BR>[basic-dialog](https://github.com/Kinoma/KPR-examples/tree/master/basic-dialog)<BR>[menu-button](https://github.com/Kinoma/KPR-examples/tree/master/menu-button)<BR>+many more
| [MultiTouchLib](../kinoma/kpr/libraries/MultiTouchLib) 	| Simplifies programming controls for devices with touch screens; includes multi-touch handling, swiping gestures, and more | - | [multitouch-picture](https://github.com/Kinoma/KPR-examples/tree/master/multitouch-picture)<BR>[multitouch-slide-browser](https://github.com/Kinoma/KPR-examples/tree/master/multitouch-slide-browser)
| [Pins](../kinoma/kpr/libraries/Pins/src) 			| The API to communicate with the BLLs of sensors/external hardware attached to the pin connectors on Kinoma Create | See the Programming with Hardware links in the Kinoma Create section of the root README of [this repository](https://github.com/Kinoma/kinomajs/) | [digital-in-hello-world](https://github.com/Kinoma/KPR-examples/tree/master/digital-in-hello-world)<BR>[analog-starter](https://github.com/Kinoma/KPR-examples/tree/master/analog-starter)<BR>[pwm-tricolor-led](https://github.com/Kinoma/KPR-examples/tree/master/pwm-tri-color-led)<BR>+many more<BR><BR>Walkthrough tutorials for a collection of common sensors are available in our our [hardware tutorials](https://github.com/Kinoma/KPR-examples/tree/master/tutorials)
| [Transitions](../kinoma/kpr/libraries/Transitions) 	| A collection of basic Transitions that can be used to switch between screens of an app | - | [transitions](https://github.com/Kinoma/KPR-examples/tree/master/transitions)<BR>[serial-7segment-display](https://github.com/Kinoma/KPR-examples/tree/master/serial-7segment-display)

***

##Projects

The [projects](../kinoma/kpr/projects) folder contains tools built using the KinomaJS platform, including the Kinoma Create and Kinoma Element launchers.

###Kinoma Create

The [Kinoma Create subfolder](../kinoma/kpr/projects/create) includes:

- The source code of the [built-in apps](../kinoma/kpr/projects/create/apps) seen on the home screen of Kinoma Create.
- The source code of the [Pins simulators](../kinoma/kpr/projects/create/shell/simulator) used when apps using built-in BLLs are run on the Kinoma Create simulator.
- Source code for the [shells](../kinoma/kpr/projects/create/shell) that host the Kinoma Create software in simulation and on the device.

###Kinoma Element

The [Kinoma Element subfolder](../kinoma/kpr/projects/element) includes:

- The source code of the [Pins simulators](../kinoma/kpr/projects/element/simulator) used when apps using built-in BLLs are run on the Kinoma Element simulator.
- The source code for [shell](../kinoma/kpr/projects/element/shell) that hosts the Kinoma Element simulator.


***

##Sources

The [sources](../kinoma/kpr/sources) folder contains the C code that implements the KinomaJS application framework; in other words, it contains the native implementation of the JavaScript objects that KinomaJS applications and shells interact with. 

***

##Notes

The [tech-notes](../kinoma/kpr/notes/tech-notes) folder contains additional documentation on miscellaneous topics that may be of interest to developers:

- [Realtime Cross-Device Communication with PubNub](../kinoma/kpr/notes/tech-notes/pubnub/pubnub.md)
- [Garbage Collection in KinomaJS](../kinoma/kpr/notes/tech-notes/garbage-collection-in-kinomajs/garbage-collection-in-kinomajs.md)
- [Using JavaScript 6th Edition Features in KinomaJS](../kinoma/kpr/notes/tech-notes/using-javascript-6th-edition-features-in-kinomajs/using-javascript-6th-edition-features-in-kinomajs.md)
- [The Mystery of THEME](../kinoma/kpr/notes/tech-notes/mystery-of-theme/mystery-of-theme.md)
- [Specifying Color in KinomaJS](../kinoma/kpr/notes/tech-notes/specifying-color-in-kinomajs/specifying-color-in-kinomajs.md)
- [Introducing KinomaJS Dictionary-based Constructors and Templates](../kinoma/kpr/notes/tech-notes/introducing-kinomajs-dictionary-based-constructors-and-templates/introducing-kinomajs-dictionary-based-constructors-and-templates.md)
- [Using Dictionary-based Constructors for KinomaJS Behaviors](../kinoma/kpr/notes/tech-notes/using-dictionary-based-constructors-for-kinomajs-behaviors/using-dictionary-based-constructors-for-kinomajs-behaviors.md)
- [Using Fractional Coordinates for Animation in KinomaJS](../kinoma/kpr/notes/tech-notes/using-fractional-coordinates-for-animation-in-kinomajs/using-fractional-coordinates-for-animation-in-kinomajs.md)
- [Playing Audio with KinomaJS](../kinoma/kpr/notes/tech-notes/playing-audio-kinomajs/playing-audio-kinomajs.md)
- [Net Scanner](../kinoma/kpr/notes/tech-notes/net-scanner/net-scanner.md)
- [Using DIAL to Launch Apps Remotely](../kinoma/kpr/notes/create-notes/using-dial-to-launch-apps-remotely/using-dial-to-launch-apps-remotely.md): introduction to the DIAL protocol, how to discover DIAL-compatible devices using Net Scanner, and how to add DIAL support to your KinomaJS app.

> Note: These resources are also available in the [Tech Notes](http://kinoma.com/develop/documentation/technotes/) section of the Kinoma website



