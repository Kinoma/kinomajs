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

	<!--

		var conn;


		var msg = MQTT.Message("ON");
		msg.qos = 2;
		msg.retain = true;
		conn.publish("kichih:gas:a", "ON", 2, true)
		conn.publish("kichih:gas:a", {
			payload: "ON", 
			qos: 2,
			retain: true, 
		});
		conn.publish("kichih:gas:a", new Chunk("aXkjSDFHKJ="));

		conn.subscribe("kinoma/#", {
			qos: 2,
			type: "JSON",
		})

		function callback(str) {
			var 
		}

	-->
	
	<object name="MQTT">
		<object name="_client" c="KPR_mqttclient_destructor">
			<function name="connect" params="host, port, options" c="KPR_mqttclient_connect"/>
			<function name="reconnect" params="" c="KPR_mqttclient_reconnect"/>
			<function name="disconnect" params="" c="KPR_mqttclient_disconnect"/>
			<function name="publish" params="topic, message" c="KPR_mqttclient_publish"/>
			<function name="subscribe" params="topic, qos" c="KPR_mqttclient_subscribe"/>
			<function name="unsubscribe" params="topic" c="KPR_mqttclient_unsubscribe"/>

			<null name="onConnect"/>
			<null name="onDisconnect"/>
			<null name="onMessage"/>
			<null name="onSubscribe"/>
			<null name="onUnsubscribe"/>
			<null name="onPublish"/>
			<null name="onError"/>
		</object>
		<function name="Client" params="" prototype="MQTT._client" c="KPR_MQTTClient"/>
		
		<object name="_message" c="KPR_mqttmessage_destructor">
			<function name="get length" c="KPR_mqttmessage_get_length"/>
			<function name="get data" c="KPR_mqttmessage_get_data"/>
			<function name="get binaryData" c="KPR_mqttmessage_get_binaryData"/>
		</object>

		<object name="_broker" c="KPR_mqttbroker_destructor">
			<function name="get clients" c="KPR_mqttbroker_get_clients"/>
			<function name="disconnect" params="clientIdentifier" c="KPR_mqttbroker_disconnect"/>

			<null name="onConnect"/>
			<null name="onDisconnect"/>
			<null name="onSubscribe"/>
			<null name="onUnsubscribe"/>
			<null name="onPublish"/>
			<null name="onError"/>
		</object>
		<function name="Broker" params="port" prototype="MQTT._broker" c="KPR_MQTTBroker"/>
	</object>
	<!-- MQTT.CLOSED   = 0 -->
	<!-- MQTT.OPENING  = 1 -->
	<!-- MQTT.OPEN     = 2 -->
	<!-- MQTT.CLOSING  = 3 -->
	<!-- MQTT.CLOSING  = 3 -->
	<program c="KPR_MQTT_patch"/>
</package>

