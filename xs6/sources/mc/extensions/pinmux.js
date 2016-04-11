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
import System from "system";

const DISCONNECTED = 0;
const _GPIO = 1 << 4;
const _I2C = 2 << 4;
const _UART = 3 << 4;
const _A2D = 4 << 4;
const _GPT = 5 << 4;

let GPIOPin = {
	// GPIO functions
	DISCONNECTED: 0,
	GPIO_IN: _GPIO | 0,
	GPIO_OUT: _GPIO | (1 << 8),
	I2C_SDA: _I2C | 0,
	I2C_SCL: _I2C | 1,
	UART_CTS: _UART | 0,
	UART_RTS: _UART | 1,
	UART_TXD: _UART | 2,
	UART_RXD: _UART | 3,
	A2D_IN: _A2D,
	GPT_IO: _GPT,
	path: "io/",

	// event type
	RISING_EDGE: 0x01,
	FALLING_EDGE: 0x02,

	_pinmap(a) {
		let pin = a[0];
		let gfunc = a[1], pfunc = -1;
		let opt;
		switch (System.device) {
		case "MW300":
		case "K5":
			switch (gfunc & 0xf0) {
			case _GPIO:
				opt = gfunc == this.GPIO_IN ? this.GPIO_INPUT : this.GPIO_OUTPUT;	// direction
				if ([6, 7, 8, 9, 10, 22, 23, 24, 25, 26, 28, 29, 30, 31, 32, 33].indexOf(pin) != -1)
					pfunc = this.PINMUX_FUNCTION_1;
				else
					pfunc = this.PINMUX_FUNCTION_0;
				break;
			case _I2C:
				if (gfunc == this.I2C_SDA) {
					if ([4, 6, 18, 20, 25, 28].indexOf(pin) != -1)
						pfunc = this.PINMUX_FUNCTION_2;
					else if (pin == 7)
						pfunc = this.PINMUX_FUNCTION_3;
					else if (pin == 9)
						pfunc = this.PINMUX_FUNCTION_4;
				}
				else {
					if ([5, 17, 19, 21, 26, 29].indexOf(pin) != -1)
						pfunc = this.PINMUX_FUNCTION_2;
					else if (pin == 8 || pin == 10)
						pfunc = this.PINMUX_FUNCTION_4;
				}
				// port
				if ([4, 5, 20, 21, 28, 29].indexOf(pin) != -1)
					opt = 0;
				else if ([6, 9, 10, 17, 18, 19, 25, 26].indexOf(pin) != -1)
					opt = 1;
				break;
			case _A2D:
				if (pin >= 42 && pin <= 49)
					pfunc = this.PINMUX_FUNCTION_1;
				else
					pfunc = -1;
				break;
			case _UART:
				pfunc = this.PINMUX_FUNCTION_2;
				// port
				if ([2, 3, 24, 27, 32, 33].indexOf(pin) != -1)
					opt = 0;	// UART0_ID
				else if ([13, 14, 38, 39, 44, 45].indexOf(pin) != -1)
					opt = 1;	// UART1_ID
				else if ([9, 10, 48, 49].indexOf(pin) != -1)
					opt = 2;	// UART2_ID
				break;
			case _GPT:
				if ([0, 1, 2, 3, 4, 5].indexOf(pin) != -1) {		// GPT0
					opt = [0, pin];
					pfunc = this.PINMUX_FUNCTION_1;
				}
				else if ([11, 12, 13, 14, 15].indexOf(pin) != -1) {	// GPT2
					opt = [2, pin - 11];
					pfunc = this.PINMUX_FUNCITON_1;
				}
				else if ([17, 18, 19, 20, 21].indexOf(pin) != -1) {	// GPT3
					opt = [3, pin - 17];
					pfunc = this.PINMUX_FUNCTION_1;
				}
				else if (pin == 24) {					// GPT1
					opt = [1, 5];
					pfunc = this.PINMUX_FUNCTION_3;
				}
				else if ([28, 29, 30, 31, 32, 33].indexOf(pin) != -1) {	// GPT1
					opt = [1, pin - 28];
					pfunc = this.PINMUX_FUNCTION_5;
				}
				else if (pin == 34) {					// GPT3
					opt = [3, 5];
					pfunc = this.PINMUX_FUNCTION_1;
				}
				else if (pin == 37) {					// GPT2
					opt = [2, 5];
					pfunc = this.PINMUX_FUNCTION_1;
				}
				break;
			case DISCONNECTED:
			default:
				return this._pinmap([pin, this.GPIO_IN]);	// set all disconnected pins to GPIO_IN
			}
			break;
		default:
			// unsupported device
			break;
		}
		return [pin, pfunc, opt];
	},

	pinmap(pinarray) {
		return pinarray.map(e => this._pinmap(e));
	},
	pinmux(pinarray) {
		if (!pinarray) {
			this._pins.forEach(e => {
				if (e.instance)
					e.instance.close();
			});
			this._pins.length = 0;
			return;
		}
		pinarray.forEach(e => {
			let pin = e[0];
			this.close(pin);
			this._pins[pin] = {func: e[1]};
		});
		let pinmap = this.pinmap(pinarray);
		this._pinmux(pinmap);
		return pinmap;
	},
	getPinmux() {
		return this._pins.map(e => e.func);
	},
	_pinmux(pinarray) @ "xs_pinmux",
	write(pin, val) @ "xs_pin_write",
	read(pin) @ "xs_pin_read",
	_pins: System.pins,
	event(pin, type, f) {
		if (type && f) {
			let newEvent = require.weak("pinmux_event");
			this.pinmux([[pin, GPIOPin.GPIO_IN]]);
			this._pins[pin].instance = newEvent(pin, type, f);
		}
		else
			this.close(pin);
	},
	a2d(o) {
		let A2D = require.weak(this.path + "A2D");
		let pin = o.pin;
		this.pinmux([[o.pin, this.A2D_IN]]);
		return this._pins[pin].instance = new A2D(0, pin - 42);	// only ID=0 is available, convert the pin number to the channel number
	},
	i2c(o) {
		let I2C = require.weak(this.path + "I2C");
		let sda = o.sda, scl = o.scl, addr = o.addr;
		let pinmap = this.pinmux([[sda, this.I2C_SDA], [scl, this.I2C_SCL]]);
		if (pinmap[0][2] != pinmap[1][2])
			// different port
			return;
		return this._pins[sda].instance = this._pins[scl].instance = new I2C(pinmap[0][2], addr);
	},
	uart(o) {
		let Serial = require.weak(this.path + "Serial");
		let rx = o.rx, tx = o.tx, baud = o.baud;
		let pinmap = this.pinmux([[rx, this.UART_RXD], [tx, this.UART_TXD]]);
		if (pinmap[0][2] != pinmap[1][2])
			// different port
			return;
		return this._pins[rx].instance = this._pins[tx].instance = new Serial(pinmap[0][2], baud);
	},
	gpt(o) {
		let GPT = require.weak(this.path + "GPT");
		let pin = o.pin;
		let pinmap = this.pinmux([[pin, this.GPT_IO]]);
		let opt = pinmap[0][2];
		return this._pins[pin].instance = new GPT(opt[0] /* timer ID */, opt[1] /* channel */);
	},
	close(pin) {
		if (this._pins[pin] && this._pins[pin].instance) {
			this._pins[pin].instance.close();
			delete this._pins[pin].instance;
		}
	},
	_init() @ "xs_pin_init",
};

GPIOPin._init();
delete GPIOPin._init;

export default GPIOPin;
