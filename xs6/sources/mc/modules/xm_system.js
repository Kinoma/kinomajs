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
var System = {
	init() @ "xs_system_init",
	fin() @ "xs_system_fin",
	get osVersion() @ "xs_system_get_osVersion",
	get device() @ "xs_system_get_device",
	get platform() @ "xs_system_get_platform",
	get hostname() @ "xs_system_get_hostname",
	set hostname(it) @ "xs_system_set_hostname",
	get time() @ "xs_system_get_time",
	set time(t) @ "xs_system_set_time",
	get timezone() @ "xs_system_get_timezone",
	set timezone(tz) @ "xs_system_set_timezone",
	get timestamp() @ "xs_system_get_timestamp",
	_init_rng(mac) @ "xs_system_init_rng",
	rng(n) @ "xs_system_get_rng",
	load(p) @ "xs_system_load",
	sleep(n) @ "xs_system_sleep",
	gc() @ "xs_system_gc",
	run() @ "xs_system_run",
	addPath(path) @ "xs_system_addPath",
	pm(level, timeout) @ "xs_system_pm",
	reboot(force) @ "xs_system_reboot",
	shutdown(force) @ "xs_system_shutdown",
	console(flag) @ "xs_system_console",
};

System.init();

export default System;
