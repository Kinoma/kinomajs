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
<package>
	<patch prototype="FskSSL">
		<function name="processPEM" params="c, certs, keyring, keyName">
			var msgs = Crypt.pem.decode(c);
			for (var i = 0; i < msgs.length; i++) {
				var o = msgs[i];
				switch (o.keyword) {
				case "CERTIFICATE":
				case "X509 CERTIFICATE":
					if (certs)
						certs.push(o.body);
					break;
				case "RSA PRIVATE KEY":
				case "DSA PRIVATE KEY":
					if (keyring)
						keyring.register(keyName, o.body, Crypt.keyInfo.pem, o.keyword);
					break;
				case "PRIVATE KEY":
					if (keyring)
						keyring.register(keyName, o.body, Crypt.keyInfo.pkcs8);
					break;
				}
			}
		</function>

		<function name="loadCerts" params="certMgr, certs, local">
			var objs;
			if (certs.length > 0) {
				// figure out the format
				if (certs.peek(0) & 0x80) {	// X.509 should start with 0x8x ...
					objs = [certs];	// let it throw an exception if the format is unrecognized
				}
				else {
					objs = [];
					this.processPEM(certs, objs);
				}
				certMgr.registerRootCerts(objs, local);
			}
			if ("policies" in certs) {
				var policyStr = certs.policies.toLowerCase();
				var policies = 0;
				if (policyStr.indexOf("alloworphan") >= 0)
					policies |= Crypt.certificate.POLICY_ALLOW_ORPHAN;
				if (policyStr.indexOf("allowloophole") >= 0)
					policies |= Crypt.certificate.POLICY_ALLOW_LOOPHOLE;
				if (policyStr.indexOf("allowselfsigned") >= 0)
					policies |= Crypt.certificate.POLICY_ALLOW_SELF_SIGNED;
				if (policyStr.indexOf("verifyhost") >= 0)
					policies |= Crypt.certificate.POLICY_VERIFY_HOST;
				certMgr.setPolicy(policies);
			}
			return objs;
		</function>

		<function name="loadKey" params="keyring, key, name">
			if (key.peek(0) & 0x80)
				keyring.register(name, key, Crypt.keyInfo.pkcs8);
			else
				this.processPEM(key, undefined, keyring, name);
		</function>
	</patch>
</package>