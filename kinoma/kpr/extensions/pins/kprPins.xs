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
		<object name="prototypes">
			<object name="ground">
				<number name="pin"/>
				<function name="init"/>
				<function name="close"/>
			</object>
			<object name="power">
				<number name="pin"/>
				<number name="voltage"/>
				<function name="init"/>
				<function name="close"/>
			</object>
			<object name="notification" c="xs_notification">
                <function name="init"/>
                <function name="close" params="" c="xs_notification_close" />
                <function name="repeat" params="" c="xs_notification_repeat" />
                <function name="invoke" params="" c="xs_notification_invoke" />
			</object>
		</object>
		<null name="config"/>
		<object name="constructors">
			<function name="Ground" params="it" prototype="PINS.prototypes.ground">
                if (!("pin" in it))
                    it = it.sandbox;
				this.pin = it.pin;
			</function>
			<function name="Power" params="it" prototype="PINS.prototypes.power">
                if (!("pin" in it))
                    it = it.sandbox;
				this.pin = it.pin;
				this.voltage = ("voltage" in it) ? it.voltage : 3.3;
			</function>
			<function name="Notification" params="it" prototype="PINS.prototypes.notification"/>
		</object>
		<function name="configure" params="configurations">
			PINS.config = this.configure_aux(configurations, true);
		</function>
		<function name="close">
			var behaviors = this.behaviors.sandbox;
			for (var i in behaviors) {
				var behavior = behaviors[i];
				if ("close" in behavior.sandbox)
					behavior.sandbox.close();
			}
			this.configurations = null;
			this.behaviors = null;
		</function>
		<function name="create" params="pin">
			pin = pin.sandbox;
			return new (this.constructors[pin.type])(pin);
		</function>
		<function name="repeat" params="interval, target, callback" c="xs_PINS_repeat"/>
		<function name="merge" params="defaultPin, configurationPin">
			for (var i in defaultPin) {
				if (!(i in configurationPin))
					configurationPin[i] = defaultPin[i];
			}
		</function>
		<function name="preconfigure" params="configurations">
			return this.configure_aux(configurations, false);
		</function>
		<function name="configure_aux" params="configurationsNS, construct">
			var constructors = this.constructors;
			var behaviors = {};
			var configurations = configurationsNS.sandbox;
			for (var i in configurations) {
				if ((i != "leftVoltage") && (i != "rightVoltage")) {
					var configuration = configurations[i].sandbox;
					var defaults = undefined;
					if (!("require" in configuration)) {
						if (!("type" in configuration))
							throw new Error("required property missing in configuration " + i + ": " + JSON.stringify(configuration));
						var type = configuration.type;
						var config = {};
						config.sandbox.require = type;
						config.sandbox.pins = {};
						config.sandbox.pins.sandbox[type.toLowerCase()] = configurations[i];
						configurations[i] = config;
						configuration = config.sandbox;
					}
					var prototype = require(configuration.require);
					var behavior = Object.create(prototype);
					behavior.sandbox.id = i;
					if ("pins" in behavior.sandbox)
						defaults = behavior.sandbox.pins.sandbox;
					var pins = ("pins" in configuration) ? configuration.pins.sandbox : undefined;
					if (pins) {
						for (var j in pins) {
							var pin = pins[j].sandbox;
							if (defaults && (j in defaults))
								this.merge(defaults[j].sandbox, pin);
							var type = pin.type;
							if (!type)
								throw new Error("Pin " + j + " missing type: " + JSON.stringify(pin));
							if (!(type in constructors))
								throw new Error("Unsupported pin type on pin " + j + ": " + JSON.stringify(pin));
							if (construct)
								behavior.sandbox[j] = new (constructors[type])(pin);
						}
					}
					if (construct && ("configure" in behavior.sandbox))
						behavior.sandbox.configure(configuration, i);	// passing i for debugging / simulators
					behaviors.sandbox[i] = behavior;
				}
			}
			if (construct)
				this.behaviors = behaviors;
			return configurationsNS;
		</function>
		<function name="metadata" params="bll">
			if (bll) {
				var prototype = require(bll.split("/")[1]);
				return ("metadata" in prototype.sandbox) ? prototype.sandbox.metadata : undefined;
			}
			var result = {};
			for (var i in this.behaviors) {
				var behavior = this.behaviors[i];
				result[i] = behavior.metadata;
			}
			return result;
		</function>
		<function name="configuration">
			return JSON.parse(JSON.sandbox.stringify(PINS.config));		//@@ hack to get object into form xsMarshall will tolerate
		</function>
		<object name="repeater" c="xs_repeater">
			<function name="close" c="xs_repeater_close"/>
		</object>
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

