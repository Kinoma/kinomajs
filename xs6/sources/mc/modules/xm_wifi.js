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
var Wifi = {
	init() @ "xs_wm_init",
	fin() @ "xs_wm_fin",
	connect(params) @ "xs_wm_connect",
	disconnect() @ "xs_wm_disconnect",
	get ip() @ "xs_wm_getIP",
	get mac() @ "xs_wm_getMAC",
	set mac(addr) {
		if (typeof addr == "string") {
			var Arith = require.weak("arith");
			this._setmac((new Arith.Integer("0x" + addr)).toChunk(6));
		}
		else
			this._setmac(addr);
	},
	_setmac(addr) @ "xs_wm_setMAC",
	get rssi() @ "xs_wm_getRSSI",
	scan(rescan) @ "xs_wm_scan",
	save() @ "xs_wm_save",
	get status() @ "xs_wm_get_status",
	get mode() @ "xs_wm_get_mode",
	get config() @ "xs_wm_get_config",
	get ssid() @ "xs_wm_get_ssid",
	getInterfaces() @ "xs_wm_getInterfaces",
	stat() @ "xs_wm_stats_sockets",
};
export default Wifi;
