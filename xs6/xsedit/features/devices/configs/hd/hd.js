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
import {
	model,
} from "shell/main";
import { 
	DeviceConfig, 
	updateCredentials 
} from "features/devices/devices";

export default class HD extends DeviceConfig {
	constructor(devices, discovery) {
		super(devices, discovery);
	}
}

HD.iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:60, height:60 });
HD.id = "com.marvell.kinoma.launcher.hd";
HD.product = "Kinoma HD";
HD.tag = "HD";
HD.url = this.uri;
