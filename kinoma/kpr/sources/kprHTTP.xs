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
		<object name="server" c="KPR_HTTP_server">
			<function name="get isSecure" c="KPR_HTTP_server_get_isSecure"/>
			<function name="get port" c="KPR_HTTP_server_get_port"/>
			<function name="get running" c="KPR_HTTP_server_get_running"/>
			<function name="start" c="KPR_HTTP_server_start"/>
			<function name="stop" c="KPR_HTTP_server_stop"/>
			<!-- callbacks to the server handler behavior -->
			<!-- function name="onAccept" params="handler, message" /-->
			<!-- function name="onInvoke" params="handler, message" /-->
		</object>
		<function name="Server" params="dictionary" prototype="HTTP.server" c="KPR_HTTP_Server"/>
	</object>
	
	<patch prototype="KPR.message">
		<function name="get remoteIP" c="KPR_message_get_remoteIP"/>
		<function name="get timeout" c="KPR_message_get_timeout"/>
		<function name="set requestPath" params="path" c="KPR_message_set_requestPath"/>
		<function name="set responsePath" params="path" c="KPR_message_set_responsePath"/>
		<function name="set timeout" params="timeout" c="KPR_message_set_timeout"/>
	</patch>

	<patch prototype="KPR.application">
		<function name="get serverIsSecure" c="KPR_context_get_serverIsSecure"/>
		<function name="get serverPort" c="KPR_context_get_serverPort"/>
		<function name="get shared" c="KPR_context_get_shared"/>
		<function name="get uuid" c="KPR_context_get_uuid"/>
		<function name="set shared" params="value" c="KPR_context_set_shared"/>
		<function name="discover" params="authority,dictionary" c="KPR_context_discover"/>
		<function name="forget" params="authority" c="KPR_context_forget"/>
		<function name="share" params="dictionary" c="KPR_context_share"/>
	</patch>
	<patch prototype="KPR.shell">
		<function name="get serverIsSecure" c="KPR_context_get_serverIsSecure"/>
		<function name="get serverPort" c="KPR_context_get_serverPort"/>
		<function name="get shared" c="KPR_context_get_shared"/>
		<function name="get uuid" c="KPR_context_get_uuid"/>
		<function name="set shared" params="value" c="KPR_context_set_shared"/>
		<function name="discover" params="authority,dictionary" c="KPR_context_discover"/>
		<function name="forget" params="authority" c="KPR_context_forget"/>
		<function name="share" params="dictionary" c="KPR_context_share"/>
	</patch>
	<patch prototype="FskSSL">
		<function name="loadRootCerts" params="calist">
			var certMgr = new Crypt.Certificate(FskSSL.certificatesInstance);
			certMgr.setPolicy(Crypt.certificate.POLICY_ALLOW_ORPHAN | Crypt.certificate.POLICY_ALLOW_LOOPHOLE); // trust all certs in the default CA list
			if (!FileSystem.getFileInfo(calist))
				return;
			var s = new Stream.File(calist);
			var c = s.readChunk(s.bytesAvailable);
			s.close();
			// add Kinoma CA
			var kinomaCA = "-----BEGIN CERTIFICATE-----\n"
				+ "MIIDnTCCAoWgAwIBAgIJALuAbh4896tlMA0GCSqGSIb3DQEBBQUAMD0xCzAJBgNV\n"
				+ "BAYTAlVTMQswCQYDVQQIEwJDQTEQMA4GA1UEChMHTWFydmVsbDEPMA0GA1UECxMG\n"
				+ "S2lub21hMB4XDTE1MDMxMzIyMTQ1MFoXDTI1MDMxMDIyMTQ1MFowPTELMAkGA1UE\n"
				+ "BhMCVVMxCzAJBgNVBAgTAkNBMRAwDgYDVQQKEwdNYXJ2ZWxsMQ8wDQYDVQQLEwZL\n"
				+ "aW5vbWEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDFIB/GkypgFw4+\n"
				+ "dMgj+qh72dOo7gMvOJNvM5+bd7dWLN+NVBFxcQHqAArvLoJv52n2oc0fVcZdAMmP\n"
				+ "JPS0oOuJshxb5B9/AY03kEgZI8VKMpFZFW4jO2KsPhgE8NfwSu5M+6jthrvAumDY\n"
				+ "dVwo2zw7tit1ThgxBBx7CA42Q5+o9jp4lgPMvNiayfgcvl1QDZHPJCD7YhDUsubc\n"
				+ "vvfx113BB5G3ZkGSBCEAQ4vygclBOF3/8q+9RSkIW/DtvhTOHUaueKF68sVlmZzf\n"
				+ "Xa+r6MjWXacEQBfzufQlzaH4mB857woyUyoSt+ZCo1irA8mlVMiJ/HGpWUpUQ75d\n"
				+ "2X5Vdgh1AgMBAAGjgZ8wgZwwHQYDVR0OBBYEFEF33c9qE6plHRZToTtX8BUqppKM\n"
				+ "MG0GA1UdIwRmMGSAFEF33c9qE6plHRZToTtX8BUqppKMoUGkPzA9MQswCQYDVQQG\n"
				+ "EwJVUzELMAkGA1UECBMCQ0ExEDAOBgNVBAoTB01hcnZlbGwxDzANBgNVBAsTBktp\n"
				+ "bm9tYYIJALuAbh4896tlMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADggEB\n"
				+ "AFnKc1iLnkl2IdbPCi5hHdYz/uOG8hw91I85wmZxcnOk3Ja9SXyETfXLMFzgqpvj\n"
				+ "4d8k+bBjy1pdSa2iPQt06tkrnVTNjP/4rGe0p/b5XtAi32UNpLDa861i+0afuQCq\n"
				+ "zJq9f2AzT5C5cqp6qzIAP9PmRjO+16JTA8lJUTg0fI2aL2VFh7HaQvD3ChOLfImA\n"
				+ "dgveWN4GhnJGZnythuDPkzSz0jsOLvgrkHoaeBtgJ7lrehKt57C10XBMAYk19FTH\n"
				+ "MeQYizQhxTBu4LL0jP/8Xd/KTnbK+Rs3dh3jiwrjqjEiAM2LQHZ3jWRbnqETReyq\n"
				+ "sS2JmaDBDafHgmCfk1TvC3I=\n"
				+ "-----END CERTIFICATE-----\n";
			s = new Stream.String(kinomaCA);
			var kinomaCAChunk = s.readChunk(s.bytesAvailable);
			s.close();
			c.append(kinomaCAChunk);

			FskSSL.loadCerts(certMgr, c);
		</function>
	</patch>
</package>
