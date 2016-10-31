<!-- Primary author: Shotaro Uchida and Brian Friedkin / Last reviewed: October 2016 by Shotaro Uchida and Brian Friedkin
-->

# KinomaJS BLE V2 API

## About This Document

The KinomaJS BLE V2 stack provides an alternate JavaScript API that supports more GATT features compared to the original [V1 BLE API](https://github.com/Kinoma/kinomajs/blob/master/kinoma/kpr/libraries/LowPAN/src/lowpan/ble.js). Notable improvements and features include:

* Modern object-oriented and class-based JavaScript API
* ES6 Promise-based GATT client procedures
* GAP security level settings per characteristic
* User defined characteristic value parser/serializer
* GATT Reliable Write support (TBD)
* GATT Read/Write Long Value support (TBD)

This document provides details on the objects that define the KinomaJS BLE V2 API, which supports both BLE central and peripheral roles. The implementation is based on the [Bluetooth Core Specification 4.2](https://www.bluetooth.com/specifications/bluetooth-core-specification), available [here](https://www.bluetooth.org/DocMan/handlers/DownloadDoc.ashx?doc_id=286439&_ga=1.53314529.858535490.1477094023) for download.

### ES6 module usage

The `lowpan/gatt` module exports the `BLE` class as default, in addition to other common APIs.

```
import BLE, {UUID, BluetoothAddress} from "lowpan/gatt";
```

## GAP API


### BLE Central usage example
The example below shows how to instantiate a BLE instance and use the callback events to scan for and discover BLE peripherals.

```
import BLE from "lowpan/gatt";

/* BLE Instance */
let ble = new BLE();
ble.onReady = () => {
	// BLE Stack is ready to use
	ble.startScanning();
};
ble.onDiscovered = device => {
	// GAP Peripheral is discovered
};
ble.onConnected = connection => {
	// Remote connection has been established
	connection.onDisconnected = () => {
		ble.startScanning();
	};
	// Perform GATT client procedures...
};
```

### BLE Peripheral usage example
The example below shows how to instantiate a BLE instance and use the callback events to advertise and monitor remote connections.

```
import BLE from "lowpan/gatt";

/* BLE Instance */
let ble = new BLE();

let advertisingParameters = {
	scanResponse: {
		incompleteUUID16List: ["180D"]
		completeName: "Polar H7 252D9F"
	}
};

ble.onReady = () => {
	// BLE Stack is ready to use
	ble.startAdvertising(advertisingParameters);
};
ble.onConnected = connection => {
	// Remote connection has been established
	ble.stopAdvertising();
	connection.onDisconnected = () => {
		ble.startAdvertising(advertisingParameters);
	};
};
```

### BLE class

#### Constructor

#####`BLE(clearBondings)`


| | | |
| --- | --- | --- |
| `clearBondings` | `boolean` | optional |

> The GAP bonding database will be cleared when `clearBondings` is set `true`.

#####`Returns`
> A `ble` instance, an object that inherits from `BLE.prototype`


#### Properties

#####`configuration`

| | | |
| --- | --- | --- |
| `configuration` | `object` | read only |

> Shorthand object for configuring BLL. Refer to the [`init(bllName)`](#init_bllName) example below for details.

#####`server`

| | | |
| --- | --- | --- |
| `server` | `object` | read only |

> A GATT server instance of the BLE stack's `Profile` class. Refer to the [`GATT API`](#gatt-api) section for further details.  

#### Callback events

#####`onConnected(connection)`

| | | |
| --- | --- | --- |
| `connection` | `object` | |

> An instance of the `BLEConnection` class.

The callback is called when the BLE stack successfully connects to a BLE central or peripheral.

#####`onDiscovered(device)`

| | | |
| --- | --- | --- |
| `device` | `object` | |

> The `device` parameter describes the peripheral device discovered and includes following properties:

> * The `address` property is an instance of `BluetoothAddress` that represents the remote address.
> * The `rssi` property is a number that represents the RSSI (current signal strength). Units are dBm, in range -127 to +20.

> For advertising events not corresponding to *Scan Response* packets, the following additional properties are available:

> * The `directed` property is a boolean that specifies whether the event corresponds to a *Directed connectable advertising event*. This typically means that the remote peripheral device wants to connect or reconnect with the local central device.
> * The `connectable` property is a `boolean` that specifies whether or not the remote device is connectable.

> * The `scannable` property is a boolean that specifies whether or not a *Scan Response* may be available later in active scanning.
> * The `advertising` property may include the properties for advertisement. Refer to the [GAP Advertisement Properties](#gap-advertisement-properties) section for details. Note that a *Directed connectable advertising event* does not include this property.

> With active scanning, *Scan Response* events may be delivered when `scannable` is ``true``. In that case, following additional properties are available:

> * The `scanResponse` property includes the properties for scan response. Refer to the [GAP Advertisement Properties](#gap-advertisement-properties) section for details.

The callback is called when a BLE peripheral is discovered. The `device` parameter describes the peripheral device discovered.

#####`onPrivacyEnabled(address)`

| | | |
| --- | --- | --- |
| `address` | `object` | |

> An instance of `BluetoothAddress` that represents the current private resolvable address

The callback is called when GAP a privacy feature has been enabled.

#####`onReady()`

The callback is called when the BLE stack is ready to use.  

#### Common methods

#####`connect(address, parameters)`

| | | |
| --- | --- | --- |
| `address` | `object` | optional |

> An instance of `BluetoothAddress` or `null`. If `address` is provided and not `null`, the BLE stack performs the *Direct Connection Establishment* procedure, otherwise the BLE stack will perform the *Auto Connection Establishment Procedure* using the white list previously provided.

| | | |
| --- | --- | --- |
| `parameters` | `object` | optional |

> The `parameters` must include the following properties:

> * The `intervalMin` property specifies the minimum connection event interval.
   * Range: 0x0006 to 0x0C80
   * Time: N * 1.25 msec
> * The `intervalMax` property specifies the maximum connection event interval.
   * Range: 0x0006 to 0x0C80
   * Time: N * 1.25 msec
> * The `timeout` property specifies the supervision timeout for the LE link.
   * Range: 0x000A to 0x0C80
   * Time: N * 10 msec
> * The `latency` property specifies the Slave latency.
   * Range: 0x0000 to 0x01F3

Initiates an asynchronous connection to a BLE peripheral. Typically this method will be called by a BLE Central.

The `onConnected` callback is called when a connection is established with a BLE Peripheral.  

#####`disablePrivacy()`

Disables the GAP privacy feature.  

#####`enablePrivacy()`

Enables the GAP privacy feature. Refer to the *Core 4.2 Specification, Vol 3, Part C: Generic Access Profile, 10.7 Privacy Feature* for details.  

<a id="init_bllName"></a>

#####`init(bllName)`

| | | |
| --- | --- | --- |
| `bllName` | `string` |
> The `bllName` must match the BLL property name passed to `Pins.configure`.

Call once immediately after the BLL is configured to initialize the BLE stack. The `onReady` callback is called after the BLE stack sucessfully initializes.  

```
/* BLL Configuration */
Pins.configure({
	ble: ble.configuration
}, success => {
	if (!success) {
		throw new Error("Unable to configure BLE");
	}
	/* Initialize BLE Stack w/ BLL Name */
	ble.init("ble");
});
```

#####`isReady()`

Returns `true` when the BLE stack is ready to use.  

#####`startAdvertising(parameters)`

| | | |
| --- | --- | --- |
| `parameters` | `object` | optional |

>  The `parameters` may include following properties:

> * The mandatory `discoverable` property is a `boolean` and used to determine the current Discoverable mode.
   * When the `discoverable` property is `true`: The optional `limited` property is a `boolean` and indicates that the Discoverable mode is either *Limited Discoverable Mode* or *General Discoverable Mode*, otherwise *General Discoverable Mode* by default.
   * When the `discoverable` property is `false`: Discoverable mode will be set to *Non-Discoverable Mode*.
> * The mandatory `connectable` propery is a `boolean` and used to determine the current Connectable mode.
   * When the `connectable` property is `true`: The optional `address` property is an instance of `BluetoothAddress` and specifies that Connectable mode is *Directed Connectable Mode*, otherwise *Undirected Connectable Mode*.
   * When the `connectable` property is `false`: Connectable mode will be *Non-Connectable Mode*.
   * 
> * The optional `fast` property is a `boolean` that specifies whether to use the fast advertisement interval. This property is set `true` by default.
> * The optional `advertising` property may include additional properties for advertisement. Refer to the [GAP Advertisement Properties](#gap-advertisement-properties) section for details.
> * The optional `scanResponse` property may include *Scan Response* properties. Refer to the [GAP Advertisement Properties](#gap-advertisement-properties) section for details.

Starts asynchronous GAP advertising. Typically this method is called by BLE Peripheral or BLE Broadcaster roles.

The `onConnected` callback is called when this peripheral connects with a central. Refer to the *Core 4.2 Specification, Vol 3, Part C: Generic Access Profile* for further details regarding discoverable and connectable modes.  

The following example shows how to configure the BLE stack for *Limited Discoverable Mode* and *Undirected Connectable Mode*. This configuration sends *Undirected connectable advertising events* with the *LE Limited Discoverable Flag* set to one for the Flags AD type within the advertising data.

```javascript
ble.startAdvertising({
	discoverable: true,
	limited: true,
	connectable: true,
	scanResponse: {
		completeName: "Kinoma Create"
	}
});
```

The following example shows how to configure the BLE stack to *Non-Discoverable Mode* and *Non-Connectable Mode*. This configuration is typical for the BLE Broadcaster role and sends *Non-connectable undirected advertising events* without setting the *LE Limited Discoverable Flag* or the *LE General Discoverable Flag*. In this example, `scanResponse` is NOT set thus the event type will NOT be *Scannable undirected advertising events*.

```javascript
ble.startAdvertising({
	discoverable: false,
	connectable: false,
	advertising: {
		shortName: "Kinoma"
	}
});
```

#####`startScanning(parameters)`

| | | |
| --- | --- | --- |
| `parameters` | `object` | optional |

>  The `parameters` may include the following properties:

> * The mandatory `discovery` property is a `boolean` and specifies whether stack will perform the GAP discovery procedure.
   * When the `discovery` property is `true`: The optional `limited` property is a `boolean` and specifies whether or not the stack will perform the *Limited Discovery Procedure*; i.e. will only discover peripherals when the Flags AD type is present and the *LE Limited Discoverable Flag* is set to one. Otherwise the stack performs the *General Discovery Procedure* by default. Refer to *Core 4.2 Specification, Vol 3, Part C: 9.2.5* and *Core 4.2 Specification, Vol 3, Part C: 9.2.6* for details.
   * When the `discovery` property is `false`: The mandatory `active` property is a `boolean` and specifies whether or not the stack will perform active scanning.
> * The optional `duplicatesFilter` property is a `boolean` and specifies whether stack will enable filtering of duplicate advertising data. This property is set `true` by default.
> * The optional `interval` and `window` properties specify the scan interval and the scan window.
   * Range: 0x0004 to 0x4000
   * Time: N * 0.625 msec

Starts asynchronous GAP scanning. This method is typically called when the application is configured for the BLE Central or BLE Observer roles.

The `onDiscovered` callback will be called for each device discovered. If no parameters are provided, the stack performs the *General Discovery Procedure* by default and enables duplicate filtering.  

The following example shows how to perform the *Observation Procedure*. The scanner is configured to perform active scanning and all advertising packets will be reported via the `onDiscovered` callback event.

```javascript
ble.startScanning({
	discovery: false,
	active: true,
	duplicatesFilter: false
});
```

The following example shows how to perform the *General Connection Establishment Procedure*. The scanner is configured to perform passive scanning and all advertising packets will be reported via the `onDiscovered` callback event. The event callback checks if the device is connectable, and then calls the `connect` method if the device is the desired target.

```javascript
ble.onDiscovered = device => {
	/* Ignore non-connectable device */
	if (!device.connectable) {
		return;
	}
	/* Check if the address is the desired target */
	if (device.address.equals(targetAddress)) {
		/* Perform connection procedure */
		ble.connect(device.address);
	}
};
/* Perform passive scanning */
ble.startScanning({
	discovery: false,
	active: false
});
```

#####`stopAdvertising()`

Stop asynchronous GAP advertising.  

#####`stopScanning()`

Stop asynchronous GAP scanning.  

### BLE Connection class

#### Properties


#####`address`

| | | |
| --- | --- | --- |
| `address ` | `object` | read only |

> An instance of the `BluetoothAddress` class corresponding to the address of the remote device.

#####`client` 

| | | |
| --- | --- | --- |
| `client ` | `object` | read only |

> A GATT client instance of the `Profile` class dedicted to this connection. Refer to the [GATT API](#gatt-api) section for further details.

#####`encrypted`

| | | |
| --- | --- | --- |
| `encrypted ` | `boolean` | read only |

> Returns `true` when the connection link is encrypted. (i.e. LE Security Mode 1)

#####`identity`

| | | |
| --- | --- | --- |
| `identity ` | `object` | read only |

> An instance of the `BluetoothAddress` class corresponding to the identity address of the remote device, which may be identical to the address provided by the `address` property. When the remote device uses a private address, and the local device has already resolved the address, then this property contains a public or static random address.

#####`peripheral`

| | | |
| --- | --- | --- |
| `peripheral ` | `boolean` | read only |

> Returns `true` when the remote device is a BLE peripheral.  

#####`parameters`

| | | |
| --- | --- | --- |
| `parameters` | `object` | read only |
  
> Current connection parameters used by the LE link, which includes the following properties:
> * The `interval` property specifies the connection event interval.
   * Range: 0x0006 to 0x0C80
   * Time: N * 1.25 msec
> * The `timeout` property specifies the supervision timeout for the LE link.
   * Range: 0x000A to 0x0C80
   * Time: N * 10 msec
> * The `latency` property specifies the Slave latency.
   * Range: 0x0000 to 0x01F3

#####`securityInfo`

| | | |
| --- | --- | --- |
| `securityInfo ` | `object` | read only |

> Current connection security information with the following properties:

> * The `address` property is an instance of `BluetoothAddress` if the identity address information was exchanged during a pairing procedure.
> * The `keySize` property is a `number` corresponding to the current LTK size.
> * The `bonding` property is a `boolean` that specifies whether or not a bond has been created.
> * The `authenticated` property is a `boolean` that specifies whether or not the pairing procedure used an authenticated method.

> Note: the `securityInfo` property will be `null` if there is no security enabled.

#### Callback events

#####`onAuthenticationCompleted(securityChanged)`

| | | |
| --- | --- | --- |
| `securityChanged` | `boolean` | |

> Set `true` when the link security status has changed. The new security information can be read from `securityInfo` property.

The callback is called when the BLE Security Manager completes a pairing and/or bonding procedure on the connection.

#####`onAuthenticationFailed(reasonCode, pairing)`

| | | |
| --- | --- | --- |
| `reasonCode` | `number` | |

> The numeric code corresponding to the failure. Refer to the *Core 4.2 Specification, Vol 2, Part D: Error Codes* for the list of `reasonCode` values.

| | | |
| --- | --- | --- |
| `pairing` | `boolean` | |

> Set `true` when the procedure failed due to a pairing error.

The callback is called when the BLE Security Manager fails to complete a pairing and/or bonding procedure on the connection. 

#####`onDisconnected(reasonCode)`

| | | |
| --- | --- | --- |
| `reasonCode` | `number` | optional |

The callback is called when the connection is disconnected. Refer to the *Core 4.2 Specification, Vol 2, Part D: Error Codes* for the list of `reasonCode` values.  

#####`onPasskeyRequested(input)`

| | | |
| --- | --- | --- |
| `input ` | `boolean` | |

> Set `true` if the local device is an input device, i.e. user may be requested to input the passkey code. If the parameter is set `false`, then the local device is considered an output device. In this case, the device may generate and display the passkey code.  

The callback is called when the BLE Security Manager requires the client to provide a passkey code. The passkey code is provided by calling the `passkeyEntry` method. 

#####`onUpdated()`

The callback is called when the connection parameter is updated.  

#### Methods

#####`disconnect(reasonCode)`

| | | |
| --- | --- | --- |
| `reasonCode` | number | optional |

> Specifies the reason code. Refer to the *Core 4.2 Specification, Vol 2, Part D: Error Codes* for the list of `reasonCode` values. 

Terminates the connection. The `onDisconnected` callback is called when the disconnection completes.  

#####`isPeripheral()`
  
Returns `true` when the remote device is a BLE peripheral.  

#####`passkeyEntry(passkey)`

| | | |
| --- | --- | --- |
| `passkey` | numeric string | |

> Must be between "000000" and "999999".

Provides a temporary passkey used by the Security Manager during a pairing procedure.

#####`readRSSI()`
  
Reads the remote device's RSSI value. This method returns a `Promise` object.  
 
```
connection.readRSSI().then(rssi => {
	// Read RSSI value
});
```

#####`setSecurityParameter(parameter)`

| | | |
| --- | --- | --- |
| `parameter` | `object` | |

> The `parameter` object may include the following properties:

> * The `bonding` property is a `boolean` that specifies whether or not the GAP layer enables *Bondable mode*.
> * The `mitm` property is a `boolean` that specifies whether or not the SM layer uses authenticated MITM protection.
> * The `display` and `keyboard` properties are `boolean` values that specify the IO capabilities of the device.

Sets the security parameter corresponding to how the Security Manager performs pairing/bonding with the remote device.

```
connection.setSecurityParameter({
	bonding: true,      // Enable Bonding
	mitm: false,        // Disable MITM
	display: false,     // Display (for Passkey) is NOT available
	keyboard: false     // Keyboard (for Passkey) is NOT available
});
```

#####`startAuthentication()`

Starts authentication on the connection.  

#####`updateConnection(parameters, l2cap)`

| | | |
| --- | --- | --- |
| `parameters` | `object` | required |

>  The mandatory `parameters` object must include the following properties:

> * The `intervalMin` property specifies the minimum connection event interval.
   * Range: 0x0006 to 0x0C80
   * Time: N * 1.25 msec
> * The `intervalMax` property specifies the maximum connection event interval.
   * Range: 0x0006 to 0x0C80
   * Time: N * 1.25 msec
> * The `timeout` property specifies the supervision timeout for the LE link.
   * Range: 0x000A to 0x0C80
   * Time: N * 10 msec
> * The `latency` property specifies the Slave latency.
   * Range: 0x0000 to 0x01F3

| | | |
| --- | --- | --- |
| `l2cap` | `boolean` | optional |

> Explicitly configures the GAP layer to use the *L2CAP Connection Parameter Update Request*.

Starts an asynchronous connection update procedure.

```
connection.updateConnection({
	intervalMin: 0x18,
	intervalMax: 028,
	timeout: 3200,
	latency: 0,
});
```

<a id="gap-advertisement-properties"></a>

### GAP Advertisement Properties

Refer to the *Suppliment to the Bluetooth Core Specification v6, Part A: Data Types Specifications* for a description of the data types.

#####`incompleteUUID16List`

An array of strings corresponding to *Incomplete List of 16-bit Service UUIDs*.

#####`completeUUID16List`

An array of strings corresponding to *Complete List of 16-bit Service UUIDs*.

#####`incompleteUUID128List`

An array of strings corresponding to *Incomplete List of 128-bit Service UUIDs*.
  
#####`completeUUID128List`

An array of strings corresponding to *Complete List of 128-bit Service UUIDs*.

#####`shortName`
 
A string corresponding to the *Shortened Local Name*.

#####`completeName`

A string corresponding to the *Complete Local Name*.

#####`flags`

A number corresponding to the *Flags*.

#####`manufacturerSpecific`

An object corresponding to the *Manufacturer Specific Data* with the following properties:

 * The `identifier` property is a number corresponding to the *Company Identifier Code*.
 * The `data` property is an array of numbers corresponding to additional manufacturer specific data.

#####`txPowerLevel`

A number corresponding to the *TX Power Level*.  

#####`connectionInterval`  

An object corresponding to the *Slave Connection Interval Range* with the following properties:

 * The `intervalMin` property is a number corresponding to the minimum connection interval value.
 * The `intervalMax` property is a number corresponding to the maximum connection interval value.

#####`solicitationUUID16List`

An array of strings corresponding to the *List of 16 bit Service Solicitation UUIDs*.  

#####`solicitationUUID128List`

An array of strings corresponding to the *List of 128 bit Service Solicitation UUIDs*.  

#####`serviceDataUUID16`

An object corresponding to the *Service Data - 16 bit UUID* with the following properties:

 * The `uuid` property is a string corresponding to the 16-bit Service UUID.
 * The `data` property is an array of numbers corresponding to additional service data.

#####`serviceDataUUID128`  

An object corresponding to *Service Data - 128 bit UUID* with the following properties:

 * The `uuid` property is a string corresponding to the 128-bit Service UUID.
 * The `data` property is an array of numbers corresponding to additional service data.

#####`appearance`

A number corresponding to the *Appearance*.  

#####`publicAddress`

A string corresponding to the *Public Target Address*.  

#####`randomAddress`

A string corresponding to the *Random Target Address*.  

#####`advertisingInterval`

A number corresponding to the *Advertising Interval*.  

#####`role`

A number corresponding to the *LE Role*.  

#####`uri`

A string represents *Uniform Resource Identifier*.  


<a id="gatt-api"></a>

## GATT API

### GATT Server example

This example shows how to setup a "GAP Service" in your application. The API object model is based on the GATT profile hierarchy specification. To simplify the code, we only add the "Device Name" characteristic here.  

```
/* Add GAP service */
let gapService = ble.server.addService({
	uuid: "1800",
	primary: true
});
/* Add "Device Name" characteristic */
gapService.addCharacteristic({
	uuid: "2A00",	          // Device Name UUID
	properties: 0x02,      // Read-Only
	formats: ["utf8s"],    // Format is UTF8S
	value: "Kinoma"        // Initial value
});
/* Deploy service */
ble.server.deploy();
```

### GATT Client example

This example shows how to subscribe to a GATT characteristic notification using a chain of multiple GATT procedures.  

```
let client = connection.client;
let service;
/* Perform primary service discovery */
client.discoverAllPrimaryServices().then(() => {
	/* Upon completion the service instance will be cached */
	service = client.getServiceByUUID(UUID.getByUUID16(0x180D));
	if (service == null) {
		throw "Heart rate service not found";
	}
	/* Perform characteristic discovery */
	return service.discoverAllCharacteristics();
}).then(() => {
	/* Check if the desired characteristic is found */
	let characteristic = service.getCharacteristicByUUID(UUID.getByUUID16(0x2A37));
	if (characteristic == null) {
		throw "Measurement characteristic not found";
	}
	/* Setup the callback*/
	characteristic.onNotification = value => {
		// Value is notified...
	};
	/* Perform characteristic descriptor discovery */
	return characteristic.discoverAllCharacteristicDescriptors();
}).then(() => {
	/* Check if client configuration is available */
	let descriptor = characteristic.getDescriptorByUUID(UUID.getByUUID16(0x2902));
	if (descriptor == null) {
		throw "Client configuration is not available";
	}
	/* Write value "Notification" */
	return descriptor.writeDescriptorValue(0x0001);
}).then(() => {
	// Configured
});
```

### Profile class

#### Common properties

#####`services`

| | | |
| --- | --- | --- |
| `services` | `array` | |

> An `Array` of all service objects, created by the application when in the BLE server role, or discovered when in the BLE client role.   

#### Common methods

#####`getServiceByUUID(uuid)`

| | | |
| --- | --- | --- |
| `uuid` | `object` | |

Returns a `service` object corresponding to the provided `uuid` object, or `null` if no service matching the `uuid` is found.  

#### Server methods

#####`addService(template)`

| | | |
| --- | --- | --- |
| `template` | `object` | |

>  The `template` object includes the following properties:

> * The `uuid` property is mandatory and corresponds to either a `UUID` object or a string representation of this service's UUID.
> * The `primary` property is optional. When provided, this boolean parameter is set `true` when the service is a primary service and set `false` when the service is a secondary service.
> * The `characteristics` property is an optional `Array` that contains template objects passed to the `addCharacteristic` method of the `Service` class. The characteristics are automatically added when the service is added.

Adds a new service from the `template` configuration object. This method returns a `service` object.

```
let service = server.addService({
	uuid: "180D",
	primary: true
});
```

#####`deploy()`  

Starts hosting GATT services. After services are successfully deployed to the local ATT database, the service object(s) are added to the `services` property, and can be retrieved by calling the `getServiceByUUID` method.  

#### Client methods

#####`discoverAllPrimaryServices()`  
#####`discoverPrimaryServiceByServiceUUID(uuid)`  

| | | |
| --- | --- | --- |
| `uuid` | `object` | |

> An instance of the `UUID` class corresponding to the uuid of the service. 

Perform a *Primary Service Discovery* procedure. Discovered primary service object(s) are added to the `services` property, and can be retrieved by calling the `getServiceByUUID` method. This method returns a `Promise` object.  

```
/* Perform primary service discovery */
client.discoverAllPrimaryServices().then(() => {
	/* Upon completion the service instance will be cached */
	let service = client.getServiceByUUID(uuid);
	// Do more procedures with this service...
});
```

#####`exchangeMTU(mtu)`  

| | | |
| --- | --- | --- |
| `mtu` | `number` | |

Perform the MTU exchange a.k.a. *Server Configuration* procedure. This method returns a `Promise` object.  

```
/* Perform server configuration */
client.exchangeMTU(158).then(() => {
	// Configured
});
```

#####`readMultipleCharacteristicValues(characteristics, sizeList)`

| | | |
| --- | --- | --- |
| `characteristics` | `Array` | |

> Array of `Characteristic` objects to be read.

| | | |
| --- | --- | --- |
| `sizeList` | `Array` | |

> Array of numbers that corresponding to each characteristic value size. The array length must be identical to number of `characteristics`.

Call to perform a *Read Multiple Characteristic Values* procedure. This method returns a `Promise` object.

### Service class

#### Common properties

#####`characteristics`

| | | |
| --- | --- | --- |
| `characteristics` | `array` | read only |

> `Array` of all characteristic objects created (for server role) or discovered (for client role).  

#####`end`

| | | |
| --- | --- | --- |
| `end` | `number` | read only |

> The ending handle value of this service. For the server role, the value is 0 if the service has not yet been deployed.  

#####`includes`

| | | |
| --- | --- | --- |
| `includes` | `array` | read only |

> `Array` of all (included) service objects, included by the application (for server role) or discovered (for client role).

#####`start`

| | | |
| --- | --- | --- |
| `start` | `number` | read only |

> The starting handle value of this service. For server role, the value is 0 if the service has not yet been deployed.  

#####`uuid`

| | | |
| --- | --- | --- |
| `uuid` | `object` | read only |

> UUID object corresponding to the UUID of this service.  

#### Common methods

#####`getCharacteristicByUUID(uuid)`

| | | |
| --- | --- | --- |
| `uuid` | `object` | |
> `UUID` object corresponding to the UUID of the characteristic requested.  

Returns the characteristic object matching the provided `uuid` object, or `null` if the characteristic is not found.  

#####`getIncludedServiceByUUID(uuid)`

| | | |
| --- | --- | --- |
| `uuid` | `object` | |
> `UUID` object corresponding to the UUID of the included service requested.  

Returns an included `service` object matching the provided `uuid` object, or `null` if the included service is not found.  

#### Server methods

#####`addCharacteristic(template)`  

| | | |
| --- | --- | --- |
| `template` | `object` | |

>  The `template` object includes the following properties:
> * The `uuid` property is mandatory and corresponds to either a `UUID` object or string representation of this characteristic's UUID.
> * The `properties` property is a mandatory bit-masked flag value that maps to the GATT *Charactertistic Properties*. Each flag corresponds to a behavior of this characteristic. (*Core 4.2 Specification, Vol 3, Part G: 3.3.1.1*)
   * If the *Broadcast (0x01)* flag is set, the *Server Characteristic Configuration* descriptor will be added.
   * If the *Read (0x02)* flag is set read access from the remote device will be permitted, otherwise it will be rejected with the error code *Read Not Permitted (0x02)*.
   * If the *Write Without Response (0x04)* flag, *Write (0x08)* flag, or *Authenticated Signed Write (0x40)* flag is set, write access using corresponding opcode from the remote device is permitted.
   * If the *Notify (0x10)* or *Indicate (0x20)* flag is set, a *Client Characteristic Configuration* descriptor will be added.
   * If the *Extended Properties (0x80)* flag is set, a *Characteristic Extended Properties* descriptor will be added.
> * The `extProperties` property is an optional bit-masked flag value that maps to the GATT *Characteristic Extended Properties*. (*Core 4.2 Specification, Vol 3, Part G: 3.3.3.1*)
   * ***Reliable Write (0x0001) is not supported.***
   * If the *Writable Auxiliaries (0x0002)* flag is set, the *Characteristic User Description* descriptor is writable.
> * The default value of `value`, `formats`, `requirements`, `onValueRead`, `onValueWrite`, `configuration` and `descriptors` properties can additionally be included in the `template` property.

Adds a new characteristic from the `template` configuration object. This method returns a `charactertistic` object.

***Note: Adding multiple characteristics with the same UUID is not supported.***

```
let characteristic = service.addCharacteristic({
	uuid: "e233a9b1-8e54-46ed-b716-8e06092caf10",
	/* Enable Read & Write, but no Write Without Response and Signed Writes */
	properties: 0x02 | 0x08
});
```
```
let characteristic = service.addCharacteristic({
	uuid: "e233a9b1-8e54-46ed-b716-8e06092caf10",
	/* Read only, but also notifiable so the Client Characteristic Configuration Descriptor will be available. */
	properties: 0x02 | 0x10
});
```

#####`addIncludedService(service)`

| | | |
| --- | --- | --- |
| `service` | `object` | |

Adds a `service` object to the GATT include definition of this service.  

#### Client methods

#####`discoverAllCharacteristics()`  
#####`discoverCharacteristicsByUUID(uuid)`  

Call to perform a *Characteristic Discovery* procedure. Discovered characteristic object(s) are added to the `characteristics` property, and can be retrieved by calling the `getCharacteristicByUUID` or `getCharacteristicByHandle` methods. This method returns a `Promise` object.

#####`findIncludedServices()`

Call to perform a *Relationship Discovery* procedure. Discovered included service object(s) are added to the `includes` property, and can be retrieved by calling the `getIncludedServiceByUUID` method. This method returns a `Promise` object.  

#####`getCharacteristicByHandle(handle)`  

| | | |
| --- | --- | --- |
| `handle` | `number` | |

> The handle value corresponding to the characteristic requested.  

Returns a `characteristic` object matching the provided handle, or `null` if the characteristic is not found.  

#####`readUsingCharacteristicUUID(uuid)`

| | | |
| --- | --- | --- |
| `uuid` | `object` | |
> `UUID` object corresponding to the characteristic's UUID.  

Call to perform a *Read Using Characteristic UUID* sub-procedure of the *Characteristic Value Read* procedure. Any newly available characteristics are added to the `characteristics` property. This method returns a `Promise` object.  

### Characteristic class

#### Common properties

#####`descriptors`

| | | |
| --- | --- | --- |
| `descriptors` | `array` | read only |

> Array of all `descriptor` objects, created by the application (for server role) or discovered (for client role).  

#####`formats`

| | | |
| --- | --- | --- |
| `formats` | `array` | read only |

> Array of format names for this characteristic value, corresponding to  the *Characteristic Presentation Format* descriptor(s).  

> When specifying multiple formats, the value will be treated as an array. Refer to the *Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile, Table 3.16 Characteristic Format types* for a description of format names (Short Name).

> Note that a `serializer` or `parser` will take precedence when available.

***Note for Client role:*** Since the descriptors are effective across the entire profile, the BLE stack cannot determine the complete format unless discovered all related attributes. Thus this property shall be configured manually by user, i.e. this property will not be configured by the stack.

***Note for Server role:*** *Characteristic Presentation Format* descriptor(s) will be available after deploying. Specifying mutiple formats will make a *Characteristic Aggregate Format* descriptor available. However, these descriptors will not be available via the `descriptors` property.

```
characteristic.formats = ["utf8"];		// Format specifying a single UTF8 string value.
characteristic.value = "Hello World";
```
```
characteristic.formats = ["uint8", "boolean"];	// Format specifying an array consisting of an uint8 and a boolean.
characteristic.value = [0xA0, true];
```

#####`handle`  

| | | |
| --- | --- | --- |
| `handle` | `number` | read only |

> Number that represents the handle of this characteristic. On server role, 0 if not deployed yet.  

#####`parser(packet)`
| | | |
| --- | --- | --- |
| `packet` | `array` |
> An array of numbers, that contains raw ATT packet values.

Parses a raw ATT value packet into a JavaScript object. See the `value` property section for details.

```
/* Parse a two octet little-endian packet into a UInt16 value */
characteristic.parser = packet => {
	let value = 0;
	value |= packet[0] & 0xFF;
	value |= (packet[1] << 8) & 0xFF;
	return value;
};
```

#####`properties`

| | | |
| --- | --- | --- |
| `properties` | `number` | read only |

> Number corresponding to the GATT *Characteristic Properties*. Refer to *Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile, Table 3.5 Characteristic Properties bit field*.  

#####`serializer(value)`

| | | |
| --- | --- | --- |
| `value` | `any` |
> User defined value that is identical to the `value` property.

Packetizes a value object. See the `value` property section for details.

```
/* Serializer that packetizes a UInt16 value into two octets stored in little-endian order. */
characteristic.serializer = value => {
	let packet = new Array(2);
	packet[0] = value & 0xFF;
	packet[1] = (value >> 8) & 0xFF;
	return packet;
};
characteristic.value = 0x180D;
```

#####`uuid`

| | | |
| --- | --- | --- |
| `uuid` | `object` | read only |

> `UUID` object corresponding to the UUID of this characteristic.  

#####`value`

| | | |
| --- | --- | --- |
| `value` | `any` | |
  
> Object corresponding to the value of this characteristic.  

Before the value is sent to the remote device, the object will be serialized to an `Array` of bytes if a `serializer` or the format is available, otherwise the object will be treated as an array of bytes and will be sent without serialization. After the value is received from the remote device, the array of bytes is parsed into the corresponding object and stored into this property if a `parser` or the format is available, otherwise the raw array of bytes is stored into this property.

For the server role, the value will be provided to the remote device immediately. Note that setting the `value` does not trigger an indication or notification. The application must first call `notifyValue` and/or `indicateValue`.  

#### Server properties
#####`configuration`

| | | |
| --- | --- | --- |
| `configuration` | `object` |

>  Configures permissions and requirements of standard GATT descriptors. The object must includes following properties:

> * The `description` property configures the *Characteristic User Description* descriptor, which includes following properties:
>   * The `readable` property is a `boolean` that specifies whether the descriptor is readable by the client.
>   * The `value` property is a string corresponding to the default value.
>   * The `requirements` property is a object which defines security requirements. See the `requirements` property of the `Descriptor` class section for further details.
> * The `client` property is an object to configure the *Characteristic Client Configuration* descriptor, which includes following properties:
>   * The `writable` property is a `boolean` that specifies if the descriptor is writable by the client.
>   * The `requirements` property is a object which defines security requirements. See the `requirements` property of the `Descriptor` class section for further details.
> * The `server` property is an object that configures *Characteristic Server Configuration* descriptor, which includes following properties:
>   * The `writable` property is a `boolean` that specifies if the descriptor is writable by the client.
>   * The `value` property is a number corresponding to the default value.
>   * The `requirements` property is a object which defines security requirements. See `requirements` property of `Descriptor` class section for further details.

#####`extProperties` 

| | | |
| --- | --- | --- |
| `extProperties` | `number` | read only |

> Number corresponding to GATT *Characteristic Extended Properties*.  

#####`requirements`  

| | | |
| --- | --- | --- |
| `requirements` | `object` | write only |

> Object that specifies the read/write GAP security requirements for this characteristic value. Remote devices trying to access this characteristic need to fullfill the security requirements. Otherwise service requests will be rejected.

> The object may include a `read` property corresponding to the read access requirement and/or a `write` property corresponding to the write access requirement. If the property is ``undefined`` or ``null``, no security is required for access. Each member will include the following properties:

> * The mandatory `encryption` property is a `boolean` that specifies whether the characteristic requires encryption for access. In the GAP specification, this is otherwise known as *LE Security Mode 1*.
> * The mandatory `authentication` property is a `boolean` that specifies whether the characteristic requires authenticated pairing for access. In the GAP specification, the `encryption` property corresponds to the *security level* of the security mode.

For GAP security level information, refer to *Core 4.2 Specification, Vol 3, Part C: Generic Access Profile, 10.2 LE Secuirty Modes*.

***Note: Current BLE stack does not support LE Secure Connection (a.k.a LE Secuirty Mode 1 - Level 4).***

```javascript
characteristic.requirements = {
	/* Read: LE security mode 1 - Level 1 (No Security)
	 * Write: LE security mode 1 - Level 3
	 */
	write: {
		/* Require encryption */
		encryption: true,
		/* Require authenticated paring */
		authentication: true
	}
};
```

```javascript
characteristic.requirements = {
	/* Read: LE security mode 1 - Level 1 (No Security)
	 * Write: LE security mode 2 - Level 1
	 */
	write: {
		/* Does not require encryption but data signing */
		encryption: false,
		/* Does not require authenticated paring. */
		authentication: false
	}
};
```

#### Server callback events

#####`onValueRead(characteristic, connection)`

| | | |
| --- | --- | --- |
| `characteristic ` | `object` | |

> An instance of the `Characteristic` class containing the value being read.

| | | |
| --- | --- | --- |
| `connection ` | `object` | |

> An instance of the `BLEConnection` class where the read is occurring.

This callback is called when this characteristic's value is read by a remote device. In the callback, set the characteristic `value` property to have the value sent back to the remote device.

```
characteristic.onValueRead = (self, connection) => {
	/* Value 'foo' will be sent to the remote device */
	self.value = foo;
};
```

#####`onValueWrite(characteristic, connection)`   

| | | |
| --- | --- | --- |
| `characteristic ` | `object` | |

> An instance of `Characteristic` class that is being written by the remote device.

| | | |
| --- | --- | --- |
| `connection ` | `object` | |

> An instance of `BLEConnection` class where the write is occurring.

This callback is called when this characteristic's value has been written by the remote device.  

```
characteristic.onValueWrite = (self, connection) => {
	/* Set 'foo' to the value written by the remote device */
	foo = self.value;
};
```

#### Client properties

#####`end`

| | | |
| --- | --- | --- |
| `end` | `number` | read only |

> Ending handle numeric value of this characteristic if known, otherwise `end` will be the same handle value as the `handle` property.  

#### Client callback events

#####`onIndication`   

This callback is called when the value has been indicated by the remote device, before a confirmation is sent.  

#####`onNotification`   

This callback is called when the value has been notified by the remote device.  

#### Common methods

#####`getDescriptorByUUID(uuid)`

| | | |
| --- | --- | --- |
| `uuid` | `object` | |

Returns a `descriptor` object matching the provided uuid object, or `null` if the descriptor is not found. If there are multiple descriptors with the same UUID, only first matching `descriptor` is returned.  

#### Server methods

#####`addDescriptor(template)`

| | | |
| --- | --- | --- |
| `template` | `object` | |

> The `template` object includes the following properties:

> * The `uuid` property is mandatory and corresponds to either a `UUID` object or string representation of this characteristic's UUID.
> * The `readable` property is a mandatory `boolean` that makes the descriptor readable by the remote device.
> * The `writable` property is a mandatory `boolean` that makes the descriptor writable by the remote device.
> * The default value of `onValueRead`, `onValueWrite`, `value`, and `requirements` properties can additionally be included in the `template` object.

Adds a new descriptor from the `template` configuration object. This method returns a `descriptor` object.

#####`notifyValue(connection, value)`
| | | |
| --- | --- | --- |
| `connection ` | `object` |

> An instance of `BLEConnection` corresponding to the desired remote device.

| | | |
| --- | --- | --- |
| `value ` | `any` | optional |

>  Will be set to the `value` property before the procedure is performed.

Performs a *Charactertistic Value Notfication* procedure. This procedure will check the *Client Characteristic Configuration* to see if the client has written the descriptor before. If not, the procedure will be cancelled.

```
characteristic.value = newValue;
/* Perform characteristic value notification */
characteristic.notifyValue(connection);
```

#####`indicateValue(connection, value)`
| | | |
| --- | --- | --- |
| `connection ` | `object` |

> An instance of `BLEConnection` corresponding to the desired remote device.

| | | |
| --- | --- | --- |
| `value` | `any` | optional |

>  Will be set to `value` property before the procedure is performed.

Performs a *Charactertistic Value Indication* procedure. This procedure will check the *Client Characteristic Configuration* to see if the client has written the descriptor before. If not, the procedure will be cancelled. This method returns a `Promise` object.

```
/* Perform characteristic value indication */
characteristic.indicateValue(connection, newValue).then(() => {
	/* Confirmation has been received */
});
```

#### Client methods

#####`discoverAllCharacteristicDescriptors()`  

Performs a *Characteristic Descriptor Discovery* procedure. Discovered descriptor object(s) are added to the `descriptors` property, and can be retrieved by calling the `getDescriptorByUUID` method. This method returns a `Promise` object.  

#####`readCharacteristicValue(length)`

| | | |
| --- | --- | --- |
| `length` | `number` | optional |

>  Used only when performing the *Read Long Characteristic Values* sub-procedure.

Performs a *Read Characteristic Value* and/or *Read Long Characteristic Values* sub-procedure of *Characteristic Value Read* procedure. This method returns a `Promise` object.  

#####`writeWithoutResponse(signed, value)`  
| | | |
| --- | --- | --- |
| `signed ` | `boolean` | optional |

> Set `true` to enable data signing. (i.e. LE Security Mode 2)

| | | |
| --- | --- | --- |
| `value` | `number` | optional |

> Will be set before the procedure is performed.

Performs a `Write Without Response` sub-procedure of *Characteristic Value Write* procedure.  

#####`writeCharacteristicValue(value)`

| | | |
| --- | --- | --- |
| `value` | `number` |

> Will be set before the procedure is performed.

Performs a *Write Characteristic Value* sub-procedure of *Characteristic Value Write* procedure. This method returns a `Promise` object.  

### Descriptor class

#### Common properties

#####`uuid`

| | | |
| --- | --- | --- |
| `uuid` | `object` | read only |

> UUID object corresponding to the UUID of this `descriptor`.  

#####`value`

| | | |
| --- | --- | --- |
| `value` | `object` | |

> Object corresponding to the value of this `descriptor`. GATT defined standard descriptors are internally parsed and serialized as follows:

> * *Characteristic Extended Properties*, *Client Characteristic Configuration*, and *Server Characteristic Configuration* are treated as *uint16* format as defined in *3.3.3.5.2 Format*.
> * *Characteristic User Description* is treated as *utf8s* format as defined in *3.3.3.5.2 Format*.
> * *Characteristic Presentation Format* is treated as an object including the following properties:
   * The **format** property is a number for *Format*.
   * The **exponent** property a number for *Exponent*.
   * The **unit** property is **UUID** object for *Unit*.
   * The **namespace** property is a number for *Name Space*.
   * The **description** property is a number for *Description*.
> * *Characteristic Aggregate Format* is treated as an ``Array`` of numbers.

#### Server properties

#####`readable`

| | | |
| --- | --- | --- |
| `readable` | `boolean` | read only |

> ``true`` if this descriptor is readable by the remote device.

#####`requirements`

| | | |
| --- | --- | --- |
| `security` | `object` | write only | 

> Object that specifies the read/write GAP security requirements for this descriptor value. Refer to the property has the same name in `Characteristic` class section for the details. 

#####`writable`

| | | |
| --- | --- | --- |
| `writable` | `boolean` | read only | 

> ``true`` if this descriptor is writable by the remote device.

#### Server callback events

#####`onValueRead(descriptor, connection)`   

This callback is called when this descriptor value has been attemped to be read by the remote device. Refer to the `onValueRead` event in the `Characteristic` class section for further details.

#####`onValueWrite(descriptor, connection)`   

This callback is called when this descriptor value has been written by remote device. Refer to the `onValueWrite` event in the `Characteristic` class section for further details.

#### Client properties

#####`handle`

| | | |
| --- | --- | --- |
| `handle` | `number` | read only | 

> Number corresponding to the handle of this descriptor.  

#### Client methods

#####`readDescriptorValue(length)`  

| | | |
| --- | --- | --- |
| `length` | `number` | optional |

> Used only when performing the *Read Long Characteristic Descriptors* sub-procedure. 

Performs a *Read Characteristic Descriptors* and/or *Read Long Characteristic Descriptors* sub-procedure of the *Characteristic Descriptors* procedure. This method returns a `Promise` object.  

#####`writeDescriptorValue(value)`  

| | | |
| --- | --- | --- |
| `value` | `number` |

> Set before the procedure is performed.

Performs a *Write Characteristic Descriptors* sub-procedure of the *Characteristic Descriptors* procedure. This method returns a `Promise` object.  

## Common API

### UUID Object

```
let uuid1 = UUID.getByUUID([0x00, 0x18]);								// Instantiate a UUID object representing the 16-bit UUID '0x1800'.
let uuid2 = UUID.getByString("72C90001-57A9-4D40-B746-534E22EC9F9E");	// Instantiate a UUID object representing a 128-bit UUID.
let uuid3 = UUID.getByUUID16(0x1800);									// Instantiate a UUID object representing the 16-bit UUID '0x1800'.
uuid1.equals(uuid3);	// Returns 'true'
```

#### Static methods

#####`getByUUID(byteArray)`
| | | |
| --- | --- | --- |
| `byteArray ` | `array` |

> Array of numbers that contains raw UUID values in LSB-first byte order. The array length must be 16 (for 128-bit UUID) or 2 (for 16-bit UUID).

Returns a UUID object corresponding to the `byteArray`.  

#####`getByString(uuidString)`
| | | |
| --- | --- | --- |
| `uuidString ` | `string` |

> String representation of the UUID, in 8-4-4-4-12 format.

Returns a UUID object corresponding to the `uuidString` string representation.  

#####`getByUUID16(uuid16)`
| | | |
| --- | --- | --- |
| `uuid16 ` | `number` |

> 16-bit UUID number.

Returns a UUID object corresponding to the 16-bit `uuid16` number.  

#### Common methods

#####`equals(target)`
| | | |
| --- | --- | --- |
| `target ` | `object` |

> An instance of UUID class to compare.

Return `true` if this UUID equals the `target` UUID.  

#####`getRawArray()`

Returns the internal byte array.  

#####`isUUID16()`

Returns `true` if this UUID is a 16-bit UUID.  

#####`toString()`

Returns a string representation of this UUID.  

#####`toUUID16()`

Returns a number corresponding to the 16-bit UUID.  

#####`toUUID128()`

Returns an array corresponding to the 128-bit UUID.  

### BluetoothAddress

```
// Instantiate a BluetoothAddress object representing a static random address
let address = BluetoothAddress.getByAddress([0xD4, 0x42, 0xFF, 0x4C, 0x6A, 0xFC], true);

address.isRandom();		// Returns 'true'
address.isIdentity();	// Returns 'true'
address.isResolvable();	// Returns 'false'
address.toString();		// Returns "FC:6A:4C:FF:42:D4"
let address2 = BluetoothAddress.getByString("FC:6A:4C:FF:42:D4", true);
address.equals(address2);	// Returns 'true'
```

#### Static methods

#####`getByAddress(byteArray, random)`  
| | | |
| --- | --- | --- |
| `byteArray ` | `array` |

> Array of numbers that contains raw address values in LSB-first byte order. The array length must be 6.


| | | |
| --- | --- | --- |
| `random` | `boolean` | |

> Set to `true` if the address is random, otherwise `false`.  

Returns a new `BluetoothAddress` object corresponding to the `byteArray`.

#####`getByString(addressString, random)`

| | | |
| --- | --- | --- |
| `addressString ` | `array` |

> String representation of the address, in XX:XX:XX:XX:XX:XX format.

| | | |
| --- | --- | --- |
| `random` | `boolean` | |

> Set to `true` if the address is random, otherwise `false`. 

Returns a new `BluetoothAddress` object corresponding to the `addressString` string.

#### Common properties

#####`type`

| | | |
| --- | --- | --- |
| `type` | `number` | read only | 

> Number corresponding to the BLE address type. (i.e. The two most significant bits of the address.).  

#####`typeString`

| | | |
| --- | --- | --- |
| `typeString` | `string` | read only |   

> String representation of this BLE address type.  

#### Common methods

#####`equals(target)`
| | | |
| --- | --- | --- |
| `target ` | `object` |

> An instance of a `BluetoothAddress` class to compare.

Returns `true` if this BLE address equals the *target* address.  

#####`getRawArray()` 

Returns the internal byte array.  

#####`isIdentity()` 
Returns `true` if this BLE address is an identity address, i.e. public address or static random address.

#####`isRandom()`  

Returns `true` if this BLE address is a random address.  

#####`isResolvable()`  

Returns `true` if this BLE address is a resolvable private address.  

#####`toString()`  

Returns the string representation of this BLE address.  