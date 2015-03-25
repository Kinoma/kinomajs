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

	<patch prototype="KPR">
		<object name="websocket" c="KPR_websocketclient">
			<function name="send" params="data" c="KPR_websocketclient_send"/>
			<function name="close" params="code, reason" c="KPR_websocketclient_close"/>

			<function name="get readyState" c="KPR_websocketclient_get_readyState"/>
			<function name="get bufferedAmount" c="KPR_websocketclient_get_bufferedAmount"/>
			<function name="get protocol" c="KPR_websocketclient_get_protocol"/>
			<function name="get url" c="KPR_websocketclient_get_url"/>

			<null name="onopen"/>
			<null name="onmessage"/>
			<null name="onclose"/>
			<null name="onerror"/>
		</object>
		<object name="websocketserver" c="KPR_websocketserver">
			<function name="get clientCount" c="KPR_websocketserver_get_clientCount"/>
			<function name="get pingInterval" c="KPR_websocketserver_get_pingInterval"/>
			<function name="set pingInterval" c="KPR_websocketserver_set_pingInterval"/>
			<function name="get pingTimeout" c="KPR_websocketserver_get_pingTimeout"/>
			<function name="set pingTimeout" c="KPR_websocketserver_set_pingTimeout"/>

			<null name="onconnect"/>
			<null name="ondisconnecting"/>
			<null name="ondisconnect"/>
			<null name="onerror"/>
		</object>
	</patch>
	
	<function name="WebSocket" params="url, protocols" prototype="KPR.websocket" c="KPR_WebSocketClient"/>
	<function name="WebSocketServer" params="port" prototype="KPR.websocketserver" c="KPR_WebSocketServer"/>
	<!-- WebSocket.CONNECTING = 0 -->
	<!-- WebSocket.OPEN       = 1 -->
	<!-- WebSocket.CLOSING    = 2 -->
	<!-- WebSocket.CLOSED     = 3 -->
	<program c="KPR_WebSocket_patch"/>
</package>