/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
import Dial from "dial";
import Launcher from "launcher";
import Pins from "pins";

class PinExplorer extends Dial {
	doConfigure(configuration) {
		trace("from: " + JSON.stringify(configuration, true) + "\n");
		
		let explorer = {};
		for (let i in configuration) {
			let pins = configuration[i].pins;
			for (let j in pins) {
				let mod = pins[j];
				let type = mod.type;
				let id;
				switch (type) {
				case "Analog":
					id = type + mod.pin;
					explorer[type + mod.pin] = {
						pins: { analogInput: mod },
						require: "dial/pinexplorer/Analog",
					};
				break;
				case "Digital":
					explorer[type + mod.pin] = {
						pins: { digital: mod },
						require: "dial/pinexplorer/Digital",
					};
				break;
				case "Ground":
					explorer[type + mod.pin] = {
						pins: { ground: mod },
						require: "dial/pinexplorer/Ground",
					};
				break;
				case "I2C":
					explorer[type + " sda " + mod.sda + " scl " + mod.clock] = {
						pins: { i2c: mod },
						require: "dial/pinexplorer/I2C",
					};
				break;
				case "Power":
					explorer[type + mod.pin] = {
						pins: { power: mod },
						require: "dial/pinexplorer/Power",
					};
				break;
				case "PWM":
					explorer[type + mod.pin] = {
						pins: { pwm: mod },
						require: "dial/pinexplorer/PWM",
					};
				break;
				case "Serial":
					explorer[type + " rx " + mod.rx + " tx " + mod.tx] = {
						pins: { serial: mod },
						require: "dial/pinexplorer/Serial",
					};
				break;
				}
			}
		}
		trace("to: " + JSON.stringify(explorer, true) + "\n");
		Pins.share();
		Pins.close();
		Pins.configure(explorer, function(success) {
			Pins.share({type: "http"}, {zeroconf: true, name:"PinExplorer"});
			var handlers = Pins.handlers;
			if (handlers.length > 0) {
				var e = handlers[0];
				pinexplorer.additionalData = '\n<additionalData><pins>"' + e._protocol + '://*:' + e._port + '/"</pins></additionalData>';
			}
		});
	}
	
	onConfiguration(configuration, args) {
		Launcher.run(this, [ args ]);
		this.doConfigure(configuration);
	}
	
	start(args) {
 		Pins.invoke("configuration", configuration => this.onConfiguration(configuration, args));
	}
}

var pinexplorer = new PinExplorer;
export default pinexplorer;
