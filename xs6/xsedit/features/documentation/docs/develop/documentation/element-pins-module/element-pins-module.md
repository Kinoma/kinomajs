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
#Using the Pins Module to Interact with Sensors on Kinoma Element

Kinoma Element offers the unique ability to configure and interact with a variety of off-the-shelf sensors using JavaScript modules. These modules are called BLLs. The Pins module provides a single API to communicate with the BLLs of the local device and remote devices.


##Adding the Pins Module to your Project
The Pins module is built into Kinoma Element's application framework, so all you have to do to use it is import the pins object from it in your application.

```
import Pins from "pins";
```
	
##Configuring pins
To configure your pins, your application must call `Pins.configure` and pass in a pins object which defines the type of pins it uses and which BLL(s) they correspond to. You can also pass in a callback to be invoked when the configuration has completed, whether successful or not.

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

Once configuration is complete, you can address the BLL through calls to `Pins.invoke()` and `Pins.repeat()`.

##Calling BLL functions

####Invoke

To call a BLL function, use `Pins.invoke(path, object, callback)` where

- **path** is the path of the BLL.
- **object** is the argument of the BLL function you are calling
- **callback** is a function to be invoked when the call has completed

The object argument is not always required. For example, you might call:

```
Pins.invoke("/button/read", function(result) {
	if (result) {
		trace("Button on.\n");
	} else {
		trace("Button off.\n");
	}
});
```
	
The callback is always optional. For example, you might call:

```
Pins.invoke("/led/write", 1);
```
	
In addition, if nothing is returned by the BLL function, the callback will not be invoked.
	
`Pins.invoke` can also be used to retrieve the active configuration by using the path "configuration".

```
Pins.invoke("configuration", function(configuration) {
	trace("Configuration: "+JSON.stringify(configuration, null, 4)+"\n")
});
```
	
Given the configuration used above, this call will return:

```
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

####Repeat
If you want to make repeated calls to a BLL function, you can use `Pins.repeat(path, interval, object, callback)` where

- **path** is the path of the BLL.
- **interval** is the amount of time (in ms) to wait between each call
- **object** is the argument of the BLL function you are calling
- **callback** is a function to be invoked when a call has completed

As with `Pins.invoke`, object and callback are optional, and if nothing is returned by the BLL function the callback will not be invoked.

The following reads a button every 500 milliseconds. and traces whether it is being pressed or not.

```
Pins.repeat("/button/read", 500, function(result) {
	if (result) {
		trace("Button on.\n");
	} else {
		trace("Button off.\n");
	}
});
```

The timer feature of the pins service is supported, by specifying a pin name as a string in place of the interval. This will use an interrupt style callback rather than periodic polling, which uses less CPU time and provides lower latencies.

```
Pins.repeat("/button/read", "digital" , function(result) {
	if (result) {
		Pins.invoke("/led/write", 1);
	} else {
		Pins.invoke("/led/write", 0);
	}
});	
```

The `Pins.repeat` function returns an object that contains a close function to end the repeat:

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

##Closing connections to pins
Finally, when the host application closes, you may wish to close the connection to a BLL. If a close function is defined in a BLL it is called automatically when the host application exits. It can be called at any other time by calling `Pins.close()`.
	

***

<!--

##Built-in BLLs
The Pins module includes BLLs for Digital, Analog, PWM, I2C, Serial, Power and Ground pins. The Digital and Analog BLLs can be used in the simulator and will create Pin Simulators automatically. All built-in BLLs can be used on an actual device, and will configure the pins as normal.

The sample in the previous section configures digital pins to use the built-in BLL **Digital.js**; you can use the built-in BLL for each hardware protocol by requiring it, which is done in the call to `Pins.configure`. 

	Pins.configure({
	    led: {
	        require: "Digital",
	        ...
	    },
	    ...       
	});
	
A few notes:

- You do not need to require the built-in Power and Ground modules to configure power and ground pins. You can simply add them into the pins object passed in, i.e.

		button: {
	        require: "Digital",
	        pins: {
				power: {pin: 59, voltage: 3.3, type: "Power"},
				ground: {pin: 60, type: "Ground"},
				digital: {pin: 61, direction: "input"},
	        }
	    }
- The names of the built-in BLLs are capitalized, i.e. **Digital.js** and **Analog.js**. However, the corresponding key passed into the pins object should be lowercase, i.e. power, ground, and digital as seen in the code above.  
- If you add your own BLL with the same name as a built-in BLL your project directory, it will override the built-in BLL. If you only need the basic functions already provided, there is no need to add additional files.

For more information on each specific protocol's built-in BLL, see the [Getting Started with Hardware](http://kinoma.com/develop/documentation/getting-started-with-hardware/) tutorials.

####Compact configuration
For applications that only use the built-in BLLs, something common in simpler projects, a simplified syntax for specifying the configuration is supported. The syntax eliminates the need for the require property by using the pin type. Using this syntax, the example from before reduces to the following:

	Pins.configure({
		ground: {pin: 51, type: "Ground"},
		led: {pin: 52, type: "Digital", direction: "output"},
		power: {pin: 59, type: "Power", voltage: 3.3 },
		ground2: {pin: 60, type: "Ground"},
		button: {pin: 61, type: "Digital", direction: "input"}
	}, function(success) {
		if (!success) trace("Failed to configure\n");
	});

Pins are accessed exactly as shown above, through the paths `/led/write` and `/button/read`, for example.

***
	
-->

##Sharing Pins Across Devices
Only a BLL accesses hardware pins directly, but the Pins module simplifies the code required to make remote procedure calls to a BLL from another device on the same network. This gives you the ability to easily share over the network the hardware capabilities of your Kinoma Element. For example, you may attach a light, motor, or other sensor to your Kinoma Element and write an application for Kinoma Create, an iOS/Android phone, or another Kinoma Element that controls it.


####Share
Once a pin configuration has been activated, it can be shared over the network by calling `Pins.share`.

**Protocols**

Because there are many different ways to provide network access to hardware pins, the Pins module uses other modules to bind the protocols to the pins. WebSockets and HTTP are currently supported. CoAP support is coming soon.

```
Pins.share("ws");
Pins.share("http");
```

<!--	Pins.share("coap"); -->
	
When configuration information is needed, an object is passed in place of the protocol strings. For example, you can specify a port number.

```
Pins.share({type: "ws", port: 5432});
```
	
Pins can be shared over several protocols at the same time by passing in an array.

```
Pins.share(["ws", "http"]);
Pins.share(["http", {type: "ws", port: 5432}]);
```

<!-- 
**PubNub**
	
You can also share using [PubNub](https://www.pubnub.com/). First you'll need to save the [KinomaJS PubNub module](https://github.com/Kinoma/KPR-examples/blob/master/pubnub/src/pubnub.js) to your src folder as **pubnub.js** and include it in your program.

	include("pubnub");

Unlike the other protocols, you must also pass in additional parameters when you call `Pins.share` to make it work. You will need to register for a PubNub account to get your own publish and subscribe keys.
	
	var PUBNUB_PUBLISH_KEY = "PUBLISH_KEY_HERE";
	var PUBNUB_SUBSCRIBE_KEY = "SUBSCRIBE_KEY_HERE";
	var PUBNUB_CHANNEL = "CHANNEL_NAME_HERE";
	
	Pins.connect("pubnub", {subKey: PUBNUB_SUBSCRIBE_KEY, pubKey: PUBNUB_PUBLISH_KEY, channelName: PUBNUB_CHANNEL});
	
Once configured, this method works the same as the protocols listed above.

-->
	
####Ending Pins Sharing
Pin sharing is ended by calling Pins.share with no arguments.

```
Pins.share();
```

##Connecting to Remote Pins

####Connect

To work to remote pins, the URL of the remote pins must be known. This may be discovered from the ZeroConf advertisement, or through application defined means. Once the URL is known, `Pins.connect` establishes a connection to the pins.

```
var httpPins = Pins.connect("http", {url: "http://10.0.1.3:80/"});
var wsPins = Pins.connect("ws", {url: "http://10.0.1.3:5760/pins/"});
```

`Pins.connect` may be passed the **connectionDescription** object found using `Pins.discover`. The next section goes over this in detail. When the **connectionDescription** contains more than one URL, the implementation of `Pins.connect` selects one to use for communication.

```
var aConnection = Pins.connect(connectionDescription);
```

The object returned by `Pins.connect` implements the same invoke, repeat, and close functions that are used to communicate with local pins.

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
	
####Discover
Following the style of KinomaJS application sharing, the dictionary can request that the shared pins be advertised using ZeroConf.

```
Pins.share("ws", {zeroconf: true, name: "pins-share-sample"});
```

The dictionary can optionally contain a uuid property. If none is provided, the value of application.uuid is used.

Remote pins are then discovered by creating a new discovery instance, and passing function callbacks for when remote pins are found and lost. The callbacks are passed a **connectionDescription** object with information about the remote pins. The **connectionDescription** object can be passed directly to `Pins.connect` to establish a connection to the remote pins.

```
var discoveryInstance = Pins.discover(function(connectionDesc) {
	trace("Found "+connectionDesc.name+"\n")
	if (connectionDesc.name == "pins-share-sample") {
		trace("Connecting to shared pins\n");
		wsPins = Pins.connect(connectionDesc);
	}
}, function(connectionDesc) {
	trace("Lost connection to "+connectionDesc.name+"\n")
});
```

The connection description contains:

- Name of the object, intended for display to the user
- UUID of remote pins
- Array of URLs to access remote pins
- Array of BLLs active on remote pins

<!--

The list of discovered devices can be filtered by their active BLLs. Passing a single BLL name discovers devices which have that BLL active.

	var discoveryInstance = new Pins.discover("led",
		function(connectionDesc) {
			trace("Found: " + connectionDesc.name + "\n");
		}
	);

Passing an array of BLL names discovers devices with one or more of those BLLs active.

	var discoveryInstance = new Pins.discover(["led", "i2c-temp"],
		function(connectionDesc) {
			trace("Found: " + connectionDesc.name + "\n");
		}
	);
-->

To end discovery, call close on the discover instance:

```
discoveryInstance.close();
```
	
***

##Advanced/Extras
####Pin Muxing
The current pin muxing is retrieved using `Pins.invoke` with a path of getPinMux.

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

For example, if the pins object passed into `Pins.configure` was:

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

```
{
	"leftPins":[2,5,0,0,0,1,4,2],
	"rightPins":[0,0,0,0,0,0,0,0],
	"leftVoltage":3.3,
	"rightVoltage":3.3
}
```
	 
The pin muxing can be set using `Pins.invoke` with a path of setPinMux.

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
		
Here is a key of what the numbers in the returned arrays represent:

- 0 = Disconnected
- 1 = 3.3V Power
- 2 = Ground
- 3 = Analog In
- 4 = Digital In
- 5 = Digital Out
- 6 = I2C Clock
- 7 = I2C Data
- 8 = Serial RX
- 9 = Serial TX
- 10 = PWM

<!--
####Metadata
BLLs may add an optional metadata export to describe their API. The metadata is modeled on Hardware Pin Simulators. The details of the metadata are outside the scope of this document.

To retrieve the metadata of a BLL, use invoke with a path of “metadata” and the name of the BLL:

	Pins.invoke("metadata/project", function(metadata) {
	});

	wsPins.invoke("metadata/project", function(metadata) {
	});

If the BLL has does not provide metadata, the value of metadata is undefined. The metadata may be retrieved both before and after a configuration has been activated.
-->

####DNS-SD
Using the DNS-SD command line tool shows the ZeroConf advertisements for pins shared using WebSockets and HTTP.

For example, say you have the following program running on the Kinoma Element simulator:

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
	
If you're on a Mac you can then use the following command in Terminal to check that your service is discoverable:

```console
dns-sd -B _kinoma_pins._tcp. local
```
	
If your app is being advertised properly, you will see something like this:

```console
Browsing for _kinoma_pins._tcp..local
DATE: ---Tue 12 Apr 2016---
16:34:23.093  ...STARTING...
Timestamp     A/R    Flags  if Domain               Service Type         Instance Name
16:34:23.094  Add        2   4 local.               _kinoma_pins._tcp.   YOUR_NAME_HERE
```

You can also look up and display information about the service using the instance name:

```console
dns-sd -L "YOUR_NAME_HERE" _kinoma_pins._tcp. local
```
	
And you will get output that looks something like this:

```console
Lookup YOUR_NAME_HERE._kinoma_pins._tcp..local
DATE: ---Tue 12 Apr 2016---
16:20:38.891  ...STARTING...
16:20:39.049  YOUR_NAME_HERE._kinoma_pins._tcp.local. can be reached at Lizzies-MacBook-Air.local.:8080 (interface 4)
bll=Digital uuid=c000b9d6-dcd6-0100-bd17-e0accb725fec _http=http://\*:10001/
```

A few notes on this:

- The TXT record contains the URL for each protocol, which is HTTP in this case. 
- The name is the value of the protocol prefixed by an underscore. 
- In the URL, the IP address is replaced with an asterisk. 
- The TXT block also contains the UUID of the advertising application, and a list of BLLs in the active configuration. This allows clients that operate with specific BLLs to quickly identify which shared pins they can communicate with.

***

##Code Tutorial

If you want to follow along with the tutorial, create two new Kinoma Element projects in [Kinoma Code](http://www.kinoma.com/develop/code). One will be attached to a [simple LED](https://www.sparkfun.com/products/531); it will use `Pins.share`. The other will control the LED using a [button](https://www.adafruit.com/products/1010); it will use `Pins.connect`.

Add this to the main.js file in one of the projects:

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

This code may look familiar--it uses the built-in digital BLL to configure an LED. If the configuration is successful, it shares the BLL using HTTP and advertises it using ZeroConf so it can be discovered by other applications on the same network.

In this sample the LED will not be controllable by the application that configures it. The controls will be in the second application. Switch over to the main.js file for the second app and add this code:

```
import Pins from "pins";
var httpPins;
```

The variable **httpPins** will be assigned to the object returned by `Pins.connect` later.
	
Now for the application's behavior. When launched, you need to do 2 things:

1. Configure the button attached to the device
2. Discover the other Kinoma Element and establish a connection to it

Then you can toggle the light on and off based on whether the button is being pressed or not. Copy this code into your main.js:

```	
var main = {
	onLaunch(){
		Pins.configure({
		    button: {
		        require: "Digital", // uses the built-in Digital BLL like the other application
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
                 is found, connect to it store the object returned by 
                 Pins.connect in httpPins so you can make calls to the remote BLL*/
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
		            trace("Lost connection to "+JSON.stringify(connectionDesc, null, 4)+"\n")
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

Here are the two applications running side by side:

<iframe width="100%" height="500" src="https://www.youtube.com/embed/tgxTQvyuH0Q?rel=0&amp;vq=hd1080" frameborder="0" allowfullscreen></iframe>

***

##Further Reading

If you'd like to take a closer look at the Pins module for Kinoma Element, you can find it in our [GitHub repository](https://github.com/Kinoma/kinomajs/tree/master/xs6/sources/mc/extensions/pins).
