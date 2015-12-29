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
import System from "system";

const _GPIO = 1 << 4;
const _I2C = 2 << 4;
const _UART = 3 << 4;
const _A2D = 4 << 4;
const _GPT = 5 << 4;

const iodir = "io/";

var GPIOPin = {
	// GPIO functions
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

	// event type
	RISING_EDGE: 0x01,
	FALLING_EDGE: 0x02,

	led(n, on) {
		if (System.device != "K5")
			return;
		var pin;
		switch (n) {
		case 0: pin = 40; break;
		case 1: pin = 41; break;
		default: return;
		}
		this.write(pin, !on);
	},
	enable_leds(){
		if (System.device == "K5") {
			this.pinmux([[40, GPIOPin.GPIO_OUT], [41, GPIOPin.GPIO_OUT]]);
		}
	},
	_pinmap(a) {
		var pin = a[0];
		var gfunc = a[1], pfunc = -1;
		var opt;
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
				if ([0, 1, 2, 3, 23, 24, 27, 30, 31, 32, 33, 37].indexOf(pin) != -1)
					opt = 0;	// UART0_ID
				else if ([11, 12, 13, 14, 35, 36, 38, 39, 42, 43, 44, 45].indexOf(pin) != -1)
					opt = 1;	// UART1_ID
				else if ([7, 8, 9, 10, 46, 47, 48, 49].indexOf(pin) != -1)
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
		pinarray.forEach(function(e) {
			var pin = e[0];
			if (this._pins[pin])
				this._pins[pin].close();
		}, this);
		this._pinmux(this.pinmap(pinarray));
	},
	_pinmux(pinarray) @ "xs_pinmux",
	write(pin, val) @ "xs_pin_write",
	read(pin) @ "xs_pin_read",
	_newEvent(pin, type, f) @ "xs_pin_newEvent",
	_pins: [],
	event(pin, type, f) {
		if (type && f) {
			if (this._pins[pin])
				this._pins[pin].close();
			this.pinmux([[pin, GPIOPin.GPIO_IN]]);
			this._pins[pin] = this._newEvent(pin, type, f);
		}
		else
			this.close(pin);
	},
	a2d(o) {
		var A2D = require.weak(iodir + "A2D");
		var pin = o.pin;
		if (this._pins[pin])
			this._pins[pin].close();
		this._pinmux([[pin, this.A2D_IN]]);
		return this._pins[pin] = new A2D(0, pin - 42);	// only ID=0 is available, convert the pin number to the channel number
	},
	i2c(o) {
		var I2C = require.weak(iodir + "I2C");
		var sda = o.sda, scl = o.scl, addr = o.addr;
		if (this._pins[sda]) {
			if (!(this._pins[sda] instanceof I2C))
				this._pins[sda].close();
		}
		if (this._pins[scl]) {
			if (!(this._pins[scl] instanceof I2C))
				this._pins[scl].close();
		}
		var pinmap = this.pinmap([[sda, this.I2C_SDA], [scl, this.I2C_SCL]]);
		if (pinmap[0][2] != pinmap[1][2])
			// different port
			return;
		this._pinmux(pinmap);
		return this._pins[sda] = this._pins[scl] = new I2C(pinmap[0][2], addr);
	},
	uart(o) {
		var Serial = require.weak(iodir + "Serial");
		var rx = o.rx, tx = o.tx, baud = o.baud;
		if (this._pins[rx])
			this._pins[rx].close();
		if (this._pins[tx])
			this._pins[tx].close();
		var pinmap = this.pinmap([[rx, this.UART_RXD], [tx, this.UART_TXD]]);
		if (pinmap[0][2] != pinmap[1][2])
			// different port
			return;
		this._pinmux(pinmap);
		return this._pins[rx] = this._pins[tx] = new Serial(pinmap[0][2], baud);
	},
	gpt(o) {
		var GPT = require.weak(iodir + "GPT");
		var pin = o.pin;
		if (this._pins[pin])
			this._pins[pin].close();
		var pinmap = this.pinmap([[pin, this.GPT_IO]]);
		this._pinmux(pinmap);
		var opt = pinmap[0][2];
		return this._pins[pin] = new GPT(opt[0] /* timer ID */, opt[1] /* channel */);
	},
	close(pin) {
		if (this._pins[pin]) {
			this._pins[pin].close();
			delete this._pins[pin];
		}
	},
	_init() @ "xs_pin_init",
};

GPIOPin._init();

export default GPIOPin;
