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

function config(http, args)
{
	switch (args.path[1]) {
	case "pins":
		var Pins = require.weak("pins");
		var params = JSON.parse(String.fromArrayBuffer(http.content));
		trace("config/pins: setting " + String.fromArrayBuffer(http.content) + "\n");
		Pins.invoke("setPinMux", params);
		break;
	case "hostname":
		System.hostname = String.fromArrayBuffer(http.content);
		break;
	case "timeserver":
		var Environment = require.weak("env");
		var env = new Environment();
		env.set("TIME_SERVER", String.fromArrayBuffer(http.content));
		env.save();
		break;
	case "timezone":
		System.timezone = JSON.parse(String.fromArrayBuffer(http.content));
		break;
	}
	http.response();
}

export default {
	onLaunch(http, args) {
		config(http, args);
	},
};
