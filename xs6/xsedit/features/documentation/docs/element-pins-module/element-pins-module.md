<!-- Version: 160822-CR / Primary author: Lizzie Prader / Last reviewed: August 2016 

Applications can configure and interact with off-the-shelf sensors on Kinoma Element using JavaScript modules called BLLs. The Pins module provides an API for communicating with BLLs, as this document demonstrates.
-->

#Using the Pins Module to Interact with Sensors on Kinoma Element

Kinoma Element offers the ability to configure and interact with a variety of off-the-shelf sensors using JavaScript modules called *BLLs.* The Pins module provides a single API to communicate with the BLLs of the local device and remote devices.

See also the document [*Programming with Hardware Pins for Kinoma Element*](../element-bll/). If you would like to take a closer look at the Pins module for Kinoma Element, you can find it [here](https://github.com/Kinoma/kinomajs/tree/master/xs6/sources/mc/extensions/pins) in Kinoma's GitHub repository.


##Adding the Pins Module to Your Project

The Pins module is built into Kinoma Element's application framework, so all you have to do to use it is import the `pins` object from it in your application.

```
import Pins from "pins";
```

##Configuring Pins

To configure pins, your application must call `Pins.configure` and pass in a `pins` object that defines the type of pins it uses and which BLL(s) they correspond to. You can also pass in a callback to be invoked when the configuration has completed, whether successful or not.

```
Pins.configure({
   led: {
      require: "Digital",
      pins: {
         ground: {pin: 1, type: "Ground"},
         digital: {pin: 2, direction: "output"},
      }
   },
   button: {
      require: "Digital",
      pins: {
         power: {pin: 6, voltage: 3.3, type: "Power"},
         ground: {pin: 7, type: "Ground"},
         digital: {pin: 8, direction: "input"},
      }
   },         
}, function(success) {
   if (!success) trace("Failed to configure\n");
});
```

##Calling BLL Functions

Once configuration is complete, you can address the BLL through calls to `Pins.invoke` and `Pins.repeat`.

###Invoke

To call a BLL function, use:

`Pins.invoke(path, object, callback)`

- `path` -- The path of the BLL.

- `object` -- *(Optional)* The argument of the BLL function you are calling.

- `callback` -- *(Optional)* The function to be invoked when the call has completed. If nothing is returned by the BLL function, the callback will not be invoked.

The `object` argument is sometimes not required, as in this example:

```
Pins.invoke("/button/read", function(result) {
   if (result) {
      trace("Button on.\n");
   } else {
      trace("Button off.\n");
   }
});
```
	
The callback is always optional. For example:

```
Pins.invoke("/led/write", 1);
```

`Pins.invoke` can also be used to retrieve the active configuration by using the path `configuration`.

```
Pins.invoke("configuration", function(configuration) {
   trace("Configuration: " + JSON.stringify(configuration, null, 4) + "\n")
});
```

Given the configuration shown earlier, this call will print the following to the console:

```console
Configuration: {
    "led":{
        "require":"Digital",
        "pins":{
            "ground":{
                "pin":1,
                "type":"Ground"
            },
            "digital":{
                "pin":2,
                "direction":"output",
                "type":"Digital"
            }
        }
    },
    "button":{
        "require":"Digital",
        "pins":{
            "power":{
                "pin":6,
                "voltage":3.3,
                "type":"Power"
            },
            "ground":{
                "pin":7,
                "type":"Ground"
            },
            "digital":{
                "pin":8,
                "direction":"input",
                "type":"Digital"
            }
        }
    }
}
```

###Repeat

If you want to make repeated calls to a BLL function, you can use:

`Pins.repeat(path, interval, object, callback)`

- `path` -- The path of the BLL.

- `object` -- *(Optional)* The argument of the BLL function you are calling.

- `interval` -- The amount of time (in milliseconds) to wait between calls.

- `callback` -- *(Optional)* The function to be invoked when a call has completed. If nothing is returned by the BLL function, the callback will not be invoked.

The following example reads a button every 500 milliseconds and traces whether it is being pressed or not.

```
Pins.repeat("/button/read", 500, function(result) {
   if (result) {
      trace("Button on.\n");
   } else {
      trace("Button off.\n");
   }
});
```

The timer feature of the Hardware Pins service is supported: if you specify a pin name as a string in place of the interval, `Pins.repeat` will use an interrupt-style callback rather than periodic polling. This will use less CPU time and provide lower latencies.

```
Pins.repeat("/button/read", "digital", function(result) {
   if (result) {
      Pins.invoke("/led/write", 1);
   } else {
      Pins.invoke("/led/write", 0);
   }
});	
```

`Pins.repeat` returns an object that contains a `close` function to end the repeat.

```
var buttonReader = Pins.repeat("/button/read", 500, function(result) {
   if (result) {
      trace("Button on.\n");
   } else {
      trace("Button off.\n");
   }
});
buttonReader.close();
```

##Closing Connections to Pins

When the host application closes, you may want to close the connection to a BLL. If a `close` function is defined in a BLL, it is called automatically when the host application exits. It can be called at any other time using `Pins.close`.

##Sharing Pins Across Devices

Only a BLL accesses hardware pins directly, but the Pins module simplifies the code required to make remote procedure calls to a BLL from another device on the same network. This enables you to easily share over the network the hardware capabilities of your Kinoma Element. For example, you can attach a light, motor, or other sensor to your Kinoma Element and write an application for Kinoma Create, an iOS/Android phone, or another Kinoma Element that controls it.

Once a pin configuration has been activated, you can share the pin over the network by calling `Pins.share`.

Because there are many different ways to provide network access to hardware pins, the Pins module uses other modules to bind the protocols to the pins. WebSockets and HTTP are currently supported.


```
Pins.share("ws");
Pins.share("http");
```

When configuration information is needed, an object is passed in place of the protocol strings. For example, you can specify a port number.

```
Pins.share({type: "ws", port: 5432});
```
	
You can share pins over several protocols at the same time by passing in an array.

```
Pins.share(["ws", "http"]);
Pins.share(["http", {type: "ws", port: 5432}]);
```
	
To end pins sharing, call `Pins.share` with no arguments.

```
Pins.share();
```

##Connecting to Remote Pins

###Connect

To work with remote pins, you must know the URL of the pins, which can be discovered from the Zeroconf advertisement or through application defined means. Once the URL is known, `Pins.connect` establishes a connection to the pins.

```
var httpPins = Pins.connect("http", {url: "http://10.0.1.3:80/"});
var wsPins = Pins.connect("ws", {url: "http://10.0.1.3:5760/pins/"});
```

`Pins.connect` may be passed the `connectionDescription` object found using `Pins.discover`; the next section goes over this in detail. When `connectionDescription` contains more than one URL, the implementation of `Pins.connect` selects one to use for communication.

```
var aConnection = Pins.connect(connectionDescription);
```

The object returned by `Pins.connect` implements the `invoke`, `repeat`, and `close` functions that are used to communicate with local pins.

```
httpPins.invoke("/led/write", 1);
	
wsPins.repeat("/button/read", 50, function(result) {
   if (result) {
      trace("Button on.\n");
   } else {
      trace("Button off.\n");
   }
});

httpPins.invoke("configuration", function(config) {
   trace("Remote configuration: " + JSON.stringify(config, null, 4) + "\n");
});

wsPins.close();
```
	
###Discover

Following the style of KinomaJS application sharing, the dictionary can request that the shared pins be advertised using Zeroconf. The name (`pins-share-sample` in this case) can be any string and is used to identify discovered applications.


```
Pins.share("ws", {zeroconf: true, name: "pins-share-sample"});
```

The dictionary can optionally contain a `uuid` property. If none is provided, the value of `application.uuid` is used.

```
Pins.share("ws", {zeroconf: true, name: "pins-share-sample", uuid: x});
```

Remote pins are then discovered by creating a discovery instance and passing function callbacks for when remote pins are found and lost. The callbacks are passed a `connectionDescription` object containing information about the remote pins. The `connectionDescription` object can be passed directly to `Pins.connect` to establish a connection to the remote pins.

```
var discoveryInstance = Pins.discover(function(connectionDesc) {
   trace("Found "+connectionDesc.name+"\n");
   if (connectionDesc.name == "pins-share-sample") {
      trace("Connecting to shared pins\n");
      wsPins = Pins.connect(connectionDesc);
   }
}, function(connectionDesc) {
   trace("Lost connection to "+connectionDesc.name+"\n")
});
```

The connection description contains:

- The name of the object, intended for display to the user

- The UUID of the remote pins

- An array of URLs to access the remote pins

- An array of BLLs that are active on the remote pins

###DNS-SD

The `dns-sd` command-line tool shows the Zeroconf advertisements for pins shared using WebSockets and HTTP.

For example, suppose you have the following program running on the Kinoma Element simulator:

```
import Pins from "pins";
Pins.configure({
   led: {
      require: "Digital",
      pins: {
         digital: { pin: 2, direction: "output" },
      }
   },
}, function(success) {
   if (success) {
      sharedPins = Pins.share("http", {zeroconf: true, name: "YOUR_NAME_HERE"});
   }	
});
```
	
If you are on a Mac you, can then use the following command in Terminal to check that your service is discoverable:

```console
dns-sd -B _kinoma_pins._tcp. local
```
	
If your app is being advertised properly, you will see something like this:


```console
Browsing for _kinoma_pins._tcp..local
DATE: ---Tue 12 Apr 2016---
16:34:23.093  ...STARTING...
Timestamp     A/R    Flags  if Domain           Service Type         Instance Name
16:34:23.094  Add        2   4 local.           _kinoma_pins._tcp.   YOUR_NAME_HERE
```

You can also look up and display information about the service using the instance name.

```console
dns-sd -L "YOUR_NAME_HERE" _kinoma_pins._tcp. local
```
	
The output will look something like this:

```console
Lookup YOUR_NAME_HERE._kinoma_pins._tcp..local
DATE: ---Tue 12 Apr 2016---
16:20:38.891  ...STARTING...
16:20:39.049  YOUR_NAME_HERE._kinoma_pins._tcp.local. can be reached at Lizzies-MacBook-Air.local.:8080 (interface 4)
bll=Digital uuid=c000b9d6-dcd6-0100-bd17-e0accb725fec _http=http://\*:10001/
```

Note:

- The `TXT` record contains the URL for each protocol--HTTP in this case. 

- The name is the value of the protocol prefixed by an underscore. 

- In the URL, the IP address is replaced by an asterisk. 

- The `TXT` record also contains the UUID of the advertising application and a list of BLLs in the active configuration. This enables clients that operate with specific BLLs to quickly identify which shared pins they can communicate with.

##Advanced Feature: Pin Muxing

The current pin muxing is retrieved using `Pins.invoke` with a path of `getPinMux`.

```
// For local pins:
Pins.invoke("getPinMux", function(mux) {
   trace(JSON.stringify(mux, null, 4));
});

// For remote pins:
wsPins.invoke("getPinMux", function(mux) {
   trace(JSON.stringify(mux, null, 4));
});
```

For example, suppose the `pins` object passed into `Pins.configure` was:

```
{
   ground: {pin: 1, type: "Ground"},
   led: {pin: 2, type: "Digital", direction: "output"},
   power: {pin: 6, type: "Power", voltage: 3.3 },
   ground2: {pin: 7, type: "Ground"},
   button: {pin: 8, type: "Digital", direction: "input"}
}
```
		
The pin muxing returned would be:

```console
{
   "leftPins":[2,5,0,0,0,1,4,2],
   "rightPins":[0,0,0,0,0,0,0,0],
   "leftVoltage":3.3,
   "rightVoltage":3.3
}
```
	 
The pin muxing can be set using `Pins.invoke` with a path of `setPinMux`.

```
// For local pins:
Pins.invoke("setPinMux", {
   leftVoltage: 3.3, rightVoltage: 3.3,
   leftPins: [1, 3, 0, 0, 0, 0, 0, 0],
   rightPins: [0, 0, 0, 0, 0, 0, 0, 0]
});

// For remote pins:
wsPins.invoke("setPinMux", {
   leftVoltage: 3.3, rightVoltage: 3.3,
   leftPins: [1, 3, 0, 0, 0, 0, 0, 0],
   rightPins: [0, 0, 0, 0, 0, 0, 0, 0]
});
```
		
Here is section what the numbers in the returned arrays represent:

- 0 = Disconnected
- 1 = 3.3V Power
- 2 = Ground
- 3 = Analog In
- 4 = Digital In
- 5 = Digital Out
- 6 = I<sup>C</sup> Clock
- 7 = I<sup>C</sup> Data
- 8 = Serial RX
- 9 = Serial TX
- 10 = PWM

##Pins Sharing Example

If you want to follow along with this example, create two Kinoma Element projects in [Kinoma Code](http://www.kinoma.com/develop/code). One will be for a Kinoma Element attached to a [simple LED](https://www.sparkfun.com/products/531); it will use `Pins.share`. The other will be for an Element attached to a [button](https://www.adafruit.com/products/1010) used to control the LED; it will use `Pins.connect`.

Add the following code to the `main.js` file in one of the projects.

```
import Pins from "pins";
	
var main = {
   onLaunch(){
      Pins.configure({
         led: {
            require: "Digital",
            pins: {
               ground: {pin: 7, type: "Ground"},
               digital: {pin: 8, direction: "output"},
            }
         }
      }, function(success) {
         if (success) {
            Pins.share("http", {zeroconf: true, name: "pins-share-sample"});
            trace("Configured and shared pins.\n");
         } else {
            trace("Failed to configure\n");
         }	
      });
   }
};
	
export default main;
```

This code may look familiar--it uses the built-in digital BLL to configure an LED. If the configuration is successful, it shares the BLL using HTTP and advertises it using Zeroconf so that it can be discovered by other applications on the same network.

In this example, the LED will not be controllable by the application that configures it; the controls will be in the second application. Switch over to the `main.js` file for the second app and add this code:

```
import Pins from "pins";
var httpPins;
```

The variable `httpPins` will be assigned to the object returned by `Pins.connect` later.
	
Now we define the application's behavior. When the application is launched, it must do these two things:

1. Configure the button attached to the device.

2. Discover the other Kinoma Element and establish a connection to it.

Then you can toggle the light on and off based on whether the button is being pressed or not. Copy this code into your `main.js`:

```	
var main = {
   onLaunch(){
      Pins.configure({
         button: {
            require: "Digital", // Uses the built-in Digital BLL like the other application
            pins: {
               power: {pin: 6, voltage: 3.3, type: "Power"},
               ground: {pin: 8, type: "Ground"},
               digital: {pin: 7, direction: "input"},
            }
         },         
      }, function(success) {
         if (success) {
            Pins.discover(function(connectionDesc) {
               trace("Found "+connectionDesc.name+"\n");
               /* Check the name of every discovered application.
               When "pins-share-sample" (the name passed into the other application) 
               is found, connect to it store the object returned by Pins.connect
               in httpPins so that you can make calls to the remote BLL */
               if (connectionDesc.name == "pins-share-sample") {
                  httpPins = Pins.connect(connectionDesc);
                  Pins.repeat("/button/read", "digital" , function(result) {
                     if (result) {
                        if (httpPins) httpPins.invoke("/led/write", 1);
                     } else {
                        if (httpPins) httpPins.invoke("/led/write", 0);
                     }
                  });
               }
            }, function(connectionDesc) {
               trace("Lost connection to "+JSON.stringify(connectionDesc, null, 4)+"\n");
               if (connectionDesc.name == "pins-share-sample") {
                  httpPins = undefined;
               }
            });
         } else {
            trace("Failed to configure pins.\n");
         }
      });
   }
};
	
export default main;
```

The video in Figure 1 shows the two applications running side by side.

**Figure 1.** Running the Pins Sharing Applications

<iframe width="100%" height="500" src="https://www.youtube.com/embed/tgxTQvyuH0Q?rel=0&amp;vq=hd1080" frameborder="0" allowfullscreen>
<p><a href="https://www.youtube.com/embed/tgxTQvyuH0Q?rel=0&amp;vq=hd1080">Kinoma Element Pins Sharing video</a></p>
</iframe>
