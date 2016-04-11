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
# Kinoma Documentation Overview

Kinoma Code is a tool to develop applications for Kinoma Create and Kinoma Element and it includes documentation for each. Most of the documents apply to one device or the other.

## [Programmer's Guide to Kinoma Element](../element/)

The Programmer's Guide to Kinoma Element is the primary technical document for developers working on Kinoma Element. The first section introduces the programming model and other inmportant information about developing JavaScript software on a resource constrained device. The second section is an API reference describing the objects, modules, and functions built-into Kinoma Element. It also includes a command line reference for working with Kinoma Element over telnet and USB.

## [KinomaJS Overview](../overview/)

The KinomaJS application framework is the foundation for applications on Kinoma Create. KinomaJS is a rich framework, with comprehensive support for structuring the application architecure, building animated user interfaces, and working with network services. This overview document introduces the key concepts used in KinomaJS, and provides examples in JavaScript and XML formats.

> Note: KinomaJS is not available for Kinoma Element.

## [KinomaJS JavaScript Reference](../javascript/)

This document is the primary reference for working with KinomaJS in JavaScript. It describes each object, function, and property in KinomaJS.

## [Introducing KinomaJS Dictionary-based Constructors and Templates](../introducing-kinomajs-dictionary-based-constructors-and-templates/)

This tech note introduces dictionary-based constructors and templates to build KinomaJS content, container, skin and style objects. Dictionaries is to simplify the coding of KinomaJS applications in JavaScript. The templates provide mechanisms similar to those provided by XML elements and attributes of KinomaJS XML documents.

## [Using JavaScript 6th Edition Features in KinomaJS](../using-javascript-6th-edition-features-in-kinomajs/)

JavaScript 6th Edition, often referred to by its technical abbreviation ES6, edition brings many new features to the JavaScript language. KinomaJS uses some of these new features, including Classes, Promises, and Generators. This Tech Note describes how to use these new capabilities in KinomaJS.

## [Programming with Hardware Pins](../pins/)

This document describes how to program Kinoma Create’s hardware pins, and serves as a reference to the pin types supported by Kinoma Create.

## [CoAP Client and Server in KinomaJS](../coap-client-and-server-in-kinomajs/)

This document introduces the [CoAP](https://en.wikipedia.org/wiki/Constrained_Application_Protocol) network protocol implementation for KinomaJS. CoAP is a lightweight protocol designed for lightweight devices, modeled on HTTP but running over UDP making it remarkably fast.

## [Embedding an HTTP Server in Your Application](../embedding-an-http-server-in-your-application/)

This document shows how to integrate an HTTP or HTTPS server into a KinomaJS application, including how to use custom certificates.

## [Using DIAL to Launch Apps Remotely](../using-dial-to-launch-apps-remotely/)

The DIAL protocol is a great tool to connect a mobile app to an IoT project running on Kinoma Create. This Tech Note introduces the DIAL protocol, how to discover DIAL-compatible devices using Net Scanner, and how to add DIAL support to a KinomaJS app.

## [Pin Simulators](../pins-simulators/)

This Tech Note explains how to build your own pins simulators, both data-driven and fully custom. Pins simulators enable you to develop your Kinoma Create application entirely on your computer by simulating the inputs and outputs of one or more hardware modules.

## [Accessing Files from KinomaJS](../files-api/)

This technical note describes the Files object available to applications built using KinomaJS. The Files module provides access to the file system of the host device.

> Note: Kinoma Element also implements a Files module with a subset of the functionality.

## [KinomaJS XML Reference](../xml/)

This document is the primary reference for working with KinomaJS from XML. KinomaJS supports an XML document format with embedded JavaScript. Some developers find this format more convenient.

> Note: Use of the KinomaJS XML document format is optional. The same results can be achieved using only JavaScript. In fact, Kinoma Code converts KinomaJS XML to JavaScript source code to run on Kinoma Create.

## Additional resources

In addition to the documents contained in Kinoma Element, there are other resources avaiable on the web to learn about building software for Kinoma Element and Kinoma Create.

### YouTube channel

The [KinomaTV](https://www.youtube.com/user/KinomaTV) channel on YouTube contains tutorials, news, and webcasts about Kinoma products and technology.

### Kinoma web site

The [Develop page](http://kinoma.com/develop/) of Kinoma web site is the jumping off point for all Kinoma development resources. There are projects to build, tutorials to learn from, the complete set of technical notes, and more.

### Forums

The [Kinoma Forums](http://forum.kinoma.com) are an online web forum where developers can ask questions, answer questions, and discuss development issues related to Kinoma harware and software. The Kinoma engineering team regularly monitors the Kinoma Forum, and responds directly to questions.

### Open source

The source code to KinomaJS and much of the software in built into Kinoma Element is avaiable as open source. The [KinomaJS respository on GitHub](https://github.com/Kinoma/kinomajs) contains the latest code. The Kinoma Element source code is in the [xs6/sources/mc](https://github.com/Kinoma/kinomajs/tree/master/xs6/sources/mc) directory.

### Hardware design files

The hardware design files for Kinoma Create [PCB, BOM](https://github.com/Kinoma/kinoma-create-pcb), and [case](https://github.com/Kinoma/kinoma-create-case) are available as open source.  The hardware design files for [Kinoma Element](https://github.com/Kinoma/Kinoma-Element-Open-source-hardware-1st-Generation) are also available.
