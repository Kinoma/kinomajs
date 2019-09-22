# KinomaJS Open Source README

This repository contains the source code and documentation of the Kinoma software platform. For a high-level overview of what you'll find here, see [this document](./notes/introducing-the-kinomajs-open-source-implementation-and-faq/introducing-the-kinomajs-open-source-implementation-and-faq.md). Links to specific sections of this repository are provided below.

***

## Build system

The build system for KinomaJS open source software supports building for Mac OS X, iOS, Android, Linux GTK, Embedded Linux, and Windows targets. The build system runs on Mac OS X (Yosemite and later), Windows (7+) and Linux (Ubuntu 14.04). 

<p align="center"> 
<img src="http://kinoma.com/img/github/osx.png" height="75" alt=""/> &nbsp;&nbsp;
<img src="http://kinoma.com/img/github/iOS.png" height="75" alt=""/> &nbsp;&nbsp;
<img src="http://kinoma.com/img/github/android.png" height="75" alt=""/> &nbsp;&nbsp;&nbsp;
<img src="http://kinoma.com/img/github/linux.png" height="75" alt=""/> &nbsp;&nbsp;
<img src="http://kinoma.com/img/github/windows.png" height="75" alt=""/>   
</p>

### Documentation

- [Build instructions](./build/README.md): describes the host build environment required to build KinomaJS for each supported target platform and the commands used to build an application

<!-- 
- [Building iOS apps](./build/ios/kinomajs-ios/kinomajs-ios.md): an in-depth explanation of how to build iOS apps using KinomaJS
- [Kinoma Create Command Line Tools](./build/kinoma-create-command-line-tool/kinoma-create-command-line-tool.md): discusses the Kinoma Create command-line tool **kct6**, which is used to install, delete, and run JavaScript applications on Kinoma Create and Kinoma Element
-->

***

## XS6

Kinoma's XS6 library implements a JavaScript virtual machine optimized for devices with limited resources. This virtual machine conforms to the 6th edition of the ECMAScript specification (ECMAScript 2015).

The JavaScript application frameworks (discussed later in this document) used on Kinoma Create, Kinoma Element, desktops, and mobile devices are designed to run efficiently on top of XS6.

### Documentation
- [XS Overview](./xs6/xsedit/features/documentation/docs/xs/xs.md) provides an introduction to the XS library.
- [XS in C](./xs6/xsedit/features/documentation/docs/xs-in-c/xs-in-c.md) documents the C interface used to extend XS beyond the ECMAScript 2015 standard.
- [ES6 Compatibility Test Results for Kinoma’s XS6 JavaScript Engine](http://kinoma.com/develop/documentation/js6/) describes XS6's compliance with the ECMAScript 2015 standard and way in which XS6 differs from other JavaScript engines.

### Source code

- The [sources](./xs6/sources) folder contains the native implementations of ES6 features.
- The [extensions](./xs6/extensions) folder contains important platform features that are not standard to ECMAScript, such as cryptography.

### `xsbug`

`xsbug` is a full-featured debugger for developers building standalone KinomaJS applications, shells, and tools on all devices running XS. `xsbug` supports over-the-air debugging of concurrent targets running on different devices. Similar to other debuggers, `xsbug` supports setting breakpoints, browsing source code, the call stack and variables. For more information about xsbug refer to the [`xsbug` documentation](./xs6/xsbug6/doc/notes.md). Instructions for building xsbug are included in the platform-specific [build instructions](./build/README.md).

***

## KinomaJS application framework

<p align="center"> <img src="http://kinoma.com/media/press-images/img/KinomaJS-sm.jpg" height="150" alt=""/></p>

The KinomaJS application framework can be used to deliver applications on a wide range of consumer electronic products, including Kinoma Create, iOS and Android devices, and desktops. The primary programming interface to KinomaJS is a JavaScript API of global constructors, functions, and objects that define the containment hierarchy, appearance, behavior, and flow of applications and shells. 

### Documentation highlights

- The [KinomaJS Overview](./xs6/xsedit/features/documentation/docs/overview/overview.md) provides a conceptual overview of KinomaJS that will put the documentation and tutorials listed below in context. **Read this first!**
- The [KinomaJS JavaScript Reference](./xs6/xsedit/features/documentation/docs/javascript/javascript.md) provides details on the objects that define the KinomaJS API.
- The [KinomaJS Tutorials](./kinoma/kpr/notes/tutorials/kinomajs-tutorials.md) provide sample code and more detailed explanations of the objects defined in the KinomaJS JavaScript Reference.
- Many of the sample applications in the [KPR-examples repository](https://github.com/Kinoma/KPR-examples) are built using KinomaJS.

> Note: These resources are also available in the [Develop Tab](http://kinoma.com/develop/) on the Kinoma website


### Source code and additional documentation

The KinomaJS source code is located in [./kinoma/kpr](./kinoma/kpr/). For an extended description of what is there and links to relevant documentation and tutorials, see [this document](./notes/kinomajs.md).

***

## Kinoma Create resources

<p align="center"> <img src="http://kinoma.com/media/press-images/img/Kinoma-Create_facing-left_sm.jpg" height="150" alt=""/></p>

Kinoma Create uses the KinomaJS application framework, so all source code, tutorials, and documentation listed above can be used as references for Kinoma Create developers. 

The following resources are specific to Kinoma Create.

### Programming with hardware 

- [Getting Started with Hardware](http://kinoma.com/develop/documentation/getting-started-with-hardware/): an overview and samples for the hardware protocols supported by Kinoma Create.
- [Programming with Hardware Pins for Kinoma Create](http://kinoma.com/develop/documentation/create-pins-module/): describes how to program Kinoma Create’s hardware pins using BLLs (JavaScript modules that interact with hardware)
- [Kinoma Create Pins module](./xs6/xsedit/features/documentation/docs/pins/pins.md): documentation on the Kinoma Create Pins module, the API to communicate with BLLs
- [Building Your Own BLLs](http://kinoma.com/develop/documentation/building-a-bll/): defines and describes the rules for building your own custom BLLs
- [Pins simulators](./kinoma/kpr/notes/tech-notes/pins-simulators-note/pins-simulators.md): explains how to build your own pins simulators, which enable you to develop applications entirely on your computer by simulating the inputs and outputs of hardware modules

### Device management

[./kinoma/kpr/notes/create-notes/](./kinoma/kpr/notes/create-notes/) contains the following references to help developers set up and properly use Kinoma Create:

- [Kinoma Create Quick Start Guide](./kinoma/kpr/notes/create-notes/create-quick-start/create-quick-start.md): instructions on how to set up and your device and run code for the first time
- [Power and Charging](./kinoma/kpr/notes/create-notes/power-and-charging/power-and-charging.md): full details on power source requirements for Kinoma Create
- [Power button](./kinoma/kpr/notes/create-notes/power-button/power-button.md): information about the functions of the power button on Kinoma Create 

***

## Kinoma Element resources

<p align="center"> <img src="http://kinoma.com/media/press-images/img/Kinoma-Element_facing-left_sm.jpg" height="150" alt=""/></p>

Kinoma Element's form factor and resources demand the use of a lighter application framework than Kinoma Create. It therefore does not use the KinomaJS application framework detailed above. However, Kinoma Element and Kinoma Create offer many of the same software APIs so developers will often be able to re-use their code that communicates with pins, as well as many modules they create.

### Documentation highlights

- The [Programmer's Guide to Kinoma Element](./xs6/xsedit/features/documentation/docs/element/element.md) contains essential information programmers developing for Kinoma Element need to know and documentation for the Kinoma Element software modules and command line interface.
- The [Kinoma Element Quick Start Guide](./xs6/xsedit/features/documentation/docs/element-quick-start/element-quick-start.md) is a guide to setting up your device and quickly getting started writing apps.

### Programming with hardware 

- [Programming with Hardware Pins for Kinoma Element](./xs6/xsedit/features/documentation/docs/element-bll/element-bll.md): describes how to program Kinoma Element's hardware pins using BLLs
- [Kinoma Element Pins module](./xs6/xsedit/features/documentation/docs/element-pins-module/element-pins-module.md): documentation on the Kinoma Element Pins module, the API to communicate with BLLs
- [Building Your Own BLLs](http://kinoma.com/develop/documentation/building-a-bll/): defines and describes the rules for building your own custom BLLs

### Source code

The source code for Kinoma Element is located in [./xs6/sources/mc/](./xs6/sources/mc/). 

***

## Kinoma Code

<p align="center"> <img src="http://kinoma.com/media/press-images/img/Kinoma-Code_sm.jpg" height="150" alt=""/></p>

Kinoma Code is our IDE for scripting apps for IoT devices. You can download it from [the Kinoma website](http://kinoma.com/develop/code) or build it by following the instructions in the [build instructions document](./build/README.md).

### Documentation

- [Kinoma Code Overview](http://kinoma.com/develop/documentation/kinoma-code-overview/): provides an overview of the features available in Kinoma Code
- [Troubleshooting Kinoma Element with Kinoma Code for Windows](http://kinoma.com/develop/documentation/element-windows-troubleshooting/): advice for getting Kinoma Code for Windows to discover your Kinoma Element

### Source code

The source for Kinoma Code (originally called xsedit) is located in [./xs6/xsedit](./xs6/xsedit).

***

## KinomaJS Blocks

<p align="center"> <img src="http://kinoma.com/media/press-images/img/KinomaJS-Blocks_sm.jpg" height="150" alt=""/></p>

KinomaJS Blocks is a visual code editor designed to help developers build starter projects for Kinoma Create and Kinoma Element. The project is built on Angular 2(RC7) and runs in a web browser. The [live version](http://kinomajsblocks.appspot.com/static/index.html) is hosted using Google App Engine, but you can modify and build it yourself by following the instructions in [this document](./tools/kinomajsblocks/README.md). 

### Documentation

- [KinomaJS Blocks documentation](./tools/kinomajsblocks/docs/blockly.md): provides information about using the KinomaJS Blocks editor and describes the functionality of each block

***

## KPL

The Kinoma Porting Layer (KPL) is the bottom of the KinomaJS stack. It is a very light portability layer, modeled, as much as practical, on POSIX. Because KinomaJS runs on a wide variety of operating systems, the goal is to isolate all direct calls to the host operating in KPL. Some RTOS hosts do not support the full ANSI C library, so it cannot even be safely assumed that functions like printf are available. To avoid surprises with the size of types, a portable-type system modeled on that used by QuickTime is used.

KinomaJS builds for Linux-based systems use KPL. For historical reasons, the iOS, Mac OS X, Android, and Windows builds do not. They rely instead on conditional compilation. It is strongly recommended that newly developed ports of the Kinoma Platform be based on KPL rather than the older method.

### Documentation

- [Kinoma Porting Layer](./core/kpl/doc/KPL.pdf): discusses how to build new ports of the Kinoma platform using the Kinoma Porting Layer, including a list of all functions required for the KPL API.

