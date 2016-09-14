<!-- This is the text (excluding cross-reference links) displayed by Kinoma Documentation Overview in Kinoma Code's Documentation activity. 
-->

#Kinoma Documentation Overview

A tool for developing applications for Kinoma Element and Kinoma Create, Kinoma Code includes documentation for both devices. Here are descriptions and links for  comprehensive documents, Tech Notes, and other helpful resources.

##Documents

###[Programmer's Guide to Kinoma Element](./element/)
For developers working on Kinoma Element, this document introduces the programming model along with other important information about developing JavaScript software on a resource-constrained device. In addition to guidance, it provides reference details for the modules, objects, and classes that make up the Kinoma Element API, and for the Kinoma Element command-line interface available over Telnet and USB. 

###[KinomaJS Overview](./overview/)
The KinomaJS application framework is the foundation for applications on Kinoma Create. KinomaJS is a rich framework, with comprehensive support for structuring the application architecture, building animated user interfaces, and working with network services. This overview document introduces the key concepts of KinomaJS and provides numerous code examples. 

>**Note:** KinomaJS is not available for Kinoma Element.

###[KinomaJS JavaScript Reference](./javascript/)
This document is the primary reference for working with KinomaJS in JavaScript; it describes each object in detail. 

###[Accessing Files from KinomaJS](./files-api/)
This document summarizes and illustrates the KinomaJS Files API, which enables applications to access the host file system to read and write files, iterate through directories, rename files, and more.

>**Note:** Kinoma Element implements a Files module with a subset of this functionality. 
 
###[KinomaJS XML Reference](./xml/)
KinomaJS supports an XML document format with embedded JavaScript, and some developers find this format more convenient. This document is the primary reference for working with KinomaJS from XML. 

>**Note:** Use of the KinomaJS XML document format is optional; the same results can be achieved using only JavaScript. (In fact, Kinoma Code converts KinomaJS XML to JavaScript source code to run on Kinoma Create.) 

###[Programming with Hardware Pins for Kinoma Create](./pins/)
This document describes how to program Kinoma Create’s hardware pins, and serves as a reference to the pin types supported by Kinoma Create. 

###[Programming with Hardware Pins for Kinoma Element](./element-bll/)
This is the same as the document described above, but for Kinoma Element.

###[Using the Pins Module to Interact with Sensors on Kinoma Element](./element-pins-module/)
Applications can configure and interact with off-the-shelf sensors on Kinoma Element using JavaScript modules called BLLs. The Pins module provides an API for communicating with BLLs, as this document demonstrates.

##Tech Notes

###[Introducing KinomaJS Dictionary-Based Constructors and Templates](./technotes/introducing-kinomajs-dictionary-based-constructors-and-templates/)
This Tech Note introduces how to use dictionary-based constructors and templates to build KinomaJS `content`, `container`, `skin`, and `style` objects. Using dictionaries helps simplify the coding of KinomaJS applications in JavaScript, and templates provide mechanisms similar to those provided by XML elements and attributes in a KinomaJS XML document. 

###[Using JavaScript 6th Edition Features in KinomaJS](./technotes/using-javascript-6th-edition-features-in-kinomajs/)
JavaScript 6th Edition (often referred to as ES6) brings many new features to the JavaScript language. We have updated KinomaJS to use some of these new features, including classes, generators, and promises. This Tech Note describes how you can use these new capabilities in KinomaJS. 

###[CoAP Client and Server in KinomaJS](./technotes/coap-client-and-server-in-kinomajs/)
Use of network connections by IoT devices is increasingly common. HTTP is a popular protocol for these connections, but there are many problems with it, such as efficiency and speed, and these factors have an impact on the device battery life. CoAP is a lightweight, fast, and reliable protocol designed to be used for constrained devices commonly found in the Internet of Things. 

###[Embedding an HTTP Server in Your Application](./technotes/embedding-an-http-server-in-your-application/)
It is often necessary for an IoT device to include an HTTP server, for setup or for retrieving information gathered by the device. A KinomaJS app can easily create an HTTP server and advertise its presence on the local network. This Tech Note describes how to start, stop, and customize your app's HTTP server.

###[Using DIAL to Launch Apps Remotely](./technotes/using-dial-to-launch-apps-remotely/)
Most IoT devices work together with a mobile app, enabling the user to monitor, configure, and control the device using a phone or tablet. The DIAL protocol created by Netflix is a great solution for connecting your mobile app to your IoT project running on Kinoma Create. This Tech Note introduces the DIAL protocol, how to discover DIAL-compatible devices using Net Scanner, and how to add DIAL support to your KinomaJS app. 

###[Pins Simulators](./technotes/pins-simulators/)
This Tech Note explains how to build your own pins simulators, both data-driven and fully custom. Pins simulators enable you to develop your Kinoma Create application entirely on your computer by simulating the inputs and outputs of one or more hardware modules.

##Other Resources
These other resources are available for learning about building software for Kinoma Element and Kinoma Create. 

###KinomaTV YouTube Channel 
The KinomaTV channel on YouTube contains tutorials, news, and webcasts about Kinoma  products and technology. 

###Kinoma Website 
The Develop page of the Kinoma website is the jumping-off point for all Kinoma development resources, including projects to build, tutorials to learn from, and the complete set of documentation and Tech Notes. 

###Kinoma Forums 
Developers can use the Kinoma Forums (divided into topics such as Kinoma Element and Kinoma Code) to ask and answer questions and discuss development issues related to Kinoma hardware and software. The Kinoma engineering team regularly monitors the Kinoma Forums and responds directly to questions. 

###Open Source 
The source code to KinomaJS and much of the software built into Kinoma Element are available as open source. The KinomaJS repository on GitHub contains the latest code; the Kinoma Element source code is in the `xs6/sources/mc` directory. 

###Hardware Design Files 
<!--From CR to KO: Please move link in the following to be behind the text "circuit boards"-->

The hardware design files for the Kinoma Create circuit boards (PCB, BOM) and case are available as open source. The hardware design files for Kinoma Element are also available.
