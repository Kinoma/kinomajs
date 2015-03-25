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
		<object name="xmpp" c="KPR_xmpp">
			<function name="connect" c="KPR_xmpp_connect"/>
			<function name="disconnect" c="KPR_xmpp_disconnect"/>
			<!--function name="get document"/-->
			<function name="get bareJID" c="KPR_xmpp_get_bareJID"/>
			<function name="get fullJID" c="KPR_xmpp_get_fullJID"/>
			<function name="get nextID" c="KPR_xmpp_get_nextID"/>
			<function name="get resource" c="KPR_xmpp_get_resource"/>
			<function name="set certificate" c="KPR_xmpp_set_certificate"/>
			<function name="send" params="stanza" c="KPR_xmpp_send"/>
			<function name="tunnel" params="proxyHost" c="KPR_xmpp_tunnel"/>
			<!-- XEP-0077: In-Band Registration -->
			<function name="cancelRegistration" c="KPR_xmpp_cancelRegistration"/>
			<function name="changePassword" c="KPR_xmpp_changePassword"/>
			<function name="register" c="KPR_xmpp_register"/>
			<!-- XEP-0106: JID Escaping -->
			<function name="escapeNode" params="node" c="KPR_xmpp_escapeNode"/>
			<function name="unescapeNode" params="node" c="KPR_xmpp_unescapeNode"/>
		</object>
	</patch>
	
	<function name="XMPP" params="behavior, domain, user, pasword, resource" prototype="KPR.xmpp" c="KPR_XMPP"/>
	<!-- XMPP.SocketNotConnectedError -->
	<!-- XMPP.SSLHandshakeFailedError -->
	<!-- XMPP.AuthFailedError -->
	<!-- XMPP.UnimplementedError -->
	<program c="KPR_XMPP_patch"/>
</package>