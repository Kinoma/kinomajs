<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
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
<package script="true">
	<program>
		Crypt = new Proxy({}, {
			get: function(target, key, receiver) {
				if ("rng" == key) {
					let rng = require.weak("crypt/rng");
					return rng.get;
				}

				//@@ rename modules to eliminate need for this mapping
				if ("SRPServer" == key)
					key = "srp";
				else if ("Ed25519" == key)
					key = "ed25519_c";
				else if ("StreamCipher" == key)
					key = "stream";
				else if ("BlockCipher" == key)
					key = "cipher";

				return require.weak("crypt/" + key.toLowerCase());
			},
			set: function() {
				throw new Error("Crypt is read-only");
			}
		});

		Arith = new Proxy({}, {
			get: function(target, key, receiver) {
				switch (key) {
				case "Integer": key = "int"; break;
				case "Z": key = "z"; break;
				case "Module": key = "mod"; break;
				case "Mont": key = "mont"; break;
				case "EC": key = "ec"; break;
				case "ECPoint": key = "ecp"; break;
				case "Ed": key = "ed"; break;
				}
				return require.weak("arith/" + key);
			},
			set: function() {
				throw new Error("Arith is read-only");
			},
		});
	</program>
</package>
