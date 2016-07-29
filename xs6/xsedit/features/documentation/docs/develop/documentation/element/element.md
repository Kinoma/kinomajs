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
# Programmer's Guide to Kinoma Element
#### March 11, 2016 @ 9 PM

<!-- # System -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/element_intro.md]{/Users/hoddie/Desktop/Element docs - Markdown/element_intro.md} -->

## Introduction

Kinoma Element is hardware and software for building Internet of Things (IoT) projects, prototypes, and products. Built around a single chip that combines an ARM CPU, Wi-Fi, and memory, Kinoma Element delivers cost-effective Wi-Fi connectivity in a palm-size package. Unlike most IoT hardware, software for Kinoma Element is primarily JavaScript using the latest ES6 version of the language. JavaScript is a natural fit for a world filled with web services that communicate using JSON, and scripting speeds development and eases delivery of updates.

The chip at the heart of Kinoma Element is the Marvell MW302 System-on-a-Chip (SoC).

- 200 MHz ARM Cortex-M4
- Wi-Fi B/G/N
- 512 KB RAM
- 4 MB flash memory
- USB
- 16 programmable pins

The JavaScript engine powering Kinoma Element is XS6, the JavaScript virtual machine created by Kinoma and optmized for situations with limited memory and CPU power. XS6 implements ES6 standard (ECMAScript 2015), so the language is the same used in web browser and servers. The environment where the JavaScript runs is different - a tiny fraction of the memory, CPU power, and storage - so the JavaScript code developers create is different too.

As a foundation for IoT projects, prototypes, and products, Kinoma Element has built in support for many popular network procotols.

- HTTP client and server
- HTTPS (TLS versions 3.1, 3.2, and 3.3) client and server
- WebSocket client and server
- CoAP client and server
- MQTT client
- mDNS announcement and discovery
- SSDP announcement

Developers may implement other network procotols in JavaScript using the built-in Socket objects, which support both TCP and UDP.

IoT projects, prototypes, and products often augment Kinoma Element with additional components - sensors, lights, motors, and buttons. Kinoma Element has 16 reconfigurable pins to connect to these components.

- Power (3.3 V)
- Ground
- Digital input
- Digital output
- Analog input
- PWM
- Serial
- I2C
- SPI (via future software update)

The Pins module in Kinoma Element is the API for working with components attached to the hardware pins. The Pins module is a high level API which interacts with pins using URLs and callbacks, much like making requests to a web server.

The remainder of this document consists of two major sections. The first section is an introduction to programming on Kinoma Element. It contains essential information programmer's developing for Kinoma Element need to know. The second section is a reference for the Kinoma Element software modules and command line interface.

## Application project file

Applications for Kinoma Element developed in the Kinoma Code IDE use a JSON project file which is always named "project.json". The project file defines the application name, ID, and entry point.

	{
		"id": "example.kinoma.marvell.com",
		"title": "Example",
		"Element": {
			"main": "main"
		}
	}

The ID is a string which uniquely identifies the application. By convention the application ID uses a dotted domain name style. The application title is a string containing the application name to display to users.

The "Element" object must be present to run the application on Kinoma Element. It tells the IDE this application is compatible with Kinoma Element. The "main" entry inside the "Element" object specifies the name of the program file (without its file name extension) to run to launch the application. Most applications use "main.js" so the program file root is "main". If the program file name is "starthere.js" the "Element" object is:

	"Element": {
		"main": "starthere"
	}

## Application structure

A program for Kinoma Element is a JavaScript module that runs without using events or callbacks and so does not export an object with event handlers. An application for Kinoma Element is a JavaScript module that exports an object with event handlers, and so can use events and callbacks.

> Note: Most Kinoma Element projects are an application. For simple experiments, the program is useful.

The following main.js is a simple example of a program. It outputs system values to the console.

	import console from "console";

	console.log(`System.host: ${System.hostname}`);
	console.log(`System.device: ${System.device}`);
	console.log(`System.osVersion: ${System.osVersion}`);
	console.log(`System.timezone: ${System.timezone}`);

An application is an object with event handlers as shown in the following main.js. The application object is the default export of the module.

	export default {
		onLaunch() {
		},
		onQuit() {
		}
	};

The onLaunch event is called when the application begins executing. The onQuit event is called when it ends. The application is terminiated after onQuit returns, immediately canceling pending asynchronous operations. An application may omit the onLaunch and onQuit events if it does not use them.

	import HTTPClientRequest from "HTTPClient";

	let main = {
		onLaunch() {
			this.request = new HTTPClientRequest("http://www.kinoma.com/file.txt");
			this.request.onTransferComplete = success => console.log(String.fromArrayBuffer(this.request.content));
			this.request.start();
		}
	};
	export default main;

## Garbage collector

The JavaScript language uses a garbage collector for memory management. The garbage collector determines which objects are no longer in use and automatically frees their memory. The garbage collector in Kinoma Element follows the JavaScript specification. Programmers developing code for Kinoma Element need to be more aware of the garbage collector than those working on web browsers and web servers:

- Kinoma Element has considerably less memory, so the garage collector executes more frequently causing unused objects to be collected sooner.
- Kinoma Element has a relatively lightweight application framework which delegates management of object lifetime to the application.

Applications on Kinoma Element need to be aware of the garbage collector when invoking asynchronous operations. Examples of asynchronous operations include HTTP client requests and an HTTP server. In the following example, the HTTP client request may be garbage collected before the request completes because once onLaunch returns the local variables (e.g. request) go out of scope, leaving no JavaScript reference to the request.

	onLaunch() {
		let request = new HTTPClientRequest("http://www.kinoma.com/file.txt");
		request.onTransferComplete = success => console.log("done");
		request.start();
	}

To ensure that the request is not garbage collected before it completes, assign the request to another object that will not be garbage collected. Because the application object is not collected until the application terminates, it is a convenient object to attach the HTTP request.

	onLaunch() {
		let request = new HTTPClientRequest("http://www.kinoma.com/file.txt");
		this.request = request;
		request.onTransferComplete = success => console.log("done");
		request.start();
	}

## Single threaded runtime

Kinoma Element uses a single threaded runtime. This approach is consistent with the single threaded model of the JavaScript language, helps to conserve memory, and generally simplifies programming. Because the programming model is single threaded, all APIs are expected either to execute quickly (e.g. a few milliseconds) or to use a callback to indicate when an operation is complete.

	Socket.resolv("kinoma.com", result => console.log("Socket.resolv complete"));

Some objects in Kinoma Element use an event driven model to report notifications. 

	let tcp = new Socket({host: "www.kinoma.com", port: 80, proto: Socket.TCP});  
	tcp.onConnect = () => console.log("connected");
	tcp.onData = buffer => console.log(`received ${buffer.byteLength} bytes")

Application scripts should follow these same principles: execute quickly or use callbacks and events to break long operations into pieces.

Because Kinoma Element is single threaded certain operations block execution. Of particular note, the telnet and USB console connections are blocked when stopped at a breakpoint in the debugger. The console resumes normal operation when the debugger allows the application to continue running.

Kinoma Element has a Watchdog Timer to detect when an application blocks for an extended period. If it detects this situation, the watchdog assumes that the application is an infinite loop or has crashed and restarts the system.

## telnet

The Kinoma Element Command Line Interface (CLI) is available over telnet on port 2323. All diagnostic output is sent to the telnet connection.

To connect to telnet from the terminal in Mac OS X using the IP address of Kinoma Element:

	telnet 10.0.1.69 2323

Kinoma Element advertises the address of its telnet service using Zeroconf (mdns), allowing a connection to be established by name rather than by IP address. Before the hostname is set, the name of Kinoma Element is based on its MAC address (xxxxxx in the following example):

	telnet "Kinoma Element-xxxxxx".local 2323
	
After the hostname has been set, connections can be established using the hostname:

	telnet <Element hostname>.local 2323
	telnet K5_25.local 2323

> Commands described in the Command Line Interface Reference are available using telnet.

## USB

The Kinoma Element Command Line Interface (CLI) is available over USB as using a serial terminal application. All diagnostic output is sent to the USB console.

There are many different serial terminal applications available, and they vary by platform. Common serial terminal applications include cu, minicom, and screen.

> Commands described in the Command Line Interface Reference are available using USB.

## Diagnostic ouptut

Kinoma Element operates a console for diagnostic output. Scripts can use `console.log` and `trace` to send information to the diagnostic output. JavaScript exceptions are logged, in addition to any diagnostic output by other modules.

Output is sent to the telnet, the USB console connection, and the debugging console output.

## TFTP

Kinoma Element contains implements the Trivial File Transfer Protocol (TFTP) to upload files to and download files from Kinoma Element. 

> Note: Most developers do not need to use TFTP directly when working with the Kinoma Code IDE. TFTP is useful for retrieving log files and directly interacting with the Kinoma Element file sytem. Because TFTP is a low level tool, it is possible to damage the file system on Kinoma Element by overwriting key files so use TFTP with caution.

To communicate with Kinoma Element using TFTP, use a TFTP client. On Mac OS X, that is the tftp command line tool. Start the tftp tool by specifying the host and port.

	tftp -e 10.0.1.10 6969
	tftp -e <Element hostname>.local 6969

The -e option indicates that the data transfer is binary data, which is recommended for Kinoma Element.

The tftp service on Kinoma Element always operates on the "k3" partition. It cannot access other partitions.

To upload a file, use the put command. 

	tftp > put hello.xsb

To update the firmware directly:

	tftp > put xsr6_mc_k5.bin

To retrieve the console log file:

	tftp > get log

## Open source

The majority of software in Kinoma Element is available under the Apache open source license. The Kinoma team is working to make all the software in Kinoma Element open source.

The software is in the [KinomaJS respository](https://github.com/Kinoma/kinomajs/tree/master/xs6/sources/mc) on GitHub.

Instructions to build the open source code are in the [readme](https://github.com/Kinoma/kinomajs/blob/master/README.md) file in the section about Kinoma Element. 

## API stability

The API document for Kinoma Element describes those modules, objects, classes, functions, and proprties that the Kinoma team designed for development of applications and BLLs. These public APIs are not expected to change significantly. Should changes be made, they will be documented.

Kinoma Element contains other software which is not documented. This software is often used to implement the public APIs, or contains capabilties that have not yet been fully developed. Developers may learn about these APIs by inspecting JavaScript objects in the debugger or reviewing the Kinoma Element open source code. These undocumented APIs may change, or may even be removed, without notice. Developers are free to use them, of course, but they may not be supported by the Kinoma engineering team.

## JavaScript implementation

KInoma Element uses the XS6 JavaScript virtual machine. XS6 implements the ES6 specification, also known as ECMAScript 2015. XS6 has excellent compatibility with the JavaScript standard, allowing developers to use the same langauge they already know from web browsers and servers.

There are some details of the JavaScript implementation to be aware of. Resource contrained hardware motivates the differences. Kinoma Element has signficantly less memory and CPU performance than is common in web browsers and servers. These differences may be relevant for large applications and applications that import code modules from other environments.

#### String.fromArrayBuffer(buffer)

Kinoma Element applications work with sensor inputs and network protocols that provide data in ArrayBuffers, to accomodate binary data. That binary data may contain string data. The ES6 specification does not provide a function to directly convert binary data to a string. The operation can be implemented in JavaScript, but will be relatively inefficient on a resource constrained device like Kinoma Element. Because this operation is common in Kinoma Element applications, the String object is extended with the built-in function fromArrayBuffer.

	let text = String.fromArrayBuffer(buffer);

The buffer must contain a valid UTF-8 string.

To extract a string from part of an ArrayBuffer, extract the data into a new ArrayBuffer first.

	let substring = buffer.slice(10, 20);
	let string = String.fromArrayBuffer(substring);

#### function object

The function object differs from the ES6 specification in three ways, each of which reduces runtime memory use.

- The length property of a function, which indicates the expected number of arguments to the function, returns undefined.
- The name property of some functions returns undefined. The name property is present in the ES6 specification for debugging purposes, and the value of the name property is defined as implementation dependent in the standard. 
- The toString property of a function returns only a stub text representation of the function (e.g. the function source code or a decompiled version of the code is unavailable).

#### Executing JavaScript source code

Both the function object constructor and the eval function allow scripts to parse and execute JavaScript source code directly. This capability is implemented on Kinoma Element, however it is recommended that scripts use the function object constructor and eval rarely, if at all. The parsing of the ES6 JavaScript and subequent byte code generation requires considerable memory, and so may fail when memory is low or the script is large.

#### require(module) and require.weak(module)

In ES6 JavaScript, modules are accessed using the import statement.

	import console from "console";
	console.log("using import statment");

The import statement creates a variable named "console", visible to all code in the loading module, to access the properties of the imported module.

Prior to ES6, the import statement is unavailable. The CommonJS Module specification, often used by pre-ES6 code, defines the require function to access a module.

	console = require("console");
	console.log("using require function");

While the functionality is similar, the runtime behavior is different. The import statement completes execution before the script containing it begins execution. The require function executes only at the time it is called. This difference means that modules referenced using import are always loaded, whereas require only loads modules when invoked.

Kinoma Element implements the global require function to load ES6 modules. The require function returns the default export of the module.

	if (loggingEnabled) {
		let console = require("console");
		console.log("log entry");
	}

CommonJS defines all calls to require for a given module return the same exported object. That limits the ability of the garbage collector to collect the module. This behavior is convenient for developers, but tends to increase overall memory use. Kinoma Element implements the require.weak function to load a module and leave it eligible for garbage collection. If the module is already in memory, require.weak returns the same exported object, so there is never more than one copy of the module loaded.

	if (loggingEnabled) {
		let console = require.weak("console");
		console.log("log entry");
	}
	// console is out of scope here, so may now be garbage collected

> Note: Applications typically use the ES6 import statement. If a developer finds memory is tight, it may be worthwhile to consider using require or require.weak to reduce runtime memory.

> Note: Not all modules are designed to be garbage collected. Scripts should only use require.weak with modules that support unloading.

#### trace(string)

The trace global function is similar to console.log, but unlike console.log, trace does not add a line feed at the end of the output.

	trace("1");
	trace("2");
	trace("3\n");
	
	// output
	123

The trace function takes a single string argument.

> Note: The output of trace may not be visible until a line feed is output.

## Relationship to Kinoma Create

Kinoma Element and Kinoma Create are both combinations of hardware and software for building IoT projects, prototypes, and products. Kinoma Element considerably smaller and lighter product, suitable for embedding digital intelligence and Wi-Fi connectivity into just about anything. Kinoma Create is built around a much more powerful CPU with more memory (approximately 200x more) running the Linux operating system all to support the needs of the built-in touch screen, audio speaker, and microphone.

Despite their significant hardware differences, Kinoma Element and Kinoma Create share a great deal of software. This makes it easier for developers to move back and forth between then.

The following are some of the ways in which Kinoma Element and Kinoma Create have software compatibilty.

- Both are powered by the XS6 virtual machine, providing the same JavaScript language implementation
- Both use the Pins module for applications to communicate with hardware pins
- Both support the BLL model of JavaScript drivers for hardware components. The same BLL often works on both
- Both support the WebSocket API
- Both support the KPR Files API 
- Both use the Kinoma Code IDE for software development

> Note: While Kinoma Element and Kinoma Create offer many of the same software APIs, it is not intended that the same applications will work for both. The different form factor and power demand different a application for each. Developers often wil be able to re-use their code that communicates with pins, as well as many modules they create.

## Turning Kinoma Element off

The recommended way to turn off Kinoma Element is to press the power button. When the light goes out, Kinoma Element has powered down.

Using the power button to power down gives Kinoma Element an opportunity to cleanly exit the current application by calling its onQuit event and announce to the network that the device is going away.

> Note: If Kinoma Element is turned off by removing power, other devices on the network will not receive a notification that Kinoma Element has turned off. They may continue to show Kinoma Element services as available in their user interface. For example, Kinoma Code may continue to show a powered down Kinoma Element in its device list for several minutes before the mDNS announcement times out.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/element_intro.md] -->



<!-- # System -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_system.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_system.md} -->

## System object

The System object contains functions that report and configure fundamental device capabilities.

	import System from "system";

### Functions

#### addPath(path)

The addPath function adds one or more directories to the search path used when loading JavaScript modules.

	System.addPath("/k2/myModules/", "/k3/moreModules/");

> Note: The module search path is configured by the Kinoma Code IDE. Most applications will not need to add additional paths.

#### gc()

The gc function runs the JavaScript garbage collector.

	System.gc();

> Note: The garbage collector runs automatically when needed, so applications do not usually need to run the garbage collector.

#### reboot(force)

The reboot function causes the device to restart. If the optional force parameter is set to true, the device is immediately restarted without cleanly exiting any applications and services.

	System.reboot();

#### shutdown(force) 

The shutdown function causes the device to shutdown. If the optional force parameter is set to true, the device is immediately shutdown without cleanly exiting any applications and services.

	System.shutdown(true);

### Properties

#### device

The device property is a string indicating the model of the device running the script.

	console.log(`Device is ${System.device}`);	 // "K5" for Kinoma Element
	
The device property is read-only.

#### hostname

The hostname property is a string indicating the name of the device.

	console.log(`Hostname is ${System.hostname}`);
	System.hostname = "khin";

The hostname property can be read and written.

#### osVersion

The osVersion property is a string indicating the version of the operating system the device is running.

	console.log(`osVersion is ${System.osVersion}`);	 // osVersion is of the form "WM/3001016"

The osVersion property is read-only.

#### platform

The platform property is a string indicating the platform software name.

	console.log(`platform is ${System.platform}`);	 // "mc" (microcontroller)for Kinoma Element

The platform property is read-only.

#### time

The time property is a Number indicating Unix time in seconds.

	console.log(`time is ${System.time}`);
	System.time += 10;		// jump 10 seconds in the future

The time property can be read and written.

> Note: The System time property is initialized from a network source when Kinoma Element first boots. The realtime clock in Kinoma Element is not maintained when power is turned off.

#### timestamp

The timestamp property is a number indicating the Unix time when the operating system software was built.

	console.log(`timestamp is ${System.timestamp}`);

The timestamp property is read-only.

#### timezone

The timezone property is a object containing a Number specifying difference in seconds between the current device location and UTC, and a Number flag indicating if daylight savings time is active.

	console.log(`Timezone offset is ${System.timezone.timedifference}`);
	console.log(`Daylight savings is ${System.timezone.dst}`);
	
	System.timezone = {timedifference: 0, dst: 1};	// London with daylight savings time active

The timezone property can be read and written.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_system.md] -->



<!-- Environment -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_env.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_env.md} -->

## class Environment
The Environment class is a simple way for applications to store small pieces of data. There is a default system environment store, and applications can create additional environment stores as necessary. An environment store can be encrypted, making it suitable for storing passwords.

	import Environment from "env";

### Constructor

#### new Environment(path, autosave, encrypt)

The Environment constructor creates a new instance for accessing a single environment store. All parameters are optional.

The path parameter indicates the path to the environment store to use. If the environment store does not already exist, it is created. If the path argument is not present, the default system environment store is used.

The autosave parameter indicates if the Environment store should be saved after each change. If autosave is false or is not present, the Environment store is only saved when the save and close functions are called.

If set to true, the encrypt parameter causes the values in the environment store to be encrypted in storage. They are automatically decrypted when accessed. Encrypted values take more time to get and set. The default system environment store is not encrypted. The environment store used to maintain the passwords for Wi-Fi access points is encrypted.

> Note: Saving the environment store takes some time, so setting autosave to false allows the application to decide when to update the environment store. For applications that only occasionally update the environment store, it is recommended to set autosave to help ensure the integrity of the environment file.

### Functions

#### close()

Closes, and if necessary saves, the environment store. After calling close, no additional calls should be made to this Environment instance.

#### get(name)

The get function returns the value in the environment store associated with the key specified by the name string argument. If the key is not present in the environment store, get returns null.

	let env = new Environment();
	let firmwareVersion = env.get("FW_VERS");

#### save()

Save stores the content of the environment store. Save is automatically performed on close, or on set if autosave is enabled.

#### set(name, value)

The set function stores the value parameter (converted to a string, if needed) to the environment store under the key specified by the name. If there is already a value associated with the name, it is replaced with the specified value.

If value is not passed, or is set passed as undefined, set removes the key associated with name from the environment store.

	let env = new Environment("myEnv");
	env.set("foo", "test");
	let foo = env.get("foo");	// returns "test"
	env.set("foo");
	foo = env.get("foo");		// returns null

### Iterator

The Environment class implements an iterator for applications to retrieve all the keys of an environment store.

	let env = new Environment();
	for (let name of env)
		console.log(`${name} = ${env.get(name)}\n`);



<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_env.md] -->



<!-- # UUID -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/ext_uuid.md]{/Users/hoddie/Desktop/Element docs - Markdown/ext_uuid.md} -->

## UUID object

The UUID object creates, and optionally caches, Universally Unique Identifier (UUID) values. 

	import uuid from "uuid";

UUID values are strings.

### Functions

#### create()

The create function generates a new UUID value. The UUID value is generated from a random number, the current time, and the network connections's MAC address.

	for (let i = 0; i < 5; i++)
		console.log(`UUID ${i}: ${uuid.create()});

#### get(name)

The get function checks the UUID cache for a UUID of the name parameter. If found, the UUID is returned. If not found, a new UUID is generated, cached, and returned.

	let deviceUUID = uuid.get("device");



<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/ext_uuid.md] -->



<!-- # Time Interval -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_timeinterval.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_timeinterval.md} -->

## class TimeInterval
The TimeInterval class implements core timer functionality. Higher level timing functions, including setTimeout and setInterval, are implemented using TimeInterval.

	import TimeInterval from "timeinterval";

> Note: Only a limited number of TimeIntervals may be allocated in the system. The limit is 24 in the default configuration.

### Constructor

#### new TimeInterval(callback, interval)

The TimeInterval constructor creates a new TimeInterval instance. When the time interval fires, the function specified by the callback parameter is called. The initial interval is specified by the interval parameters in milliseconds.

The newly created TimeInterval instance is not active. The start function must be called to activate the interval.

	let timer = new TimeInterval(() => console.log(`another second ${Date.now()}`), 1000);
	timer.start();

### Functions

#### close()

The close function closes the TimeInterval instance, so no further callbacks fire. After calling close, no additional calls should be made to this TimeInterval instance.

#### start(interval)

The start function activates the TimeInterval so that the callback fires after the TimeInterval's interval has elapsed. The interval parameter is optional. If not present, the most recent interval passed to the constructor or start function is used.

#### stop()

The stop function deactivates the TimeInterval. The callback does not fire until start is called to activate the TimeInterval.

### Values

#### interval

The current interval in milliseconds of the TimeInterval instance. This is a read-only value.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_timeinterval.md] -->



<!-- # Timer -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_timer.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_timer.md} -->

## Timer module

The timer module contains implementations of four functions commonly used by HTML5 applications for time callbacks.

	import {setInterval, clearInterval, setTimeout, clearTimeout} from "timer";
	
The Timer module is implememted using the TimeInterval class. The Timer module guarantees that pending timeouts and intervals cannnot be garbage collected, whereas TimeInterval instances can be garbage collected.

The Kinoma Element application runtime makes the setInterval, clearInterval, setTimeout, and clearTimeout functions availabe as global functions for the convenience of developers famiilar with HTML5.

### Functions

#### clearInterval(interval)

clearInterval cancels a repeating callback created using setInterval.

	let repeater = setInterval(() => console.log(`Tick $(Date.now()`), 1000);
	....some time later
	clearInterval(repeater);

#### clearTimeout(timeout)

clearTimeout cancels a one time callback created using setTimeout.

	let oneshot = setTimeout(() => console.log("Two seconds later"), 2000);
	....less than two seconds later
	clearTimeout(oneshot);

#### setInterval(callback, delay)

setInterval schedules a repeating callback at an interval specified in milliseconds using the interval parameter.

	setInterval(() => console.log(`Tick $(Date.now()`), 1000);

setInterval returns a reference to the new interval which can be used to cancel the interval using clearInterval.

#### setTimeout(callback, delay)

setTimeout schedules a one time callback to be called after a specified time specified in milliseconds using the timeout parameter.

	setTimeout(() => console.log("Two seconds later"), 2000);

setTime returns a reference to the new timeout which can be used to cancel the interval using clearTimeout.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_timer.md] -->



<!-- # Watchdog Timer -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_watchdogtimer.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_watchdogtimer.md} -->

## Watchdog Timer object

The Watch Dog Timer object monitors device activity. If it detects that the device has been blocked for an extended period of time, the watch dog timer assumes the application has crashed and restarts the device.

	import wdt from "watchdogtimer";

The length of time the watch dog timer waits before deciding the application has crashed depends on the mode of operation. It is always at least 5 seconds.

Most applications do not need to operate interact with the watchdog timer directly, as they do not typically perform operations that block for an extended period. If an application performs a blocking operation that it expects to take more than 5 seconds, it has two options to prevent the watchdog timer from restarting the device:

* Periodically call the watch dog timer to tell it that the application is operating normally by calling strobe. This resets the watch dog timer.
* Disable the watch dog timer when starting the operating and restart it when complete.

In general, it is preferred for applications to use the strobe approach.

> Note: Certain operations temporarily disable the watchdog timer. In particular, the watchdog timer is disabled while debugging sesssion so breakpoints do not cause the watchdog to time out and reboot the device.

### Functions

#### resume()

The resume function reenables the watchdog timer after it has been disabled by the stop function.

	wdt.resume();

#### strobe()

The strobe function tells the watch dog timer that the application is properly funcitoning during a long task. Calling strobe resets the watchdog timer.

	wdt.strobe();

#### stop()

The stop function disables the watchdog timer. Stop should only be used when absolutely necessary as it prevents the system from detecting when an application is stuck in an infinite loop.

	wdt.stop();
	
Use the resume function to reenable the watchdog timer.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_watchdogtimer.md] -->



<!-- # Files -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_files.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_files.md} -->

## class Files
The Files class provides tools for working with the file system.

	import Files from "files";

The embedded file system implementation has some limitations:

- File paths can be up to 128 bytes, including the file name, file extension, and all parent directory names
- Path names are encoded with UTF-8 encoding, so non-ASCII characters use more than one byte.
- The file system uses "/" as the directory separator. The file system does not ignore multiple slashes in a row so "/k1/data/foo.txt" and "/k1/data//foo.txt" refer to different files.
- The path to a directory or volume always ends with a "/". The path to a file never ends with a "/".
- Empty directories are unsupported. If a directory contains no files or directories, it is automatically deleted.

> Note: The Files class is designed for upward compatibility with the KinomaJS Files API, although they are not identical.

### Iterators

#### Files.Iterator(path);

The Files.Iterator retrieves all the files and directories contained in the directory specified by the path parameter.

	for (let item of Files.Iterator(path))
		console.log(item.name);

#### Files.VolumeIterator();

The Files.VolumeIterator retrieves all the volumes on the device.

	for (let item of Files.VolumeIterator())
		console.log(`Volume name ${item.name} at path ${item.path}`);

### Static functions

#### deleteDirectory(path)

Deletes the directory specified by the path parameter. deleteDirectory is a recursive operation, so all files and directories contained in the directory are also deleted.

	Files.deleteDirectory("/k1/data/");

#### deleteFile(path)

Deletes the file with the specified path.

	Files.deleteFile("/k1/data/foo.txt");

#### deleteVolume(name)

Erases the volume with the specified name.

	Files.deleteVolume("k1");

#### getInfo(path)

Retrieves information about the file at the specified path. If no file exists at the specified path, Files.getInfo returns undefined.

	let path = "/k1/data/foo.txt";
	let info = Files.getInfo(path);
	console.log(`File ${path} has ${info.size} bytes.`);

#### getSpecialDirectory(name)

Retrieves the path to a special system designated directory associated with the name parameter.

	Files.getSpecialDirectory("documentsDirectory");

The following special directory names are available:

* applicationDirectory
* preferencesDirectory
* documentsDirectory
* variableDirectory
* nativeApplicationDirectory (Simulator only)

#### getVolumeInfo(path)

Retrieves information about the volume at the specified path. If no volume exists at the specified path, Files.getVolumeInfo returns undefined.

	let path = "/k1/";
	let info = Files.getVolumeInfo(path);
	console.log(`Volume ${path} has ${info.size} bytes with removable ${info.removable}.`);

#### read(path)

Reads the entire contents of the file at the specified path into an ArrayBuffer.

	let buffer = Files.read("/k1/data/foo.txt");
	console.log(String.fromArrayBuffer(buffer));

#### renameFile(from, to)

Renames the file at the path specified by the from parameter to the name given in the to parameter.

	Files.renameFile("/k1/data/foo.txt", "bar.txt");

#### write(path, buffer)

Replaces the content of the file at the specified path with the content of the buffer parameter. The buffer argument may be either an ArrayBuffer or String

	Files.write("/k1/data/foo.bin", new ArrayBuffer(10));
	Files.write("/k1/data/foo.txt", "Hello, world.");


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_files.md] -->



<!-- # File -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_file.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_file.md} -->

## class File
The File class provides tools for reading and writing data to an individual file.

	import File from "file";

> Note: See the Files class for information on paths.

### Constructor

#### new File(path, mode)

The File constructor opens the file specified by the path argument. The mode parameter is optional. If the mode is 0 or unspecified, the file is opened for read-only access. If the mode is 1, the file is opened for write access.

If the file does not exist and the mode requests write access, the file is created and then opened.

	let file = new File("/k1/data/foo.txt");

### Functions

#### close()

Closes the file. After calling close, no additional calls should be made to this Files instance.

#### read(type, count, buffer)

Performs a read of data from the file. The count specifies the number of bytes read. The type argument indicates which JavaScript object to read the data into. Pass String for type to read the data into a string, pass ArrayBuffer to read the data into an ArrayBuffer.

	let str = file.read(String, 5);
	let bytes = file.read(ArrayBuffer, 10);
	let all = file.read(ArrayBuffer, file.bytesAvailable);

If type specifies an ArrayBuffer, an ArrayBuffer can be passed in the buffer argument as an optimization to minimize buffer allocations.
	
	let buffer = new ArrayBuffer(10);
	file.read(ArrayBuffer, butter.byteLength, buffer);
	file.read(ArrayBuffer, butter.byteLength, buffer);

#### readChar()

Reads one byte from the file, returning the value as a Number between 0 and 255.

	let byte = file.readChar();

#### write(..items)

The write function stores data to the file.

Each argument is an integer representing a byte value to store, a string, an ArrayBuffer, or an array of integers, strings, ArrayBuffers, and Arrays.

	file.write(42);
	file.write("a string", [13, 10]);
	file.write((buffer.length >> 8) & 0xff, buffer.length & 0xff, buffer);

### Values

#### length

The length property is the total number of bytes in the file. Read this property to retrieve the size of the file, and set it to change the size of the file.

	console.log(`This file contains ${file.length} bytes.`);
	file.length = 0;	// empty file contents

#### position

The position property is the current read/write position in the file. This property may be read and written.

The read, readChar, and write functions update the position. When the file is first opened, the position is 0.

	file.position = file.length;	// set position to end of file


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_file.md] -->



<!-- # HTTP Client -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_http_client.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_http_client.md} -->

## class HTTPClientRequest

The HTTPClientRequest class implements support for making a request to an HTTP server.

	import HTTPClientRequest from "HTTPClient";

HTTPClientRequest supports HTTP and HTTPS connections.

### Examples

#### GET JSON

	let request = new HTTPClientRequest("https://www.kinoma.com/example.json");
	request.onTransferComplete = success => {
		let body = String.fromArrayBuffer(request.content);
		let message = JSON.parse(body);
		// etc.
	}
	request.start();

#### POST JSON

	let request = new HTTPClientRequest("https://www.kinoma.com/post");
	request.method = "POST";
	request.setHeader("Content-Type", "application/JSON");
	request.start(JSON.stringify({
		command: "something",
		value: 12,
		option: "do the right thing"
	}));

#### Download to file

	let request = new HTTPClientRequest("http://www.kinoma.com/file.txt");
	let file = new Files("/k1/file.txt");
	request.onDataReady = buffer => file.write(buffer);
	request.onTransferComplete = success => file.close();
	request.start();

### Constructor

#### new HTTPClientRequest(url)

The HTTPClientRequest constructor takes a URL parameter to initialize a new HTTP request. Call start to initiate the request to the server.

	let request = new HTTPClientRequest("http://www.kinoma.com/index.html");

The new HTTP request is initialized with a request method of GET. Set the method property to change it.

### Functions

#### getHeader(name)

Retrieves the header from the HTTP response headers. If no header is found with the specified name, undefined is returned.

	let when = request.getHeader("date");

HTTP response headers are only available after the onHeaders event is invoked.

> Note: HTTP header names are case insensitive, so getHeader("FOO") and getHeader("Foo") refer to the same header.

#### setHeader(name, value)

Adds an header name and value to the HTTP request headers.

	request.setHeader("Content-Type", "text/plain");

#### start(body)

Begins the HTTP request to the server.

If present, the optional body parameter is a String or ArrayBuffer for the request body.

### Values

#### content

The content property contains the complete HTTP response body after the onTransferComplete event is called.

If an application replaces the onDataReady event, the content is undefined.

#### method

The method property contains the request method of the request.

	request.method = "HEAD";
	
The default method is "GET".

#### statusCode

The statusCode property contains the HTTP status in the response headers. It is available after the onHeaders event has fired.

	if (request.statusCode == 404)
		console.log("resource not found");

### Events

#### onDataReady(buffer)

The onDataReady event is called when data arrives for the response body. The buffer parameter is an ArrayBuffer containing the data. onDataReady may be called multiple times for a single request.

	request.onDataReady = buffer => {
		console.log("Received ${buffer.byteLength} bytes");
		super.onDataReady(buffer);
	}

> Note: Applications are not required to implement onDataReady. The default implementation of onDataReady concatenates all the data into a single ArrayBuffer that is available when the onTransferComplete event is called. The onDataReady event is useful for applications that require immediate processing of the incoming data.

#### onDataSent()

The onDataSent event is called after the request body has been completely transmitted to the server. If there is no request body, onDataSent is not called.

	request.onDataSent = () => {
		console.log("HTTP request body sent");
	}

#### onHeaders()

The onHeaders event is called when the HTTP response headers are available.

	request.onHeaders = () => {
		this.mime = request.getHeader("Content-Type");
	}

#### onTransferComplete(success)

The onTransferComplete event is called when the entire HTTP response body has been received. The success parameter is a boolean indicating if the request completed successfully. In some cases the HTTP statusCode may not be known, for example when the server is unreachable.

The message body is stored in the content property of the HTTPClientRequest instance, unless the application has replaced the default implementation of onDataReady.

	request.onTransferComplete = success => {
		if (success && (200 == request.statusCode)) {
			let bodyText = String.fromArrayBuffer(request.content);
			console.log(bodyText);
		}
		else
			console.log(`request failed with error ${request.statusCode}`);
	}

<!--
	HTTPClientRequest.close()	- for consistency and control

	Chunked encoding response body in client
	Request body - not headers added (Content-Length, at least)
 	User name & password on client

-->


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_http_client.md] -->



<!-- # HTTP Server -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_http_server.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_http_server.md} -->

## class HTTPServer

The HTTPServer class provides support for applications to implement HTTP and HTTPS servers.

	import HTTPServer from "HTTPServer";

When the server receives a request, it calls the onRequest event to respond, passing the HTTPServerRequest instance that represents the request.

### Examples

#### Echo server

This HTTP server responds to all requests with the requested URL in a text document.

	let server = new HTTPServer({port: 80});
	server.onRequest = request => {
		request.response("text/plain", `Your URL is ${request.url}.`);
	}

#### File server

This HTTP server returns files based on the URL path. The form of the URL is http://[ip address]/file/path, for example http://10.0.12/file/k1/foo.txt.

	let server = new HTTPServer({port: 80});
	server.onRequest = request => {
		if (request.url.startsWith("/file/")) {
			let data = Files.read(request.url. substring("/file/".length));
			request.response("text/plain", data);
		}
		else
			request.errorResponse(403, "Forbidden");
	}

#### REST JSON server
This HTTPS server responds to JSON requests with a portion of their JSON data converted to lowercase.

	let server = new HTTPServer({port: 443, ssl: true});
	server.onRequest = request => {
		let json = JSON.parse(request.content);
		if (json.name)
			json.name = json.name.toLowerCase();
		request.response("application/JSON", JSON.stringify(json));
	}

### Constructor

#### new HTTPServer(params)

The HTTPServer constructor takes a single argument, a dictionary of initialization properties.

	let server = new HTTPServer({port: 80});
	let secureServer = new HTTPServer({port: 443, ssl: true});

There are three properties in the params dictionary:

* port - the port number that the HTTP server will listen on for connection requests.
* socket - a ListeningSocket to use for listening for connection requests. If a socket is not provided, the HTTPServer will allocate one.
* ssl - a Boolean indicating if this connection should be secure.

> Note: The server is active immediately after being created.
> 
<!-- @@ ssl should be renamed tis or just secure? 
If value is a number, is it possible to request a specific version of TLS?
-->

### Functions

#### close()

The close function terminates the HTTPSever. All active connections are closed immediately, as is the listening socket.

> Note: The server closes the listening socket only if it allocated it.

### Events

#### onRequest(request)

The onRequest event is called when the HTTP server recieves a new connection. When the onRequest event is received, the request headers and request body, if any, are available.

	server.onRequest = request => {
		console.log(`URL: ${request.url);
		console.log(`Date header: ${request.getHeader("date")});
		if (request.content)
			console.log(`Request body: ${String.fromArrayBuffer(request.content)}`);
		request.response("text/plain", "Hello");
	}


Multiple HTTP server requests may be active at the same time.

<!-- @@
How to spool a large response? responseWithChunk, putChunk, terminateChunk... nothing tells HTTP server the request is done? Connection will stay open until time out? No flush. 

-->

## class HTTPServerRequest

An HTTPServerRequest object is created by the HTTPServer for each incoming request. The request object is passed to the application by the onRequest even on the HTTP server.

> Note: Applications do not create HTTPServerRequest objects directly.

### Functions

#### errorResponse(statusCode, reason, close)

Sets the response for a request that was not successfully handled. The HTTP status code is passed in the statusCode parameter. The text reason for failure is passed in the reason parameter. If the optional close parameter is true, the socket associated with the request are closed immediately after the response is sent.

	request.errorResponse(404, "Not Found");
	request.errorResponse(403, "Forbidden", true);	// close immediately	

#### getHeader(name)

Retrieves the header from the HTTP request headers. If no header is found with the specified name, undefined is returned.

	let when = request.getHeader("date");

> Note: HTTP header names are case insensitive, so getHeader("FOO") and getHeader("Foo") refer to the same header.

#### putChunk(buffer)

putChunk sends part of the data in a chunked response. See responseWithChunked for an example.

#### response(contentType, buffer, close)

Sets the response for a request that was successfully handled. The contentType parameter is the MIME type of the data. The buffer parameter is either a String or ArrayBuffer containing the response body. If the optional close parameter is true, the socket associated with the request is closed immediately after the response is sent.

	request.response();		// no response body
	request.response("text/plain", "Hello");
	request.response("application/JSON", JSON.stringify({foo: 1});

#### responseWithChunk(contentType)

The responseWithChunk function sends a chunked response, e.g. a response using "Transfer-encoding: chunked". responseWithChunk is used together with putChunk and terminateChunk. The optional contentType parameter sets the Content-Type header in the response.

	server.onRequest = request => {
		this.request = request;
		request.responseWithChunk();
	}

	... some time after onRequest is received
	this.request.putChunk("some text");

	... more time passes
	this.request.putChunk(new ArrayBuffer(5));
	this.request.terminateChunk();

#### setHeader(name, value)

Adds an header name and value to the HTTP response headers.

	request.setHeader("Content-Type", "text/plain");

#### terminateChunk(footer)

terminateChunk finishes sending a chunked response. See responseWithChunked for an complete example. The optional footer parameter is a dictionary sent at the end of the message.

	request.terminateChunk({status: 3});

### Values

#### content

The content property contains the complete HTTP request body as an ArrayBuffer.

#### method

The method property is the HTTP method of the request as a string.

#### url

The url property is the path, query, and fragment of the URL of the request made to the http server.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_http_server.md] -->



<!-- # WebSockets -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/websocket.md]{/Users/hoddie/Desktop/Element docs - Markdown/websocket.md} -->

## WebSocket module

The websocket module implements exports two objects, a WebSocket client constructor and a WebSocketServer construtor.

	import {WebSocket, WebSocketServer} from "websocket";

The WebSocket constructor is compatible subset of the WebSocket object  in HTML5. The WebSocketServer implements an API based on the WebSocket client.

The websocket module supports WS and WSS connections.

### Examples

#### WebSocket client

A simple WebSocket client that connects to an echo server, sends a single message, and when the response is received closes the connection. All events are logged to the console.

	let client = new WebSocket("ws://echo.websocket.org");
	client.onopen = () => {
		console.log(`ws client open`);
		client.send("HELLO");
	}
	client.onmessage = msg => {
		console.log(`ws client message ${msg.data}`);
		client.close();
	}
	client.onclose = () => console.log(`ws client close`);
	client.onerror = () => console.log(`ws client error`);

#### WebSocket server

A simple WebSocket server that listens for connections on port 10000. The server echos back to the client each message received.

	let server = new WebSocketServer(10000);
	server.onStart = function(client) {
		client.onopen = () => console.log(`ws server client.open`);
		client.onmessage = msg => {
			console.log(`ws server client.onmessage ${msg.data}`);
			client.send(msg.data);
		};
		client.onclose = () => console.log(`ws server client.close`);
		client.onerror = () => console.log(`ws server client.error`);
	};
	server.onClose = () => console.log(`ws server close`);

The WebSocketServer invokes the onStart event for each new connection, passing a client instance. The client instance implements the same API as the WebSocket client.

### Constructors

#### new WebSocket(url)

Creates a new WebSocket client and initiates a connection to the url specified. If the connection is established successfully, the onopen event is invoked. If the connection attempt fails, the onerror event is invoked.

	let client = new WebSocket("ws://echo.websocket.org");

#### new WebSocketServer(port)

Opens a new WebSocket server, listening on the port specified. When a new connection is received, the onStart event is invoked.

	let server = new WebSocketServer(10000);

### Functions - WebSocket client

#### close()

Closes the WebSocket client, terminating the connection to the server. No calls to the WebSocket client should be made after calling close.

	client.close();

#### send(data)

Transmits the data. The data can be either an ArrayBuffer or a String. WebSockets always transmits Strings as UTF-8 data.

	client.send("Hello");
	client.send(new ArrayBuffer(8));

### Events - WebSocket client

#### onclose()

The onclose event is called when the remote endpoint closes the connection.

#### onerror()

The onerror event is called when an error is detected, such as a dropped connection.

#### onmessage(msg)

The onmessage event is invoked when a message is received from the remote endpoint. The message data is avaiable on msg.data. The data can be either a String or an ArrayBuffer.

	function onmessage(msg) {
		if ("String" === typeof msg.data)
			console.log("received String");
		else
			console.log("received ArrayBuffer");
	}

#### onopen()

The onopen event is invoked when the connection is successfully established to the remote endpoint.

### Functions - WebSocket server

#### close()

Closes the WebSocket server, terminating active client connections.

	server.close();

### Events - WebSocket server

#### onStart(client)

The onStart event is invoked each time a new client connects to the server. The new client instance is passed to the event, which should set the event handlers needed to interact with the client.

A trivial onStart handler that waits for the first message from the client, sends a "goodbye" message, and then closes the connection.

	server.onStart = function(client) {
		client.onmessage = msg => {
			client.send("goodbye");
			client.close();
		};
	};


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/websocket.md] -->



<!-- # mDNS -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/mdns.md]{/Users/hoddie/Desktop/Element docs - Markdown/mdns.md} -->

## mDNS object

The mdns object implements both the client and server funtions of the Zero Configuration network protocol, which is also known as Bonjour.

	import mdns from "mdns";

### Examples

#### Announce service

This announces a telnet service avaiable on port 2323.

	mdns.add("_telnet._tcp", System.hostname, 2323);

This ends the announcement of the telnet service.

	mdns.remove("_telnet._tcp");

#### Monitor for service

This monitors the local network for available telnet services.

	mdns.query("_telnet._tcp" service => {
		console.log(`${service.status} "${service.keys.name}" ${service.service} at ${service.addr}.${service.port}`
	});

	output:
		found "Bob's Element" _telnet._tcp at 10.0.0.14:2323
		lost "Bob's Element" _telnet._tcp at 10.0.0.14:2323

This ends monitoring the local network for telnet services.

	mdns.query("_telnet._tcp");

### Functions

#### add(service, name, port, txt, ttl)

Begins announcing available of a new network service. 

	mdns.add("_telnet._tcp", "Bob's Kinoma Create", 2323);

The format of the service parameter string is defined by the mDNS specificaiton. The name parameter is a human readable string that typically identifies the device running the service. The port parameter is the port number the server implementing the service is avalable on.

The optional txt parameter is an object whose properties key/value pairs to include in the mDNS service TXT record.

	mdns.add("_telnet._tcp", "Bob's Kinoma Create", 2323, {status: "online", launched: Date.now()});

The mDNS protocol limits the size of the TXT record. Use the update function to modify the TXT record afer adding the service. 

The optional ttl parameter is the "time to live" in seconds. If the ttl parameter is omitted, the default value is 255.

#### query(service, callback)

Register a callback function to be invoked when instances of the specified service are discovered or lost.

	mdns.query("_telnet._tcp", service => {
		console.log(`${service.status} "${service.keys.name}" ${service.service} at ${service.addr}.${service.port}`
	});

The service object contains the following properies:

- status: "found" when the device is initially discovered, "update" when values in the TXT record change, and "lost" when the device is no longer available
- service: the type of the service, e.g. `_kinoma_pins._tcp`
- name - the human readable name of the service
- addr: the IP address of the service, e.g. "10.0.1.14"
- port: the port number the service is available on
- keys.txt - the key/value pairs provided by the device

The following is a complete service record provided by the Kinoma Element Pins sharing service.

	{
		status: "found"
		service: "_kinoma_pins._tcp",
		name: "fakeBLL test",
		addr: "10.0.1.14",
		port: 9999,
		keys: {
			txt: {
				bll: "fakeSensor",
				uuid: "000167C3-67C3-1001-E494-00504302fe01",
				_ws: "ws://*:8900/"
			}
		}
	}

To end notifications, call query without the callback parameter.

	mdns.query("_telnet._tcp");

#### remove(service)

Ends the announcement of the service named. The service parameter string must exactly match the string used when calling add.

	mdns.remove("_telnet._tcp");

#### resolv(name, callback)

Performs a one time search for the specified device name.

	let name = "Bob's Kinoma Create";
	mdns.resolv(name, address => {
		if (address)
			console.log(`Found ${name} at IP address ${address}`);
		else
			console.log(`Unable to find ${name}`);
	});

#### update(service, txt)

Changes the content of the TXT record included in the mDNS service record.

	mdns.update("_telnet._tcp", {status: "door open"});

To clear the TXT record, omit the txt parameter.

	mdns.update("_telnet._tcp");


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/mdns.md] -->



<!-- # SSDP -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_ssdp.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_ssdp.md} -->

## SSDP object

The SSDP object implements the server side of the Simple Service Discovery Protocol used by the UPnP Device Architecture and others. 

	import SSDP from "ssdp";

> Note: The SSDP object provides the announcement of a service to the network. The service is provided by an HTTP server, which the application needs to implement separately.

### Functions

#### add(description)

The add function adds an SSDP service description to the list of descriptions the SSDP object broadcasts for discovery.

	SSDP.add({
		DEVICE_TYPE: "shell",
		DEVICE_VERSION: 1,
		DEVICE_SCHEMA: "urn:schemas-kinoma-com",
		HTTP_PORT: 10000,
		LOCATION: "/",
	});

The HTTP_PORT and LOCATION properties define the HTTP server for clients to  communicate with this service.

> Note: The SSDP object supports announcing multiple services at the same time.

#### remove(description)

The remove function removes an SSDP service description from the list of descriptions the SSDP broadcasts for discovery.

	SSDP.remove({
		DEVICE_TYPE: "shell",
		DEVICE_VERSION: 1,
		DEVICE_SCHEMA: "urn:schemas-kinoma-com",
		HTTP_PORT: 10000,
		LOCATION: "/",
	});

All five fields in the service description must match between add and remove for the service to be removed.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_ssdp.md] -->



<!-- # Net -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/net.md]{/Users/hoddie/Desktop/Element docs - Markdown/net.md} -->

## Net object

The Net object provides functions which are used to work with URLs and network addresses.

	import Net from 'net';

### Functions

#### parseUrl(url)

parseURL breaks a URL into its consituent parts: scheme, user, password, authority, path, name, query, and fragment. The parts are returned in an object.

	let parse = Net.parseUrl("http://www.kinoma.com:123/where?command=2#frag");
	// parse.scheme == "http"
	// parse.port == 123
	// parse.path == "where"
	// parse.query == "command=2"
	// parse.fragment = "frag"

<!-- @@ not parseURL or parseURI?? -->

#### isDottedAddress(address)

isDottedAddress examines the address parameter to determine if it is a valid IP address in the form ww.xx.yy.zz, such as 10.0.1.4.

	isDottedAddress("www.kinoma.com"); // false
	isDottedAddress("10.0.1.2"); // true
	isDottedAddress("10.0.1");	 // false
	isDottedAddress("10.0.1.256"); // false


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/net.md] -->



<!-- Socket, ListenerSocket, SecureSocket -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_socket.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_socket.md} -->

## class Socket
The Socket class implements a non-blocking network socket API with support for both TCP streams and UDP.

	import {Socket, ListeningSocket} from "socket";

### Constructor

#### new Socket(params)

The params argument contains a dictionary of configuration values for the new socket.

	let tcp = new Socket({host: "www.kinoma.com", port: 80, proto: Socket.TCP});
	let udp = new Socket({host: BROADCAST_ADDR, port: BROADCAST_PORT, proto: Socket.UDP, multicast: addr, ttl: 255});

The following properties are supported by the dictionary

* **host**: the host name or IP address in the notation (IP4 only)
* **port**: the port number to connect
* **proto**: the protocol for the socket, either Socket.TCP or Socket.UDP
* **multicast**: the interface address in the dot notation (IP4 only) (optional) (UDP only)
* **ttl**: the maximum hop count beyond the local network

### Functions

#### close()

Immediately disposes the socket, releasing any associated resources.

#### flush()

Forces all pending data to be sent.

> Note: most applications don't need to flush data. This function is primarily for internal use.

#### read(type, count, buffer)

Performs a non-blocking read of data from the socket. The count parameter specifies the number of bytes to read. The type argument indicates which JavaScript object to read the data into. Pass String for type to read the data into a string, pass ArrayBuffer to read the data into an ArrayBuffer.

	let str = socket.read(String, 5);
	let bytes = socket.read(ArrayBuffer, 10);
	let all = socket.read(ArrayBuffer, socket.bytesAvailable);

If type specifies an ArrayBuffer, an ArrayBuffer can be passed in the buffer argument as an optimization to minimize buffer allocations.
	
	let buffer = new ArrayBuffer(10);
	socket.read(ArrayBuffer, butter.byteLength, buffer);
	socket.read(ArrayBuffer, butter.byteLength, buffer);

#### recv(byesAvailable, buffer)

Performs a non-blocking read of data from the socket into an ArrayBuffer. byesAvailable indicates the number of bytes to read, and is limited by the number of bytes read to be read as indicated by socket.bytesAvailable. 

An existing ArrayBuffer can be passed in the optional buffer argument as an optimization to minimize buffer allocations.

	let buffer = socket.recv(8);

> Note: recv is the low level function used to implement the high level read function. Most applications use read, not recv.

#### send(data, address)

send is a non-blocking function to transmit data on the socket. 
The single data argument can be an integer representing the byte value to transmit, a string, an ArrayBuffer, or an array containing integers, strings, ArrayBuffers, and Arrays. The address parameter is optional, and only used for UDP sockets. If provided it is a string indicating the IP address and port to send the data to.

	socket.send("a string");
	socket.send(13);
	socket.send(["a string", 13, 10]);
	socket.send(arrayBuffer, "10.0.0.3:2882");

If the data cannot be completely transmitted without blocking, send returns 0 and no data is transmitted. If all data is successfully is transmitted, send returns 1. If an error occurs, an exception is thrown.

Applications can check the socket.bytesWritable property for how many bytes the socket is able to accept for transmission.

> Note: send is a low level function, used to transmit UDP packet data and to implement the high level write function. Most applications use the write function.

#### write(...items)

write is a non-blocking function to transmit data on the socket.

Each argument is an integer representing a byte value to transmit, a string, an ArrayBuffer, or an array of integers, strings, ArrayBuffers, and Arrays.

	socket.write("a string", [13, 10]);
	socket.write((buffer.length >> 8) & 0xff, buffer.length & 0xff, buffer);

The write function only transmits complete arguments. The return value indicates how many arguments were transmitted. If the data can be completely transmitted without blocking, write returns the number of arguments. If there are three arguments and only the first can be transmitted, write returns 1.

Applications can check the socket.bytesWritable property for how many bytes the socket is able to accept for transmission.

### Events

#### onClose()

The onClose event is generated when the socket is about to be closed. This can happen as the result of an error, for example when the devices disconnects from the network.

The onClose function is responsible for calling close on the socket.

	function onClose() {
		// clean-up
		this.close();
	}

#### onConnect()

The onConnect event is generated when the socket successfully connects to the host specified in the dictionary passed to the constructor.

#### onData(buffer)

The onData event is generated with bytes that have been received by the socket. The buffer argument contains the bytes that have been read from the socket in an ArrayBuffer.

#### onError()

The onError event is generated when the socket encounters an error, such as loss of network connection. The application can perform any necessary clean-up.

The onError function is responsible for calling close on the socket.

	function onError() {
		// clean-up
		this.close();
	}

#### onMessage(bytesAvailable)

The onMessage event is generated when data is ready to be received by the socket. If the onMessage event is not overridden, the default implementation reads the data available on the socket and generates the onData event. The bytesAvailable argument indicates the number of bytes ready to be read from the socket.

> Note: Most applications use onData, instead of onMessage. onMessage is useful for applications that require more control over how the data is read.

### Values

#### addr

The IP address of the socket. This is a read-only value.

#### bytesAvailable

The number of bytes ready to be read on the socket.  This is a read-only value.

#### bytesWritable

The number of bytes that can be written to the socket. This is a read-only value.

#### port

The port number of the socket. This is a read-only value.

#### peer

The IP address and port this socket is connected to in a string in the format of "[peerAddr]:[peerPort]", for example "10.0.1.12:80".

#### peerAddr

The IP address this socket is connected to.

#### peerPort

The port number this socket is connected to.

### Static functions

#### static resolv(name, callback)

Performs an asynchronous name resolution. The name argument is the name to be resolved. The callback argument is the function to call when the name resolution is complete. The callback function a single argument, which is a string with the IP address the name resolved to, or null if the name could not be resolved.

	Socket.resolv("www.kinoma.com" result => console.log(`resolved to ${result ? result : "ERROR"}));

### Static constants

#### Socket.TCP = "tcp"

#### Socket.UDP = "udp"

## class ListeningSocket extends Socket

The ListeningSocket class implements a network listener for use by network server protocols implementations that accept incoming connections, such as HTTP and WebSockets.

### Constructor

#### new ListeningSocket(params)

	sock = new ListeningSocket({port: 5151, proto: Socket.UDP});
	sock = new ListeningSocket({addr: "239.255.255.250", port: 1900, proto: Socket.UDP, membership: ifc.addr, ttl: 2});

### Functions

#### accept(socket)

Call accept() to accept an incoming connection request, and return the a socket for the connection.

	let incoming = socket.accept();

### Events

#### onConnect()

The onConnect event is generated when an incoming connection request is received. Use socket.accept() to accept the connection request and create a new socket for the connection.

## class SecureSocket extends Socket

The SecureSocket class implements the TLS protocol using a Socket. SecureSocket is a building block for secure communication, including the https and wss protocols.

	import SecureSocket from "SecureSocket";

### Constructor

#### new SecureSocket(params)
The SecureSocket extends the initialization dictionary of the Socket object with an options property to configure the TLS connection.

	let socket = new SecureSocket({host: adr, port: port, proto: Socket.TCP, options: {extensions: {server_name: adr, max_fragment_length: 1024}}});

SecureSocket adds the following properties to the params dictionary:

* **protocolVersion**: The version of the TLS protocol to implement, currently supports 3.1 (0x3031), 3.2 (0x3032), and 3.3 (0x3033). Note that 3.3 is supported only on Kinoma Element. The default is 3.1. 
* **cache**: If true, enable the session cache; if false, disable. The default is true.
* **verifyHost**: If true, verify the host name in certificates. The default is false.
* **extensions**: An object that contains additional options:
* **extensions.server_name**: The name of the server the client is connecting to.
* **extensions.max_fragment_length**: The maximum fragment length of a TLS packet on this socket. The default value is 16,384.


<!-- @@ can we merge extensions and options back to the root? - would be much simpler to write -->

> Note: The SecureSocket object will support specifying a certification in the future.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_socket.md] -->



<!-- # Debug -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_debug.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_debug.md} -->

## Debug object
The Debug object contains functions to assist in debugging JavaScript code.

	import Debug from "debug";

> Most applications do not need to use the Debug object directly. The functions on the Debug object are used to implement support for IDEs such as Kinoma Code.

### Functions

#### gc(flag)

The gc function turns the JavaScript garbage collector on if flag is true and off if flag is false.

	Debug.gc(false);
	...
	Debug.gc(true);

> Note: Disabling the garbage collector on memory a constrained device like Kinoma Element is not recommended as it may increase JavaScript memory use leading to system instability.

#### login(host, name)

The login function establishes an debugging connection to the xsbug debugger running at the address specified by the host parameter. The name parameter is optional and specifies the name of the application being debugged.

	let success = Debug.login("10.0.1.2", "my app");
	let success = Debug.login("10.0.1.3:5002", "my app");

> Note: Only one debugging connection can be active at a time.

#### logout()

The logout function terminates the active xsbug debugging session, if one is active.

	Debug.logout();

##### report(silent)

The report function provides information about the current memory use of the JavaScript virtual machine.

Calling report with silent set to false, or with the silent parameter omitted, outputs a memory report to the console.

	=== heap info ===
	malloc: 18672 free, 338 allocations, 15552 	biggest block, 40752 by Fsk
	heap2: 0x123340, 0x14fa80, 182080 remains
	heap3: 0x2001f7f4, 12 remains
	===
	# Chunk allocation: reserved 26432 used 19768 peak 26416 bytes, 1 blocks
	# Slot allocation: reserved 97520 used 95648 peak 95984 bytes, 0 free

Calling report with silent set to true returns an object with information about the current memory.

	let info = Debug.report(true);
	console.log(`Slot memory: ${info.slot}, chunk memory: ${info.chunk}`);

#### setBreakpoint(file, line)

The setBreakpoint function adds a debugging breakpoint for the specified file at the specified line number.

	Debug.setBreakpoint("/k1/main.jsb", 10);


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_debug.md] -->



<!-- # Console -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_console.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_console.md} -->

## Console object

The console object provides functions to control the output of diagnostic information.

	import console from "console";

Console output is sent to the following output locations, when active:

* Telnet
* USB serial connection
* Log file at the path /k2/log
* JavaScript debugging connection (Kinoma Code, Kinoma Studio, xsbug)

> Note: The console is blocked during certain operations. For example, when stopped at a breakpoint in the debugger, the console will not respond.

### Functions

### log(...params)

The log function accepts one or more parameters, converts them to human readable form, and outputs them to the active console outputs.

The log function is converts the following JavaScript objects to human readable strings for output: undefined, null, Boolean, Number, String, Symbol, Function, Array, and Object.

	console.log("one", 2, {three: 3}, ["four"]);

### Values

#### enable

The enable property is true when the console it output to the console log file and false when output the log file is inactive.

	console.enable = true;

The enable property can be read and written.


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_console.md] -->



<!-- # Command Line Interface (CLI) -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_cli.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_cli.md} -->

## Command Line Interface (CLI) object

The CLI implements the parsing and execution of commands. The CLI object is used by telnet and USB to execute commands.

	import CLI from "CLI";

Applicatons do not usually invoke the CLI object directly. However, for debugging purposes it can be useful to exceute a command programmatically, for example to display system state at a specific point in execution.

### Functions

### evaluate(line)

The evaluate function takes a single command line as input in the line parameter. If command line is sucessfully parsed, it is executed and any output is sent to the console.

	CLI.evaluate("modules"); //loaded modules
	CLI.evaluate("scan"); // visible Wi-Fi access points
	CLI.evaluate("printenv"); // default environment store


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_cli.md] -->



<!-- # Command Line Reference -->


<!-- include [/Users/hoddie/Desktop/Element docs - Markdown/xm_cli_reference.md]{/Users/hoddie/Desktop/Element docs - Markdown/xm_cli_reference.md} -->

## Command Line Interface Reference

The Kinoma Element Command Line Interface (CLI) is available over telnet and USB.

Command line parameters are separated by spaces. Parameters that contain spaces are enclosed in double quotes.

	cd k1
	cat "a file.txt"

###Reference

#### `cat` path

Displays the contents of the file at the location specified by "path".

	[jphAir.local]$ cat /k1/mc.env
	.ver\01\0FW_VER\00.99.7\0ELEMENT_SHELL\01\0UUID
	\025FB6A35-95CB-1A04-82EE-000000000000\0XSBUG_HOST\010.0.1.69:5003\0\0

#### `cd` [path]

Changes the working directory. If "path" is not provided, the working directory is set to the root directory. The "path" can be relative or absolute.

#### `connect` ssid security password hidden save

Connect to the specified Wi-Fi access point. The name of the Wi-Fi access point is given in the ssid parameter. Security is set to "wpa2" for access points using the WPA2 protocol or "none" for access points with no security. The password must always be provided; for open access points use "undefined" for the password. Set hidden to true if the access point is hidden, otherwise set it to false. Set save to true to save this connection to the wifi environment variable group so it will be used for automatic connections in the future.

	[jphAir.local]$ connect "KinomaWPA" wpa2 cube-able-stub false false
	[jphAir.local]$ connect "Marvell Cafe" none "" false true

#### `date`

Display the current date and time as returned by `Date()`.

#### `eval` expression

Runs the JavaScript eval function on the specified expression.

	[jphAir.local]$ eval 12+13
	25
	[jphAir.local]$ eval "console.log('Hello, world.')"
	Hello, world.

#### `gc`

Run the JavaScript garbage collector to free any unreferenced objects, including modules.

#### `getenv` name

Display the environment variable "name" from the default environment varilable group.

#### `hexdump` path

Displays the contents of the file at the location specified by "path" in hexadecimal format.

	[jphAir.local]$ hexdump /k1/mc.env
	2e 76 65 72 00 31 00 46 57 5f 56 45 52 00 30 2e 
	39 39 2e 37 00 45 4c 45 4d 45 4e 54 5f 53 48 45 
	4c 4c 00 31 00 55 55 49 44 00 32 35 46 42 36 41 

#### `hostname`

Display the hostname of the Kinoma Element.

#### `ip`

Display the IP address of the Kinoma Element.

#### `launch` path

Launches the application at the path specified.

#### `load` module

Loads the specified module using require.weak().

#### `ls` path

List the files and directories in path. If path is not specified, the working directory is used.

	[jphAir.local]$ ls /k1
	mc.env
	wifi

#### `mac`

Display the MAC address of the Kinoma Element.

#### `modules`

Displays the currently loaded modules. Use `modules te` to display only loaded modules that begin with "te".

	[jphAir.local]$ modules
	 1: application
	 2: board_led
	 3: CLI
	 4: console
	 5: inetd
	 6: mdns
	 7: pinmux

#### `netstat`

Displays details about the current network status.

	[jphAir.local]$ netstat	
	Type	l_port		l_ipaddr	r_port		r_ipadd   State
	CP		8081		0.0.0.0        	1		0.0.0.0   Listen
		    969			0.0.0.0        	0		0.0.0.0       
 	UDP		5353		0.0.0.0        	0		0.0.0.0       
 	UDP		12345		127.0.0.1      	0		0.0.0.0       
 	UDP		12346		127.0.0.1      	0		0.0.0.0       
 	UDP		5353		0.0.0.0        	0		0.0.0.0       
 	TCP		2323		0.0.0.0        	1		0.0.0.0   Listen
	TCP		10000		0.0.0.0        	1		0.0.0.0   Listen
	UDP		1900		239.255.255.250	0		0.0.0.0        
	UDP		49158		0.0.0.0        	0		0.0.0.0        
	TCP		8081		0.0.0.0        	1		0.0.0.0   Listen

#### `printenv` [group] [encrypted]

Displays all the environment variables in the specified environment variable group. If no environment variable group is specified, the environment variables of the default group are displayed.

Set the optional encrypted parameter to true if the environment variables are encrypted. 

	[jphAir.local]$ printenv
	FW_VER=0.99.7
	ELEMENT_SHELL=1
	UUID=25FB6A35-95CB-1A04-82EE-000000000000

#### `pwd`

Displays the working directory.

	[jphAir.local]$ pwd
	/k1

#### `quit`

Quits all currently running applications.

#### `reboot` [mode]

Restarts Kinoma Element.

If "mode" is true, the device reboots immediately without tearing down the current running services.

#### `reconnect` mode

If "mode" is 1, Kinoma Element attempts to reconnect to the current Wi-F access point. If "mode" is 2, Kinoma Element disconnects from the Wi-Fi access point and enters uAP (Micro Access Point) mode.

#### `report`

Display memory usage statistics of the JavaScript virtual machine.

	[jphAir.local]$ report
	=== heap info ===
	malloc: 29360 free, 365 allocations, 26240 biggest block, 40928 by Fsk
	heap2: 0x126740, 0x151a80, 176960 remains
	heap3: 0x2001f7f4, 12 remains
	===
	# Chunk allocation: reserved 26432 used 24304 peak 26416 bytes, 1 blocks
	# Slot allocation: reserved 78832 used 72672 peak 78416 bytes, 0 free

Note: the Kinoma Element Simulator displays only the last two lines, Chunk and Slot allocation.

#### `rename` from to

Renames the file specified by "from" to the name "to" in the working directory.

#### `rmdir` path

Deletes the directory specified by "path".

#### `rm` path

Deletes the file specified by "path".

#### `saveenv`

Writes the default environment variables group to storage.

#### `scan` [flush]

Performs a scan for Wi-Fi access points visible to Kinoma Element and displays results. If the "flush" parameter is missing, the scan results are from the Kinoma Element Wi-Fi access point cache; if it is set to true, the cache is emptied and a full re-scan is performed.

#### `setenv` name value

Set the environment variable "name" to "value" in the default environment variables group.

#### `shutdown` [force]

Turns off Kinoma Element. If "force" is present and set to true, Kinoma Element is immediately restarted without first terminating any active applications or network services.

#### `timestamp`

Display the timestamp of the Kinoma Element firmware build.

#### `unsetenv` name

Removes the environment variable "name" from the default environment variables group.

#### `update` [target] [do not update]

Begins the Kinoma Element firmware update process. This command does not check to see if an update is needed, so it always performs an update to the latest firmware.

If the "target" and "do not update" parameters are optional. The "target" parameter may be set to `ELEMENT_FIRMWARE_SMOKE`, `ELEMENT_FIRMWARE_QA` or `ELEMENT_FIRMWARE_RELEASE`. For most developers, `ELEMENT_FIRMWARE_RELEASE` is the recommended value.

If the "do not update" parameter is set to true, the firmware files are downloaded but not installed. This is primarily useful for debugging the firmware update process.

#### `version`

Displays the firmware version number and the timestamp of the Kinoma Element firmware build.

	[jphAir.local]$ version 0.99.7 (Wed Dec 31 1969 15:59:59 GMT-0800 (PST))

#### `xsbug` [host]

Connect to the stand-alone xsbug JavaScript debugger. This is useful in advanced debugging scenarios, but is not necessary when working with the Kinoma Code IDE.

If "host" is not provided, the environment variable XSBUG_HOST from the default environment variable group is used.

	xsbug 10.0.1.69:5003 


<!-- end include [/Users/hoddie/Desktop/Element docs - Markdown/xm_cli_reference.md] -->



# To Do

Pins

CoAP

MQTT

<!--

Crypto

LED (internal)

Notes:

[RI] Note for myself: uuid is stored in the environment file, so once it's set it wont' be changed unless someone erases it expicitly. The MAC address is a part of it so it might look weird to have the same MAC address for K5 minis. Also the date is always reset to 0 so the random number and the date are possible to conflict among units. This can happen the UUID is created before the time is set and it's likely to happen as the device goes to the provisioning mode at the first use.

HTTP Client and Server have a number of features to be implemented....

-->
