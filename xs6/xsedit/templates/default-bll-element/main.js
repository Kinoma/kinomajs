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
import Pins from "pins";

let main = {
	onLaunch(){
		Pins.configure({
			sensor: {
				require: "bll",
				pins: {
					// Specify the pins required by this BLL
				}
			},
		}, function(success) {
			if (success) {
				// After the pins have been configured, issue single/repeated commands to the BLLs
			} else {
				trace("Failed to configure pins.\n");
			}
		});
	}
};

export default main;