<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package script="true">
	<import href="kpr.xs" link="dynamic"/>
	
	<object name="PINS">
		<object name="behaviors"/>
		<object name="constructors"/>
		<object name="prototypes"/>
		<function name="configure" params="configurations">
			var constructors = this.constructors;
			var behaviors = {};
			configurations = configurations.sandbox;
			for (var i in configurations) {
				var configuration = configurations[i].sandbox;
				var defaults;
                if (!("require" in configuration))
                    throw new Error("require property missing in configuration " + i + ": " + JSON.stringify(configuration));
				var prototype = require(configuration.require);
				var behavior = Object.create(prototype);
				behavior.sandbox.id = i;
				if ("pins" in behavior.sandbox)
					defaults = behavior.sandbox.pins.sandbox;
				var pins = configuration.pins.sandbox;
				for (var j in pins) {
					var pin = pins[j].sandbox;
					if (defaults && (j in defaults))
						this.merge(defaults[j].sandbox, pin);
                    var type = pin.type;
                    if (!type)
                        throw new Error("Pin " + j + " missing type: " + JSON.stringify(pin));
                    if (!(type in constructors))
                        throw new Error("Unsupported pin type on pin " + j + ": " + JSON.stringify(pin));
					behavior.sandbox[j] = new (constructors[type])(pin);
				}
				if ("configure" in behavior.sandbox)
					behavior.sandbox.configure(configuration);
				behaviors.sandbox[i] = behavior;
			}
			this.behaviors = behaviors;
		</function>
		<function name="close">
			var behaviors = this.behaviors.sandbox;
			for (var i in behaviors) {
				var behavior = behaviors[i];
				if ("close" in behavior.sandbox)
					behavior.sandbox.close();
			}
		</function>
		<function name="create" params="pin">
			pin = pin.sandbox;
			return new (this.constructors[pin.type])(pin);
		</function>
		<function name="merge" params="defaultPin, configurationPin">
			for (var i in defaultPin) {
				if (!(i in configurationPin))
					configurationPin[i] = defaultPin[i];
			}
		</function>
	</object>
	
	<patch prototype="KPR.message">
		<function name="get requestObject" c="KPR_message_get_requestObject"/>
		<function name="set requestObject" c="KPR_message_set_requestObject"/>
	</patch>
	<function name="MessageWithObject" params="url, object" prototype="KPR.message">
		Message.call(this, url);
		this.requestObject = object;
		this.setRequestHeader("referrer", "xkpr://" + (application ? application.id : shell.id));
	</function>
	<import href="a2d.xs"/>
	<import href="audioIn.xs"/>
	<import href="gpio.xs"/>
	<import href="i2c.xs"/>
<!--
	<import href="k4.xs"/>
-->
	<import href="pwm.xs"/>
	<import href="serial.xs"/>
	<import href="sensorUtils.xs"/>
<!--
	<import href="spi.xs"/>
-->
	
	<import href="mockup.xs"/>
</package>