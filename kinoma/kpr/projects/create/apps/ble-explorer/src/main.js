//@program
/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
var FTHEME = require("themes/flat/theme");
THEME = require("themes/sample/theme");
for (var i in FTHEME)
	THEME[i] = FTHEME[i];
var MODEL = require("mobile/model");
var CONTROL = require("mobile/control");
var SCROLLER = require("mobile/scroller");
var CREATIONS = require("creations/creations");
var SCREEN = require("mobile/screen");
var Pins = require("pins");

// ----------------------------------------------------------------------------------
// styles
// ----------------------------------------------------------------------------------
var applicationStyle = new Style({font: "16px Fira Sans"});
var categoryHeaderStyle = new Style({font:"22px", color:"white", horizontal:"left", vertical:"middle", left: 10, right: 10, lines:"1"});
var titleStyle = new Style({font:"24px", color:"black", horizontal:"center", vertical:"middle", lines:"1"});
var listItemStyle = new Style({font:"24px", color:"black", horizontal:"left", vertical:"middle", lines:"1"});
var listSmallItemStyle = new Style({font:"16px", color:"black", horizontal:"left", vertical:"middle", lines:"1"});
var listAddressStyle = new Style({font:"18px", color:"gray", horizontal:"left", vertical:"middle", lines:"1"});
var rssiStyle = new Style({font:"20px", color:"black", horizontal:"middle", vertical:"middle"});

// ----------------------------------------------------------------------------------
// assets
// ----------------------------------------------------------------------------------
var categoryHeaderSkin = new Skin({fill: "#6cb535"});
var dividerLineSkin = new Skin({fill: "#e2e2e2"});
var listArrowTexture = new Texture("./assets/list-arrow.png");
var listArrowSkin = new Skin({ texture: listArrowTexture,  x:0, y:0, width:32, height:32, states:32 });
var rssiTexture = new Texture("./assets/circle-strength-13-40x40.png");
var rssiSkin = new Skin({ texture:rssiTexture,  x:0, y:0, width:40, height:40, variants:40 });
var whiteSkin = new Skin({fill: "white"});

// ----------------------------------------------------------------------------------
// UI common layouts
// ----------------------------------------------------------------------------------

var ListEmptyItem = Column.template(function($) { return {
	left: 0, right: 0, top: 40,
	contents: [
		CREATIONS.BusyPicture($),
		Label($, {left:10, right:10, top: 30, style: titleStyle, string: $}),
	]
}});

var ListGenericItem = Label.template(function($) { return {
	left: 14, right: 0, top: 0, skin: THEME.lineSkin, style: listItemStyle, string: $
}});

var CategoryHeader = Label.template(function($) { return {
	left: 0, right: 0, height: 24, style: categoryHeaderStyle, skin: categoryHeaderSkin, string: $
}});

// ----------------------------------------------------------------------------------
// Characteristic Value Screen (displays one characteristic's value)
// ----------------------------------------------------------------------------------

Handler.bind("/value", MODEL.ScreenBehavior({
	hasSelection: function(data, delta) {
		var selection = data.selection + delta;
		return (0 <= selection) && (selection < data.items.length)
	},
	getSelection: function(data, delta) {
		data.selection += delta;
		return data.items[data.selection];
	},
	onDescribe: function(query, selection) {
		var peripheral = model.data.peripherals[query.peripheral_address];
		var service = peripheral.services[query.service_uuid];
		var data = {
			title: peripheral.name,
			Screen: CharacteristicValueScreen,
			peripheral: peripheral,
			service: service,
			handle: query.handle,
			name: query.name,
			uuid: query.characteristic_uuid,
			ascii: "N/A",
			hex: "N/A",
			items: [],
			selection: -1,
			more: false
		};
		return data;
	}
}));

var BLEServiceCharacteristicValueBrowserBehavior = Behavior({	
	isAsciiReadable: function(chunk) {
		// It seems that some ASCII characteristic values are delivered with trailing 0-bytes. So we scan backwards to catch that case.
		var hasAscii = false;
		for (var i = chunk.length - 1; i >= 0; --i) {
			var byte = chunk[i];
			if ((0 == byte) && !hasAscii)
				continue;
			else if (byte < 32 || byte > 126)
				return false;
			else
				hasAscii = true;
		}
		return hasAscii;
	},
	onCreate: function(container, data) {
		this.data = data;
		this.container = container;
		this.peripheral = this.data.peripheral;
		this.service = this.data.service;
	},
	onDisplayed: function(container) {
		var peripheral = this.peripheral;
		var params = {connection: peripheral.connection, characteristic: this.data.handle};
		Pins.invoke("/ble/gattReadCharacteristicValue", params);
	},
	onGattCharacteristicValueFound: function(container, response) {
		var chunk = response.value;
		var hexString = "";
		for (var i = 0, c = chunk.length; i < c; ++i) {
			var byte = chunk[i];
			var hex = byte.toString(16);
			if (byte < 16)
				hex = '0' + hex;
			hexString += hex;
			if (i < c - 1)
				hexString += ' ';
		}
		if (!hexString)
			hexString = "N/A";
		this.data.hex = hexString;
		
		var valueString = "";
		if (this.isAsciiReadable(chunk)) {
			for (var i = 0, c = chunk.length; i < c; ++i) {
				var byte = chunk[i];
				valueString += String.fromCharCode(byte);
			}
		}
		if (!valueString)
			valueString = "N/A";
		this.data.ascii = valueString;
		
		container.distribute("onCharacteristicValueRead");
	}
});

var CharacteristicValueItem = Column.template(function($) { return {
	left: 0, right: 0, top: 0, skin: THEME.lineSkin, behavior: SCREEN.ListItemBehavior,
	contents: [
		Line($, {
			left: 5, right: 0, top: 0, bottom: 0,
			contents: [
				Column($, {anchor: "CHARACTERISTIC",
					left: 5, right: 0, top: 0, bottom: 0,
					contents: [
						Label($, { anchor: "ASCII", left: 5, right: 5, style: listSmallItemStyle, string: 'ASCII: ' + $.ascii  }),
						Label($, { anchor: "HEX", left: 5, right: 5, style: listSmallItemStyle, string: 'Hex: ' + $.hex }),
					]
				}),
			]
		}),
		Content($, {left: 0, right: 0, bottom: 0, height: 1, skin: dividerLineSkin})
	]
}});

var ListCharacteristicItem = Column.template(function($) { return {
	left: 0, right: 0, top: 0, skin: THEME.lineSkin,
	contents: [
		Line($, {
			left: 5, right: 0, top: 0, bottom: 0,
			contents: [
				Column($, {anchor: "CHARACTERISTIC",
					left: 5, right: 0, top: 0, bottom: 0,
					contents: [
						Label($, { anchor: "NAME", left: 5, right: 5, style: listItemStyle, string: $.name  }),
						Label($, { anchor: "UUID", left: 5, right: 5, style: listSmallItemStyle, string: 'UUID: ' + $.uuid }),
					]
				}),
			]
		}),
		Content($, {left: 0, right: 0, bottom: 0, height: 1, skin: dividerLineSkin})
	]
}});

var CharacteristicValueScreen = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: whiteSkin,
	contents: [
		CREATIONS.DynamicHeader($),
		Container($, {
			left: 0, right: 0, top: 32, bottom: 0, behavior: BLEServiceCharacteristicValueBrowserBehavior,
			contents: [
				SCROLLER.VerticalScroller($, {
					clip: true,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							behavior: SCREEN.ListBehavior({
								addBusyLine: function(list) {
									list.add(new ListEmptyItem("Reading characteristic value..."));
								},
								addItemLine: function(list, item, index) {
									list.add(new CharacteristicValueItem(item));
								},
								addLines: function(list, items, more) {
									if (items.length) {
										list.add(new CategoryHeader("Service"));
										list.add(new ListGenericItem(this.service.name));
										list.add(new CategoryHeader("Characteristic"));
										list.add(new ListCharacteristicItem({name: this.data.name, uuid: this.data.uuid}));
										list.add(new CategoryHeader("Value"));
									}
									for (var i = 0, c = items.length; i < c; ++i) {
										this.addItemLine(list, items[i], i);
									}
									if (0 == list.length)
										this.addBusyLine(list);
								},
								onCreate: function(list, data) {
									this.data = data;
									this.peripheral = this.data.peripheral;
									this.service = this.data.service;
									this.addBusyLine(list);
								},
								onCharacteristicValueRead: function(list) {
									this.reload(list);
								},
								reload: function(list) {
									list.cancel();
									list.empty();
									var items = this.data.items = [];
									var item = {
										uuid: this.data.uuid,
										ascii: this.data.ascii,
										hex: this.data.hex
									}
									items.push(item);
									this.addLines(list, items, false);
								},
							}),
						}),
						SCROLLER.VerticalScrollbar($),
						SCROLLER.BottomScrollerShadow($)
					]
				}),
			],
		})
	]
}});

// ----------------------------------------------------------------------------------
// Characteristics Screen (displays one service's characteristic names, UUIDs and properties)
// ----------------------------------------------------------------------------------

Handler.bind("/characteristics", MODEL.ScreenBehavior({
	hasSelection: function(data, delta) {
		var selection = data.selection + delta;
		return (0 <= selection) && (selection < data.items.length)
	},
	getSelection: function(data, delta) {
		data.selection += delta;
		return data.items[data.selection];
	},
	onDescribe: function(query, selection) {
		var peripheral = model.data.peripherals[query.peripheral_address];
		var service = peripheral.services[query.service_uuid];
		var data = {
			title: peripheral.name,
			Screen: CharacteristicsScreen,
			peripheral: peripheral,
			service: service,
			items: [],
			selection: -1,
			more: false
		};
		return data;
	}
}));

var BLEServiceCharacteristicsBrowserBehavior = Behavior({	
	onCreate: function(container, data) {
		this.data = data;
		this.container = container;
		this.peripheral = this.data.peripheral;
		this.service = this.data.service;
	},
	onDisplayed: function(container) {
		if (!this.service.discoveredCharacteristics) {
			var params = {
				connection: this.peripheral.connection,
				start: this.service.start_handle,
				end: this.service.end_handle
			};
			Pins.invoke("/ble/gattDiscoverAllCharacteristics", params);
		}
		else
			application.distribute("onServiceCharacteristicsAdded", this.service);
	},
	onGattCharacteristicFound: function(container, result) {
		var service = this.service;
		var characteristics = service.characteristics;
		var characteristic = {
			properties: result.properties,
			handle: result.characteristic,
			uuid: result.uuid
		};
		characteristics.push(characteristic);
	},
	onGattRequestCompleted: function(container, response) {
		this.service.discoveredCharacteristics = true;
		application.distribute("onServiceCharacteristicsAdded", this.service);
	}
});

var CharacteristicItem = Column.template(function($) { return {
	left: 0, right: 0, top: 0, skin: THEME.lineSkin, active: "action" in $,
	behavior: SCREEN.ListItemBehavior($),
	contents: [
		Line($, {
			left: 5, right: 0, top: 0, bottom: 0,
			contents: [
				Column($, {anchor: "CHARACTERISTIC",
					left: 5, right: 0, top: 0, bottom: 0,
					contents: [
						Label($, { anchor: "NAME", left: 5, right: 5, style: listItemStyle, string: $.name }),
						Label($, { anchor: "UUID", left: 5, right: 5, style: listSmallItemStyle, string: 'UUID: ' + $.uuid }),
						Label($, { anchor: "PROPERTIES", left: 5, right: 5, style: listSmallItemStyle, string: 'Properties: ' + $.properties }),
					]
				}),
				Content($, {right: 0, top: 0, bottom: 0, skin: listArrowSkin, visible: "action" in $}),
			]
		}),
		Content($, {left: 0, right: 0, bottom: 0, height: 1, skin: dividerLineSkin})
	]
}});

var CharacteristicsScreen = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: whiteSkin,
	contents: [
		CREATIONS.DynamicHeader($),
		Container($, {
			left: 0, right: 0, top: 32, bottom: 0, behavior: BLEServiceCharacteristicsBrowserBehavior,
			contents: [
				SCROLLER.VerticalScroller($, {
					clip: true,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							behavior: SCREEN.ListBehavior({
								addBusyLine: function(list) {
									list.add(new ListEmptyItem("Discovering characteristics..."));
								},
								addItemLine: function(list, item, index) {
									list.add(new CharacteristicItem(item));
								},
								addLines: function(list, items, more) {
									if (items.length) {
										list.add(new CategoryHeader("Service"));
										list.add(new ListGenericItem(this.service.name));
										list.add(new CategoryHeader("Characteristics"), list.first);
									}
									for (var i = 0, c = items.length; i < c; ++i) {
										this.addItemLine(list, items[i], i);
									}
									if (0 == list.length)
										this.addBusyLine(list);
								},
								onCreate: function(list, data) {
									this.data = data;
									this.peripheral = this.data.peripheral;
									this.service = this.data.service;
									this.gatt_service = (this.service.uuid in model.data.gatt_services ? model.data.gatt_services[this.service.uuid] : undefined);
									this.reload(list);
								},
								onServiceCharacteristicsAdded: function(list, service) {
									this.reload(list);
								},
								reload: function(list) {
									list.cancel();
									list.empty();
									var items = this.data.items = [];
									var characteristics = this.service.characteristics;
									var gatt_service = this.gatt_service;
									for (var i = 0, c = characteristics.length; i < c; ++i) {
										var characteristic = characteristics[i];
										var item = {
											name: "Unknown Characteristic",
											handle: characteristic.handle,
											uuid: characteristic.uuid,
											properties: this.getPropertyStrings(characteristic.properties),
										}
										if (gatt_service && (characteristic.uuid in gatt_service.characteristics))
											item.name = gatt_service.characteristics[characteristic.uuid].name;
										if (-1 != item.properties.indexOf("Read")) {
											item.action = "/value?" + serializeQuery({
												peripheral_address: this.peripheral.address,
												service_uuid: this.service.uuid,
												handle: characteristic.handle,
												characteristic_uuid: item.uuid,
												name: item.name
											});
										}
										items.push(item);
									}
									this.addLines(list, items, false);
								},
								getPropertyStrings: function(properties) {
									var propertyStrings = [];
									for (var i = 0, c = properties.length; i < c; ++i) {
										var property = properties[i];
										if ("broadcast" == property)
											propertyStrings.push("Broadcast");
										else if ("read" == property)
											propertyStrings.push("Read");
										else if ("writeWithoutResponse" == property)
											propertyStrings.push("Write without response");
										else if ("write" == property)
											propertyStrings.push("Write");
										else if ("notify" == property)
											propertyStrings.push("Notify");
										else if ("indicate" == property)
											propertyStrings.push("Indicate");
									}
									return propertyStrings.join(",");
								},
							}),
						}),
						SCROLLER.VerticalScrollbar($),
						SCROLLER.BottomScrollerShadow($)
					]
				}),
			],
		})
	]
}});

// ----------------------------------------------------------------------------------
// Services Screen
// ----------------------------------------------------------------------------------

Handler.bind("/services", MODEL.ScreenBehavior({
	hasSelection: function(data, delta) {
		var selection = data.selection + delta;
		return (0 <= selection) && (selection < data.items.length)
	},
	getSelection: function(data, delta) {
		data.selection += delta;
		return data.items[data.selection];
	},
	onDescribe: function(query, selection) {
		var peripheral = model.data.peripherals[query.address];
		var data = {
			title: peripheral.name,
			Screen: ServicesScreen,
			peripheral: peripheral,
			items: [],
			selection: -1,
			more: false
		};
		return data;
	}
}));

var BLEServiceDiscoveryBehavior = Behavior({
	discoverServices: function() {
		var container = this.container;
		var peripheral = this.peripheral;
		
		// Discover all primary services
		peripheral.services = {};
		Pins.invoke("/ble/gattDiscoverAllPrimaryServices", { connection:peripheral.connection });
	},
	onCreate: function(container, data) {
		this.data = data;
		this.container = container;
		this.peripheral = this.data.peripheral;
	},
	onDisplayed: function(container) {
		var peripheral = this.peripheral;
		
		// Connect to peripheral if not already connected
		if (null === peripheral.connection) {
			// The connection interval values are in units of 1.25 ms. A shorter connection interval speeds up discovery.
			// Connection interval values must also be divisible by (connection count * 2.5 ms)
			// The link supervision timeout, expressed in units of 10 ms, governs how long the link layer waits before disconnecting a device.
			var connection_interval = 16;	// 20 ms
			var timeout = 50;				// 1/2 second
			var params = {
				address: this.peripheral.address,
				addressType: this.peripheral.addressType,
				intervals: {min:connection_interval, max:connection_interval},
				timeout: timeout,
			};
			Pins.invoke("/ble/gapConnect", params);
			return;
		}
		
		// Stop scanning if we're already connected to the peripheral
		if (null != peripheral.connection) {
			this.stopScanning();
		}
		
		// Discover services if not already discovered
		if (!peripheral.discoveredServices) {
			this.discoverServices();
		}
	},
	onGattRequestCompleted: function(container, response) {
		this.peripheral.discoveredServices = true;
	},
	onGattServiceFound: function(container, result) {
		var peripheral = this.peripheral;
		var uuid = result.uuid;
		
		// Filter out Generic Access and Generic Attribute services - we don't want to browse these
		if ("1800" == uuid || "1801" == uuid)
			return;
			
		var service = {
			peripheral_address: peripheral.address,
			peripheral_name: peripheral.name,
			connection_handle: result.connection,
			start_handle: result.start,
			end_handle: result.end,
			characteristics: [],
			discoveredCharacteristics: false,
			name: "Unknown Service",
			type: null
		}
		service.uuid = uuid;
		if (!service.uuid)
			throw new Error("Service missing UUID");
		if (service.uuid in model.data.gatt_services) {
			var gatt_service = model.data.gatt_services[service.uuid];
			service.name = gatt_service.name;
			service.type = gatt_service.type;
		}
		//trace("gatt service discovered: " + JSON.stringify(service) + "\n");
		
		// Add service to peripheral
		peripheral.services[service.uuid] = service;
		application.distribute("onServiceAdded", peripheral, service);		
	},
	onPeripheralConnected: function(container, connection) {
		// Save the peripheral's connection handle and discover services
		if (null == this.peripheral.connection) {
			//trace("peripheral connected, discovering services\n");
			this.peripheral.connection = connection;
			this.stopScanning();
			this.discoverServices();
		}
		else {
			trace("Peripheral connected with non-null connection handle?!?\n");
		}
	},
	onPeripheralDisconnected: function(container, connection) {
		// If our peripheral disconnected, let the application handle the event and go back home
		// We don't care if another peripheral disconnected. That will be handled by the main screen when we get back home
		if (this.peripheral.connection != connection)
			return true;
	},
	stopScanning: function() {
		Pins.invoke("/ble/gapStopScanning");
	}
});

var ServiceItem = Column.template(function($) { return {
	left: 0, right: 0, top: 0, height: 48, skin: THEME.lineSkin, active: true,
	behavior: SCREEN.ListItemBehavior({
		onDisplaying: function(item) {
			var service = this.data.service;
			this.data.action = "/characteristics?" + serializeQuery({
				peripheral_address: service.peripheral_address,
				service_uuid: service.uuid
			});
		},
	}),
	contents: [
		Line($, {
			left: 5, right: 0, top: 0, bottom: 0,
			contents: [
				Text($, { 
					left: 5, right: 0, top: 0, bottom: 0,
					blocks: [
						{ style: listItemStyle, string: $.service.name },
						{ style: listSmallItemStyle, string: 'UUID: ' + $.service.uuid },
					]
				}),
				Content($, {right: 0, top: 0, bottom: 0, skin: listArrowSkin}),
			]
		}),
		Content($, {left: 0, right: 0, bottom: 0, height: 1, skin: dividerLineSkin})
	]
}});

var ServicesScreen = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: whiteSkin,
	contents: [
		CREATIONS.DynamicHeader($),
		Container($, {
			left: 0, right: 0, top: 32, bottom: 0, behavior: BLEServiceDiscoveryBehavior,
			contents: [
				SCROLLER.VerticalScroller($, {
					clip: true,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							behavior: SCREEN.ListBehavior({
								addBusyLine: function(list) {
									list.add(new ListEmptyItem("Discovering services..."));
								},
								addItemLine: function(list, item, index) {
									list.add(new ServiceItem(item));
								},
								addLines: function(list, items, more) {
									for (var i = 0, c = items.length; i < c; ++i) {
										this.addItemLine(list, items[i], i);
									}
									if (0 == list.length)
										this.addBusyLine(list);
									else
										list.insert(new CategoryHeader("Services"), list.first);
								},
								onCreate: function(list, data) {
									this.data = data;
									this.reload(list);
								},
								onServiceAdded: function(list, peripheral, service) {
									this.reload(list);
								},
								reload: function(list) {
									list.cancel();
									list.empty();
									var entries = [];
									var services = this.data.peripheral.services;
									for (var uuid in services)
										entries.push(services[uuid]);
									entries.sort(function(a, b) {return a.name.toLowerCase().compare(b.name.toLowerCase())});
									var length = entries.length;
									this.data.items = new Array(length);
									for (var i = 0; i < length; ++i)
										this.data.items[i] = {service: entries[i]};
									this.addLines(list, this.data.items, false);
								}
							}),
						}),
						SCROLLER.VerticalScrollbar($),
						SCROLLER.BottomScrollerShadow($)
					]
				}),
			],
		})
	]
}});

// ----------------------------------------------------------------------------------
// Discovered devices Screen (main screen)
// ----------------------------------------------------------------------------------

Handler.bind("/main", MODEL.ScreenBehavior({
	hasSelection: function(data, delta) {
		var selection = data.selection + delta;
		return (0 <= selection) && (selection < data.items.length)
	},
	getSelection: function(data, delta) {
		data.selection += delta;
		return data.items[data.selection];
	},
	onDescribe: function(query, selection) {
		return {
			Screen: DevicesScreen,
			selection: -1,
			items: [],
			more: false,
        	title: "BLE Explorer",
		};
	}
}));

var BLEDeviceDiscoveryBehavior = Behavior({
	onCreate: function(container, data) {
		this.container = container;
		this.advertisementInterval = 3000;
		container.interval = this.advertisementInterval;
		
		Pins.invoke("/ble/gapStartScanning");

		container.start();
		container.distribute("onScanStart");
	},
	onPeripheralDiscovered: function(container, peripheral) {
		//trace("discovered response: " + JSON.stringify(peripheral) + "\n");
		
		var address = peripheral.address;
		
		// Save off this peripheral discovered
		if (!(address in model.data.peripherals)) {
			peripheral.name = "Unknown";
			peripheral.connection = null;
			peripheral.services = {};
			peripheral.discoveredServices = false;
			
			model.data.peripherals[address] = peripheral;
			
			container.distribute("onPeripheralAdded", peripheral);
		}
		
		// Update the saved peripheral's signal level and name
		else {
			model.data.peripherals[address].rssi = peripheral.rssi;
			for (var i = 0, data = peripheral.data, c = data.length; i < c; ++i) {
				var entry = data[i];
				if (9 == entry.flag) {	// Complete local name advertising packet type
					model.data.peripherals[address].name = entry.data;
					break;
				}
			}
			container.distribute("onPeripheralChanged", model.data.peripherals[address]);
		}
		var peripheral = model.data.peripherals[address];
		
		// Remember the last time this peripheral advertised
		peripheral.lastAdvertisementTime = Date.now();
	},
	onRSSIChanged: function(container, connection, rssi) {
		for (var address in model.data.peripherals) {
			var peripheral = model.data.peripherals[address];
			if (connection == peripheral.connection) {
				peripheral.rssi = rssi;
				peripheral.lastAdvertisementTime = Date.now();
				break;
			}
		}
	},
	onTimeChanged: function(container) {
		// Remove any peripherals that haven't provided a scan response in a reasonable amount of time
		var now = Date.now();
		for (var address in model.data.peripherals) {
			var peripheral = model.data.peripherals[address];
			if ((-1 != peripheral.lastAdvertisementTime) && (now - peripheral.lastAdvertisementTime > this.advertisementInterval)) {
				delete model.data.peripherals[address];
				application.distribute("onPeripheralRemoved", peripheral);
			}
		}
	},
});

var DeviceListItem = Column.template(function($) { return {
	left: 0, right: 0, top: 0, height: 48, active: true, skin: THEME.lineSkin,
	behavior: SCREEN.ListItemBehavior({
		onComplete: function(container) {
			this.readRSSI(container);
		},
		onCreate: function(container, data) {
			SCREEN.ListItemBehavior.prototype.onCreate.call(this, container, data);
			this.peripheral = this.data.peripheral;
		},
		onDisplaying: function(container) {
			this.setRSSI(this.peripheral.rssi);
		},
		onPeripheralChanged: function(container, peripheral) {
			if (peripheral.address == this.peripheral.address) {
				this.data.NAME.string = peripheral.name;
				this.setRSSI(peripheral.rssi);
			}
		},
		onPeripheralRemoved: function(container, peripheral) {
			if (peripheral.address == this.peripheral.address) {
				container.cancel();
			}
		},
		onRSSIChanged: function(container, connection, rssi) {
			if (connection == this.peripheral.connection) {
				this.setRSSI(rssi);
				container.wait(1000);
			}
		},
		onScanStart: function(container) {
			// Kick off reading RSSI value for connected peripheral
			if (null != this.peripheral.connection)
				container.wait(1000);
		},
		readRSSI: function(container) {
			var peripheral = this.peripheral;
			var params = {connection: peripheral.connection};
			Pins.invoke("/ble/gapReadRSSI", params);
		},
		setRSSI: function(rssi) {
			this.data.RSSI.string = rssi;
			var level = this.data.LEVEL;
			if (rssi < -84) level.variant = 1;
			else if (rssi < -81) level.variant = 2;
			else if (rssi < -78) level.variant = 3;
			else if (rssi < -75) level.variant = 4;
			else if (rssi < -72) level.variant = 5;
			else if (rssi < -69) level.variant = 6;
			else if (rssi < -66) level.variant = 7;
			else if (rssi < -63) level.variant = 8;
			else if (rssi < -60) level.variant = 9;
			else if (rssi < -58) level.variant = 10;
			else if (rssi < -55) level.variant = 11;
			else level.variant = 12;
		}
	}),
	contents: [
		Line($, {
			left: 2, right: 0, top: 0, bottom: 0,
			contents: [
				Container($, {
					anchor: "LEVEL", left: 5, width: 40, height: 40, skin: rssiSkin,
					contents: [
						Label($, {anchor: "RSSI", left: 0, right: 0, style: rssiStyle})
					]
				}),
				Column($, {
					left: 0, right: 0,
					contents: [
						Label($, {anchor: "NAME", left: 10, right: 0, style: listItemStyle, string: $.peripheral.name}),
						Label($, {anchor: "ADDRESS", left: 10, right: 0, style: listAddressStyle, string: $.peripheral.address}),
					]
				}),
				Content($, {right: 0, top: 0, bottom: 0, skin: listArrowSkin}),
			]
		}),
		Content($, {left: 0, right: 0, bottom: 0, height: 1, skin: dividerLineSkin})
	]
}});

var DevicesScreen = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: whiteSkin,
	behavior: Behavior({
		onBackButton: function(container) {
			application.invoke(new Message("xkpr://shell/close?id=" + application.id));
		},
	}),
	contents: [
		CREATIONS.DynamicHeader($),
		Container($, {
			left: 0, right: 0, top: 32, bottom: 0, behavior: BLEDeviceDiscoveryBehavior,
			contents: [
				SCROLLER.VerticalScroller($, {
					clip: true,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							behavior: SCREEN.ListBehavior({
								addBusyLine: function(list) {
									list.add(new ListEmptyItem("Scanning..."));
								},
								addItemLine: function(list, item, index) {
									list.add(new DeviceListItem(item));
								},
								addLines: function(list, items, more) {
									for (var i = 0, c = items.length; i < c; ++i) {
										this.addItemLine(list, items[i], i);
									}
									if (0 == list.length)
										this.addBusyLine(list);
								},
								onCreate: function(list, data) {
									this.data = data;
									this.reload(list);
								},
								onPeripheralAdded: function(list, peripheral) {
									this.reload(list);
								},
								onPeripheralRemoved: function(list, peripheral) {
									this.reload(list);
									return true;
								},
								reload: function(list) {
									list.cancel();
									list.empty();
									this.data.items.length = [];
									for (var address in model.data.peripherals) {
										var peripheral = model.data.peripherals[address];
										var item = {
											peripheral: peripheral,
											action: "/services?" + serializeQuery({address: peripheral.address})
										}
										this.data.items.push(item);
									}
									this.addLines(list, this.data.items, false);
								}
							}),
						}),
						SCROLLER.VerticalScrollbar($),
						SCROLLER.BottomScrollerShadow($)
					]
				}),
			],
		})
	]
}});

// ----------------------------------------------------------------------------------
// Application
// ----------------------------------------------------------------------------------

var ApplicationBehavior = MODEL.ApplicationBehavior.template({
	onBLENotification: function(application, response) {
		var notification = response.notification;
		
		if ("gap/discover" == notification) {
			var peripheral = response;
			application.distribute("onPeripheralDiscovered", peripheral);
		}
		else if ("gap/connect" == notification) {
			var connection = response.connection;
			application.distribute("onPeripheralConnected", connection);
		}
		else if ("gap/disconnect" == notification) {
			var connection = response.connection;
			application.distribute("onPeripheralDisconnected", connection);
		}
		else if ("gap/rssi" == notification) {
			var connection = response.connection;
			var rssi = response.rssi;
			application.distribute("onRSSIChanged", connection, rssi);
		}
		else if ("gatt/service" == notification) {
			application.distribute("onGattServiceFound", response);
		}
		else if ("gatt/characteristic" == notification) {
			application.distribute("onGattCharacteristicFound", response);
		}
		else if ("gatt/characteristic/value" == notification) {
			application.distribute("onGattCharacteristicValueFound", response);
		}
		else if ("gatt/request/complete" == notification) {
			application.distribute("onGattRequestCompleted", response);
		}
		else if ("system/reset" == notification) {
			application.distribute("onSystemReset", response);
		}
		else {
			trace("Unhandled notification: " + JSON.stringify(response) + "\n");
		}
	},
	onLaunch: function(application) {
        Pins.configure({
            ble: {
                require: "/lowpan/ble",
			}
		}, function (success) {
			if (!success) return;
			
			application.style = applicationStyle;
			
			this.data = {
				title: "BLE Explorer",
				gatt_services: Files.readJSON(mergeURI(application.url, "./services.json")),
				peripherals: {}
			};
		
			Pins.when("ble", "notification", this.onBLENotification.bind(this, application));

			// Load the screen configured by /main
			MODEL.ApplicationBehavior.prototype.onLaunch.call(this);
		}.bind(this));
    },
	onPeripheralDisconnected: function(application, connection) {
		// Remove disconnected peripheral from peripheral list
		for (var address in this.data.peripherals) {
			var peripheral = this.data.peripherals[address];
			if (peripheral.connection == connection) {
				delete this.data.peripherals[address];
				
				// Go home
				application.invoke(new Message("/home"));
			
				// Let everyone know
				application.distribute("onPeripheralRemoved", peripheral);
				break;
			}
		}
	}
});

var model = application.behavior = new ApplicationBehavior(application);
