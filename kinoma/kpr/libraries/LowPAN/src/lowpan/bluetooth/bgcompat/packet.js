//@module
// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

/** Require from io-utils */
var Utils = require("../../common/utils");
var Buffers = require("../../common/buffers");
var BTUtils = require("../core/btutils");

function Packet(dataType, data, byteOrder) {

  this.typeFlag = dataType;
  this.raw = data;
  this.byteOrder = byteOrder;

  // Grab type from data structure
  var type = Packet.ADTypes[dataType];

  // If it exists
  if (type) {
    // Set name
    this.type = type.name;
    // Set data based on the best way to resolve the data (ie toString, to hex array)
    this.data = type.resolve ? type.resolve(data, dataType, byteOrder) : data;
  }
  else {
    // Set defaults if type not known
    this.type = "Unknown";
    this.data = data;
  }
}

// Data structure
Packet.ADTypes = {
    0x01 : { name : "Flags", resolve: toStringArray },
    0x02 : { name : "Incomplete List of 16-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 2)},
    0x03 : { name : "Complete List of 16-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 2) },
    0x04 : { name : "Incomplete List of 32-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 4) },
    0x05 : { name : "Complete List of 32-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 4) },
    0x06 : { name : "Incomplete List of 128-bit Service Class UUIDs", resolve: toUUIDStringArray.bind(null, 16) },
    0x07 : { name : "Complete List of 128-bit Service Class UUIDs", resolve: toUUIDStringArray.bind(null, 16) },
    0x08 : { name : "Shortened Local Name", resolve: toString },
    0x09 : { name : "Complete Local Name", resolve: toString },
    0x0A : { name : "Tx Power Level", resolve: toSignedInt },
    0x0D : { name : "Class of Device", resolve: toOctetString.bind(null, 3) },
    0x0E : { name : "Simple Pairing Hash C", resolve: toOctetString.bind(null, 16) },
    0x0F : { name : "Simple Pairing Randomizer R", resolve: toOctetString.bind(null, 16) },
    0x10 : { name : "Device ID", resolve: toOctetString.bind(null, 16) },
    // 0x10 : { name : "Security Manager TK Value", resolve: null }
    0x11 : { name : "Security Manager Out of Band Flags", resolve : toOctetString.bind(null, 16) },
    0x12 : { name : "Slave Connection Interval Range", resolve : toOctetStringArray.bind(null, 2) },
    0x14 : { name : "List of 16-bit Service Solicitation UUIDs", resolve : toOctetStringArray.bind(null, 2) },
    0x1F : { name : "List of 32-bit Service Solicitation UUIDs", resolve : toOctetStringArray.bind(null, 4) },
    0x15 : { name : "List of 128-bit Service Solicitation UUIDs", resolve : toUUIDStringArray.bind(null, 8) },
    0x16 : { name : "Service Data", resolve : toOctetStringArray.bind(null, 1) },
    0x17 : { name : "Public Target Address", resolve : toOctetStringArray.bind(null, 6) },
    0x18 : { name : "Random Target Address", resolve : toOctetStringArray.bind(null, 6) },
    0x19 : { name : "Appearance" , resolve : null },
    0x1A : { name : "Advertising Interval" , resolve : toOctetStringArray.bind(null, 2)  },
    0x1B : { name : "LE Bluetooth Device Address", resolve : toOctetStringArray.bind(null, 6) },
    0x1C : { name : "LE Role", resolve : null },
    0x1D : { name : "Simple Pairing Hash C-256", resolve : toOctetStringArray.bind(null, 16) },
    0x1E : { name : "Simple Pairing Randomizer R-256", resolve : toOctetStringArray.bind(null, 16) },
    0x20 : { name : "Service Data - 32-bit UUID", resolve : toOctetStringArray.bind(null, 4) },
    0x21 : { name : "Service Data - 128-bit UUID", resolve : toUUIDStringArray.bind(null, 16) },
    0x3D : { name : "3D Information Data", resolve : null },
    0xFF : { name : "Manufacturer Specific Data", resolve : null },
}

// Converts data flags to an array of readable strings
function toStringArray(data) {
  var arr = [];

  if (!data) {
    arr.push('None');
  }
  else {
    var flags = data[0];
    if (flags & (1 << 0)) {
      arr.push('LE Limited Discoverable Mode');
    }
    if (flags & (1 << 1)) {
      arr.push('LE General Discoverable Mode');
    }
    if (flags & (1 << 2)) {
      arr.push('BR/EDR Not Supported');
    }
    if (flags & (1 << 3)) {
      arr.push('Simultaneous LE and BR/EDR to Same Device Capable (Controller)');
    }
    if (flags & (1 << 4)) {
      arr.push('Simultaneous LE and BR/EDR to Same Device Capable (Host)');
    }
    if (!arr.length) {
      arr.push('None');
    }
  }

  return arr;
}

// Converts buffer to array of strings
// (some uuids are 128 bits which we can't represent as numbers b/c it's too big
// and I don't want the user to have to worry about endian-ness)
function toOctetStringArray(numBytes, data, dataType, byteOrder) {
  var uuids = [];

  var bytes;

  // If data is undefined, returne empty array
  if (!data) return [];

  // Make sure there are enough bytes
  if (numBytes > data.length) {
    throw new Error("Not enough bytes for single UUID");
  }

  // Make sure it will divide nicely
  if (data.length % numBytes) {
    throw new Error("Not enough bytes to complete each UUID. Needs to be multiple of " + numBytes.toString());
  }

  // Go through the array
  while (data) {

    // grab an octet string
    var uuid = toOctetString(numBytes, data, dataType, byteOrder);

    // Put into the array
    uuids.push(uuid);

    // Move the buffer forward
    data = data.slice(numBytes, data.length);

    // Weird hack
    if (!data || !data.length) data = null;
  }

  return uuids;
}

function toUUIDStringArray(numBytes, data, dataType, byteOrder) {
  var buffer = Buffers.ByteBuffer.wrap(data);
  var uuids = [];
  while (buffer.remaining() > 0) {
    uuids.push(BTUtils.UUID.getByUUID(buffer.getByteArray(numBytes)).toString());
  }
  return uuids;
}

// Simply prints buffer, utf8 encoded
function toString(data) {
  var s = "";
  for (var i = 0; i < data.length; i++) {
    s += String.fromCharCode(data[i]);
  }
  return s;
}

// Only uses for signal strength
function toSignedInt(data) {
  if (!data) return 0;
  return data[0];
}

// converts buffer to array of uuid strings
function toOctetString(numOctets, data, dataType, byteOrder) {
  var str = "";
  for (var i = 0; i < numOctets; i++) {
    if (byteOrder == "LE") {
      str = Utils.toHexString(data[i], 1, "") + str;
    } else {
      str = str + Utils.toHexString(data[i], 1, "");
    }
  }
  return str;
}

module.exports.Packet = Packet;
module.exports.toOctetStringArray = toOctetStringArray;
