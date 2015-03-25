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
	<patch prototype="Crypt">
		<null name="certificatesInstance" script="true"/>	<!-- shared instance -->

		<object name="certificateInstanceProto" prototype="Crypt.persistentList">
			<!-- parser/serializer for the cert object -->
			<function name="parse" params="c" script="false">
				return(Crypt.x509.decode(c));
			</function>

			<function name="serialize" params="o" script="false">
				return(o instanceof Chunk ? o: o.ber());
			</function>
		</object>
		<function name="CertificateInstance" params="" prototype="Crypt.certificateInstanceProto">
			Crypt.PersistentList.call(this);
		</function>

		<object name="certificate" prototype="Crypt.persistentListClient">
			<number name="POLICY_ALLOW_ORPHAN"	value="0x1"/>
			<number name="POLICY_ALLOW_SELF_SIGNED"	value="0x2"/>
			<number name="POLICY_ALLOW_LOOPHOLE"	value="0x4"/>
			<number name="POLICY_VERIFY_HOST"	value="0x8"/>
			<number name="POLICY_VERIFY_VALIDITY"	value="0x10"/>
			<number name="policy" value="0" script="false"/>	<!-- no orphan, no self-signed by default -->
			<function name="setPolicy" params="flags">
				this.policy = flags;
			</function>
			<function name="getPolicy" params="">
				return(this.policy);
			</function>
			<null name="localCerts"/>

			<function name="matchIssuer" params="issuerSKI, issuerDN, keyUsage, o" script="false">
				if (issuerSKI && issuerSKI.comp(o.tbsCertificate.extensions.subjectKeyId) == 0)
					// if issuerSKI can be specified (by probably authority key ID), exact match can be done
					return(true);
				var subjectKeyUsage = o.tbsCertificate.extensions.keyUsage.length ? o.tbsCertificate.extensions.keyUsage: null;
				// return(issuerDN == o.tbsCertificate.subject && (!keyUsage || !subjectKeyUsage || keyUsage.comp(subjectKeyUsage) == 0));
				return(issuerDN == o.tbsCertificate.subject);
			</function>

			<function name="findRoot" params="c, certs" script="false">
				var issuerDN = c.tbsCertificate.issuer;
				var keyUsage = c.tbsCertificate.extensions.keyUsage.length ? c.tbsCertificate.extensions.keyUsage : undefined;
				var issuerSKI = c.tbsCertificate.extensions.authorityKeyId.keyIdentifier.length ? c.tbsCertificate.extensions.authorityKeyId.keyIdentifier : undefined;

				if (certs) {
					// search in the parameter
					for (var i = 0; i < certs.length; i++) {
						var o = certs[i]
						if (((this.policy & this.POLICY_ALLOW_SELF_SIGNED) || o != c) &&
						    this.matchIssuer(issuerSKI, issuerDN, keyUsage, o)) {
							if (Crypt.x509.verify(c, o.getKey()))
								return(o);
						}
					}
				}
				if (this.localCerts) {
					// search in the local certs
					for (var i = 0; i < this.localCerts.length; i++) {
						var o = this.localCerts[i];
						if (this.matchIssuer(issuerSKI, issuerDN, keyUsage, o)) {
							if (Crypt.x509.verify(c, o.getKey()))
								return(o);
						}
					}
				}
				// search in the global certs
				if (issuerSKI) {
					var o = this.get(issuerSKI.toString());
					if (o) {
						o.trusted = true;
						/* don't needs to reorder */
						if (Crypt.x509.verify(c, o.getKey()))
							return(o);
						/* otherwise fall thru just in case... */
					}
				}
				for (var i = 0, o; (o = this.nth(i)); i++) {
					if (this.matchIssuer(issuerSKI, issuerDN, keyUsage, o)) {
						o.trusted = true;
						this.push(i);
						if (Crypt.x509.verify(c, o.getKey()))
							return(o);
					}
				}
				// return undefined
			</function>

			<function name="makeCertChain" params="c, rootDN">
				// make a cert chain from 'c' to the root (not including) -- the root cert must be identified uniquely by rootDN
				var certs = [];
				// the end is the first
				var keyUsage = c.tbsCertificate.extensions.keyUsage;
				if (!keyUsage.length) keyUsage = undefined;
				while (c.tbsCertificate.subject != rootDN) {
					certs.push(c.ber());
					var issuer = undefined, issuerSKI = undefined;
					// if c has authorityKeyId, use it, otherwise, use issuer DN
					if (c.tbsCertificate.extensions.authorityKeyId.keyIdentifier.length)
						var issuerSKI = c.tbsCertificate.extensions.authorityKeyId.keyIdentifier;
					else
						var issuerDN = c.tbsCertificate.issuer;
					for (var i = 0, o; (o = this.nth(i)); i++) {
						if (o.tbsCertificate.issuer == o.tbsCertificate.subject)
							continue;	// avoid to find out itself
						if (this.matchIssuer(issuerSKI, issuerDN, keyUsage, o)) {
							issuer = o;
							break;
						}
					}
					if (!issuer)
						break;
					c = issuer;
				}
				return(certs);
			</function>

			<function name="verifyValidity" params="c">
				var t = new Date();
				return t > c.tbsCertificate.validity.notBefore && t < c.tbsCertificate.validity.notAfter;
			</function>

			<function name="processCerts" params="certChunks, register">
				if (!certChunks.length)
					return;		// undefined

				// decode all certs
				var certs = [];
				for (var i = 0; i < certChunks.length; i++)
					certs.push(Crypt.x509.decode(certChunks[i]));

				if (!(this.policy & this.POLICY_ALLOW_LOOPHOLE)) {
					// verify all certs
					for (var i = 0; i < certs.length; i++) {
						var c = certs[i];
						if ((this.policy & this.POLICY_VERIFY_VALIDITY) && !this.verifyValidity(c))
							throw new Crypt.Error(Crypt.error.kCryptVerificationError);
						var root = this.findRoot(c, certs);
						if (!root) {
							if (!(this.policy & this.POLICY_ALLOW_ORPHAN)) {
								var t = this.get(c.getKey().keyName);
								if (!t || t.__ber__.comp(c.__ber__) != 0)
									throw new Crypt.Error(Crypt.error.kCryptVerificationError);
							}
							else {
								trace("WARNING: no root certificate for " + c.tbsCertificate.subject + "\n");
								debugger;
							}
						}
						c.root = root;
						if (root && !root.hasOwnProperty("trusted"))
							root.child = c;
					}
				}

				// then, register if required
				if (register) {
					for (var i = 0; i < certs.length; i++) {
						var c = certs[i];
						if (!(this.policy & this.POLICY_ALLOW_ORPHAN)) {
							// check if the cert chain is not broken
							for (var cc = c; cc.hasOwnProperty("root") && cc.root; cc = cc.root)
								;
							if (!cc.hasOwnProperty("trusted"))
								throw new Crypt.Error(Crypt.error.kCryptVerificationError);
						}
						if (!c.hasOwnProperty("trusted"))
							this.set(c.getKey().keyName, c);
					}
				}

				// return the most descendant
				var descendant = certs[0];
				for (var i = 0; i < certs.length; i++) {
					var c = certs[i];
					if (!c.hasOwnProperty("child")) {
						descendant = c;
						break;
					}
				}
				return descendant;
			</function>

			<function name="registerRootCerts" params="certs, local">
				if (local) {
					if (certs && certs.length > 0) {
						this.localCerts = Array();
						for (var i = 0; i < certs.length; i++)
							this.localCerts.push(Crypt.x509.decode(certs[i]));
					}
				}
				else {
					for (var i = 0; i < certs.length; i++) {
						var c = certs[i];
						var skid = Crypt.x509.decodeSubjectKeyId(c);
						if (skid)
							this.set(skid.toString(), c);
						else {
							trace("WARNING: unrecognized cert: " + c.toString() + "\n");
							break;
						}
					}
				}
			</function>
		</object>
		<function name="Certificate" params="certificatesInstance" prototype="Crypt.certificate">
			Crypt.PersistentListClient.call(this, certificatesInstance ? certificatesInstance : Crypt.certificatesInstance);
		</function>
	</patch>

	<program>
		Crypt.certificatesInstance = new Crypt.CertificateInstance();
	</program>
</package>