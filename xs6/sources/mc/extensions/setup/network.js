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
import Wifi from "wifi";

function process(http, args)
{
	try {
		if (http.method == "GET") {
			let json = { "ip":Wifi.ip, "mac":Wifi.mac, "ssid":Wifi.ssid, "level":Wifi.rssi };
			http.response("application/json", JSON.stringify(json));
		}
		else if (http.method == "PUT") {
			http.response();
 			Wifi.connect(args.query);
		}
		else
			http.errorResponse(405, "Method Not Allowed");
	} catch(error) {
		http.errorResponse(505, "Internal Server Error");
	}
}

export default {
	onLaunch(http, args) {
		process(http, args);
	},
};
