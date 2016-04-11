/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

export default class Telnetd @ "xs_telnetd_destructor" {
	constructor(sock) @ "xs_telnetd_start";
	close() @ "xs_telnetd_close";
	evaluate(line) {
		let CLI = require.weak("CLI");
		CLI.evaluate(line);
	}
};
