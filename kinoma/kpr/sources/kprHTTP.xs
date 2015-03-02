<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<package script="true">
	<patch prototype="KPR">
		<function name="MD5" c="KPR_MD5"/>
	</patch>
			
	<object name="HTTP">
		<object name="Cache">
			<function name="clear" c="KPR_HTTP_Cache_clear"/>
		</object>
		<object name="Cookies">
			<function name="clear" c="KPR_HTTP_Cookies_clear"/>
			<function name="get" params="url" c="KPR_HTTP_Cookies_get"/>
			<function name="set" params="url, value" c="KPR_HTTP_Cookies_set"/>
		</object>
		<object name="Keychain">
			<function name="clear" c="KPR_HTTP_Keychain_clear"/>
			<function name="get" params="host, realm" c="KPR_HTTP_Keychain_get"/>
			<function name="remove" params="host, realm" c="KPR_HTTP_Keychain_remove"/>
			<function name="set" params="host, realm, user, password" c="KPR_HTTP_Keychain_set"/>
		</object>
	</object>
	
	<patch prototype="KPR.message">
		<function name="get remoteIP" c="KPR_message_get_remoteIP"/>
		<function name="get timeout" c="KPR_message_get_timeout"/>
		<function name="set requestPath" params="path" c="KPR_message_set_requestPath"/>
		<function name="set responsePath" params="path" c="KPR_message_set_responsePath"/>
		<function name="set timeout" params="timeout" c="KPR_message_set_timeout"/>
	</patch>

	<patch prototype="KPR.application">
		<function name="get serverPort" c="KPR_context_get_serverPort"/>
		<function name="get shared" c="KPR_context_get_shared"/>
		<function name="get uuid" c="KPR_context_get_uuid"/>
		<function name="set shared" params="value" c="KPR_context_set_shared"/>
		<function name="discover" params="authority,dictionary" c="KPR_context_discover"/>
		<function name="forget" params="authority" c="KPR_context_forget"/>
		<function name="share" params="dictionary" c="KPR_context_share"/>
	</patch>
	<patch prototype="KPR.shell">
		<function name="get serverPort" c="KPR_context_get_serverPort"/>
		<function name="get shared" c="KPR_context_get_shared"/>
		<function name="get uuid" c="KPR_context_get_uuid"/>
		<function name="set shared" params="value" c="KPR_context_set_shared"/>
		<function name="discover" params="authority,dictionary" c="KPR_context_discover"/>
		<function name="forget" params="authority" c="KPR_context_forget"/>
		<function name="share" params="dictionary" c="KPR_context_share"/>
	</patch>
</package>