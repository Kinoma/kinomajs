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
const spinnerSkin = new Skin({ texture:new Texture("assets/spinner.png", 1), x:0, y:0, width:80, height:80, variants:80 });
const waitSkin = new Skin({ texture:new Texture("assets/wait.png", 2), x:0, y:0, width:20, height:20, variants:20 });

import tool from "shell/tool";

import { 
	model,
} from "shell/main";

export class HelperBehavior extends Behavior {
	onDeviceHelperDown(container, device) {
		trace("onDeviceHelperDown -> doClose\n");
		container.bubble("doClose");
	}
	onDeviceHelperUp(container) {
	}
	onDeviceSelected(container, device) {
		if (this.data.device != device) {
			trace("onDeviceSelected -> doClose\n");
			container.bubble("doClose");
		}
	}
	onDisplayed(container) {
		var device = model.devicesFeature.currentDevice;
		if (!device.ws && device.helperProject) {
			device.helperHost = device.debugHost;
			tool.execute("run", model.devicesFeature.currentDevice, device.helperProject, model.debugFeature);
		}
		else {
			this.onDeviceHelperUp(container);
		}
	}
}

export class SpinnerBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
		content.duration = 125;
		content.variant = 0;
	}
	onDisplayed(content) {
		content.start();
	}
	onFinished(content) {
		var variant = content.variant + 1;
		if (variant == 24) variant = 0;
		content.variant = variant;
		content.time = 0;
		content.start();
	}
}

export class ValueBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
	}
	onDisplaying(content) {
		this.onUpdate(content);
	}
	onUpdate(content) {
		debugger
	}
}

export const SpinnerContent = Content.template($ => ({ left:0, right:0, width:80, height:80, skin:spinnerSkin, Behavior: SpinnerBehavior }));
export const WaitContent = Content.template($ => ({ width:20, height:20, skin:waitSkin, Behavior: SpinnerBehavior }));
