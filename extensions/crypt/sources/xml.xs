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
		<object name="xmlProto">
			<null name="proto" script="false"/>
			<null name="keyring" script="false"/>
			<null name="certificates" script="false"/>
			<null name="key" script="false"/>
			<null name="resolvedKey" script="false"/>
			<null name="enc_f" script="false"/>
			<null name="defaultSignatureProperties"/>
			<null name="defaultEncryptionProperties"/>
			<undefined name="transformCallback"/>
			<undefined name="urrInstance"/>

			<object name="signatureAlgorithm" script="false">
				<null name="sig"/>
				<null name="hash"/>
			</object>
			<function name="SignatureAlgorithm" params="proto, key, privFlag" prototype="Crypt.xmlProto.signatureAlgorithm" script="false">
				switch (proto.signedInfo.signatureAlgorithm) {
				case "http://www.w3.org/2000/09/xmldsig#dsa-sha1":
					if (!key.hasOwnProperty("dsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					this.sig = new Crypt.DSA(key.dsaKey, privFlag);
					this.hash = new Crypt.SHA1();
					break;
				case "http://www.w3.org/2000/09/xmldsig#rsa-sha1":
					if (!key.hasOwnProperty("rsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					var oid = [1, 3, 14, 3, 2, 26];
					this.sig = new Crypt.PKCS1_5(key.rsaKey, privFlag, oid);
					this.hash = new Crypt.SHA1();
					break;
				case "http://www.w3.org/2000/09/xmldsig-more#rsa-sha256":
					if (!key.hasOwnProperty("rsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					var oid = [2, 16, 840, 1, 101, 3, 4, 2, 1];
					this.sig = new Crypt.PKCS1_5(key.rsaKey, privFlag, oid);
					this.hash = new Crypt.SHA256();
					break;
				case "http://www.w3.org/2000/09/xmldsig-more#rsa-sha512":
					if (!key.hasOwnProperty("rsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					var oid = [2, 16, 840, 1, 101, 3, 4, 2, 3];
					this.sig = new Crypt.PKCS1_5(key.rsaKey, privFlag, oid);
					this.hash = new Crypt.SHA512();
					break;
				case "http://www.w3.org/2000/09/xmldsig#hmac-sha1":
					if (!key.hasOwnProperty("sym"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					this.sig = new Crypt.HMAC(new Crypt.SHA1(), key.sym, proto.signedInfo.hmacOutputLength);
					break;
				case "http://www.w3.org/2001/04/xmldsig-more#hmac-md5":
					if (!key.hasOwnProperty("sym"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					this.sig = new Crypt.HMAC(new Crypt.MD5(), key.sym, proto.signedInfo.hmacOutputLength);
					break;
				case "http://www.w3.org/2000/09/xmldsig#sha1":
					this.hash = new Crypt.SHA1();
					break;
				case "http://www.w3.org/2001/04/xmldsig-more#md5":
					this.hash = new Crypt.MD5();
					break;
				default:
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);
				}
			</function>

			<function name="DigestAlgorithm" params="algo" prototype="Crypt.xmlProto.signatureAlgorithm" script="false">
				switch (algo) {
				case "http://www.w3.org/2000/09/xmldsig#sha1":
					this.hash = new Crypt.SHA1();
					break;
				case "http://www.w3.org/2001/04/xmlenc#sha256":
					this.hash = new Crypt.SHA256();
					break;
				case "http://www.w3.org/2001/04/xmlenc#sha512":
					this.hash = new Crypt.SHA512();
					break;
				case "http://www.w3.org/2001/04/xmldsig-more#md5":
					this.hash = new Crypt.MD5();
					break;
				default:
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);
				}
			</function>

			<object name="encryptionAlgorithm" script="false">
				<null name="enc"/>
				<null name="encMode"/>
				<string name="digestMethod"/>

				<object name="cbc">
					<function name="encrypt" params="blkenc, data, iv">
						if (!iv)
							iv = Crypt.defaultRNG.get(blkenc.blockSize);
						var outData = new Chunk();
						outData.append(iv);				// prepend IV for decryption
						var cbcMode = new Crypt.CBC(blkenc, iv, -1);// use RFC2630 padding mode
						outData.append(cbcMode.encrypt(data));
						return(outData);
					</function>
					<function name="decrypt" params="blkenc, data">
						var iv = data.slice(0, blkenc.blockSize);	// read IV from the input data
						var cbcMode = new Crypt.CBC(blkenc, iv, -1);// use RFC2630 padding mode
						return(cbcMode.decrypt(data.slice(blkenc.blockSize)));
					</function>
				</object>

				<object name="kw3DES">
					<null name="kwEnc"/>
					<string name="iv" value="0x4adda22c79e82105"/>
					<function name="reverse" params="c" script="false">
						var r = new Chunk(c.length);
						for (var i = 0, l = c.length; i < l; i++)
							r.poke(l - i - 1, c.peek(i));
						return(r);
					</function>
					<function name="encrypt" params="wk">
						var h = new Crypt.SHA1();
						h.update(wk);
						var cks = h.close().slice(0, 8);
						var iv1 = Crypt.defaultRNG.get(8)
						var cbc = new Crypt.CBC(this.kwEnc, iv1);
						var wkcks = new Chunk();
						wkcks.append(wk);
						wkcks.append(cks);
						var temp1 = cbc.encrypt(wkcks);
						iv1.append(temp1);
						var temp3 = this.reverse(iv1);
						var cbc = new Crypt.CBC(this.kwEnc, (new Arith.Integer(this.iv)).toChunk());
						return(cbc.encrypt(temp3));
					</function>
					<function name="decrypt" params="C">
						var cbc = new Crypt.CBC(this.kwEnc, (new Arith.Integer(this.iv)).toChunk());
						var temp3 = cbc.decrypt(C);
						var temp2 = this.reverse(temp3);
						var iv1 = temp2.slice(0, 8);
						var temp1 = temp2.slice(8);
						var cbc = new Crypt.CBC(this.kwEnc, iv1);
						var wkcks = cbc.decrypt(temp1);
						var wk = wkcks.slice(0, -8);
						var cks = wkcks.slice(-8);
						var h = new Crypt.SHA1();
						h.update(wk);
						if (cks.ncomp(h.close(), 8) != 0)
							throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
						return(wk);
					</function>
				</object>

				<object name="kwAES">
					<null name="kwEnc"/>
					<string name="magic" value="0xa6a6a6a6a6a6a6a6"/>
					<function name="xor" params="r, t, c" script="false">
						r.poke32(0, c.peek32(0));
						r.poke32(4, c.peek32(4) ^ t);
					</function>
					<function name="encrypt" params="P">
						var N = P.length / 8;
						var A = (new Arith.Integer(this.magic)).toChunk();
						var B = new Chunk(16);
						if (N == 1) {
							var p1 = new Chunk();
							p1.append(magic);
							p1.append(P);
							this.kwEnc.encrypt(p1, B);
							return(B);
						}
						var R = [0];	// make it 1 origin
						for (var i = 0; i < P.length; i += 8)
							R.push(P.slice(i, i + 8));
						for (var j = 0; j <= 5; j++) {
							for (var i = 1; i <= N; i++) {
								var t = i + j * N;
								var p1 = new Chunk();
								p1.append(A);
								p1.append(R[i]);
								this.kwEnc.encrypt(p1, B);
								this.xor(A, t, B.slice(0, 8));
								R[i] = B.slice(8);
							}
						}
						var C = new Chunk();
						C.append(A);
						for (var i = 1; i <= N; i++)
							C.append(R[i]);
						return(C);
					</function>
					<function name="decrypt" params="C">
						var N = (C.length / 8) - 1;
						var B = new Chunk(16);
						if (N == 1) {
							var A = (new Arith.Integer(this.magic)).toChunk();
							this.kwEnc.decrypt(C, B);
							if (A.comp(B.slice(0, 8)) != 0)
								throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
							return(B.slice(8));
						}
						var A = C.slice(0, 8);
						var R = [0];	// make it 1 origin
						for (var i = 8; i < C.length; i += 8)
							R.push(C.slice(i, i + 8));
						for (var j = 5; j >= 0; --j) {
							for (var i = N; i >= 1; --i) {
								var t = i + j * N;
								var p1 = new Chunk(8);
								this.xor(p1, t, A);
								p1.append(R[i]);
								this.kwEnc.decrypt(p1, B);
								A = B.slice(0, 8);
								R[i] = B.slice(8);
							}
						}
						if (A.comp((new Arith.Integer(this.magic)).toChunk()) != 0)
							throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
						var P = new Chunk();
						for (var i = 1; i <= N; i++)
							P.append(R[i]);
						return(P);
					</function>
				</object>

				<function name="getKeySize" params="algo">
					switch (algo) {
					case "http://www.w3.org/2001/04/xmlenc#tripledes-cbc":
					case "http://www.w3.org/2001/04/xmlenc#kw-tripledes":
						return(192);	// 3key triple DES in the XML enc specification
					case "http://www.w3.org/2001/04/xmlenc#aes128-cbc":
					case "http://www.w3.org/2001/04/xmlenc#kw-aes128":
						return(128);
					case "http://www.w3.org/2001/04/xmlenc#aes192-cbc":
					case "http://www.w3.org/2001/04/xmlenc#kw-aes192":
						return(192);
					case "http://www.w3.org/2001/04/xmlenc#aes256-cbc":
					case "http://www.w3.org/2001/04/xmlenc#kw-aes256":
						return(256);
					default:
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					}
				</function>
			</object>
			<function name="EncryptionAlgorithm" params="proto, key, privFlag" prototype="Crypt.xmlProto.encryptionAlgorithm" script="false">
				switch (proto.encryptionMethod.encryptionAlgorithm) {
				case "http://www.w3.org/2001/04/xmlenc#tripledes-cbc":
					if (!key.hasOwnProperty("sym"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					this.enc = new Crypt.TDES(key.sym);
					this.encMode = this.cbc;
					break;
				case "http://www.w3.org/2001/04/xmlenc#aes128-cbc":
					var keySize = 128;
				case "http://www.w3.org/2001/04/xmlenc#aes192-cbc":
					if (!keySize) var keySize = 192;
				case "http://www.w3.org/2001/04/xmlenc#aes256-cbc":
					if (!key.hasOwnProperty("sym"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					if (!keySize) var keySize = 256;
					this.enc = new Crypt.AES(key.sym, keySize / 8);
					this.encMode = this.cbc;
					break;
				case "http://www.w3.org/2001/04/xmlenc#rsa-1_5":
					this.enc = new Crypt.PKCS1_5(key.rsaKey, privFlag);
					break;
				case "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p":
					if (!key.hasOwnProperty("rsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					var P = proto.encryptionMethod.oaepParams;
					var a = new Crypt.xmlProto.DigestAlgorithm(proto.encryptionMethod.digestAlgorithm ? proto.encryptionMethod.digestAlgorithm: Crypt.xmlProto.defaultEncryptionProperties.encryptionMethod.digestAlgorithm);
					this.enc = new Crypt.OAEP(key.rsaKey, privFlag, a.hash, P);
					break;
				case "http://www.w3.org/2001/04/xmlenc#kw-tripledes":
					if (!key.hasOwnProperty("sym"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					this.enc = this.kw3DES;
					this.enc.kwEnc = new Crypt.TDES(key.sym);
					break;
				case "http://www.w3.org/2001/04/xmlenc#kw-aes128":
					var keySize = 128;
				case "http://www.w3.org/2001/04/xmlenc#kw-aes192":
					if (!keySize) var keySize = 192;
				case "http://www.w3.org/2001/04/xmlenc#kw-aes256":
					if (!key.hasOwnProperty("sym"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					if (!keySize) var keySize = 256;
					this.enc = this.kwAES;
					this.enc.kwEnc = new Crypt.AES(key.sym, keySize / 8);
					break;
				default:
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);
				}
			</function>

			<function name="arrayRemoveElement" params="array, index" script="false">
				// copy the array exept the specified index
				var newArray = [];
				for (var i = 0, l = array.length; i < l; i++) {
					if (i != index)
						newArray.push(array[i]);
				}
				return(newArray);
			</function>

			<function name="findXMLAttr" params="name, rootElm" script="false">
				for (var i = 0, attrs = rootElm.attributes; i < attrs.length; i++) {
					if (attrs[i].prefix == "xml" && attrs[i].name == name)
						return(true);
				}
				return(false);
			</function>

			<function name="inheritXMLAttr" params="elm, rootElm" script="false">
				for (; xs.isInstanceOf(elm, xs.infoset.element); elm = elm.parent) {
					for (var i = 0, attr = elm.attributes; i < attr.length; i++) {
						if (attr[i].prefix == "xml" && !this.findXMLAttr(attr[i].name, rootElm))
							rootElm.attributes.push(attr[i]);
					}
				}
			</function>

			<function name="findNS" params="name, rootElm" script="false">
				for (var i = 0, attrs = rootElm.xmlnsAttributes; i < attrs.length; i++) {
					if (attrs[i].name == name)
						return(true);
				}
				return(false);
			</function>

			<function name="inheritNS" params="elm, rootElm" script="false">
				for (; xs.isInstanceOf(elm, xs.infoset.element); elm = elm.parent) {
					for (var i = 0, attr = elm.xmlnsAttributes; i < attr.length; i++) {
						if (!this.findNS(attr[i].name, rootElm))
							rootElm.xmlnsAttributes.push(attr[i]);
					}
				}
			</function>

			<number name="CANON_NO_NS"	value="1"/>
			<number name="CANON_NO_XMLATTR"	value="2"/>
			<number name="CANON_ASIS"	value="3"/>

			<function name="makeItRoot" params="elm, flags">
				var root = xs.newInstanceOf(xs.infoset.document);
				var e = xs.newInstanceOf(xs.infoset.element);
				// copy the element to root.element
				e.parent = root;
				e.children = [];
				for (var i = 0; i < elm.children.length; i++) {
					e.children[i] = elm.children[i];
					e.children[i].parent = e;
				}
				e.name = elm.name;
				e.namespace = elm.namespace;
				e.prefix = elm.prefix;
				e.attributes = [];
				for (var i = 0; i < elm.attributes.length; i++) {
					e.attributes[i] = elm.attributes[i];
					e.attributes[i].parent = e;
				}
				e.xmlnsAttributes = [];
				for (var i = 0; i < elm.xmlnsAttributes.length; i++) {
					e.xmlnsAttributes[i] = elm.xmlnsAttributes[i];
					e.xmlnsAttributes[i].parent = e;
				}
				if (!flags) flags = 0;
				if (!(flags & this.CANON_NO_NS))
					this.inheritNS(elm.parent, e);
				if (!(flags & this.CANON_NO_XMLATTR))
					this.inheritXMLAttr(elm.parent, e);
				root.element = e;
				root.children = [root.element];
				root.encoding = "UTF-8";
				root.version = "1.0";
				return(root);
			</function>

			<function name="getAncientNS" params="elm, name" script="false">
				while ("parent" in elm) {
					elm = elm.parent;
					if ("xmlnsAttributes" in elm) {
						for (var i = 0, attr = elm.xmlnsAttributes, l = attr.length; i < l; i++) {
							if (attr[i].name == name)
								return(attr[i]);
						}
					}
				}
				// return undefined
			</function>

			<function name="exclProcessNS" params="elm, ns" script="false">
				for (var i = 0, attrs = elm.xmlnsAttributes, l = attrs.length; i < l; i++) {
					var a = attrs[i];
					if (a.name == ns)
						return(a);
				}
				return(this.getAncientNS(elm, ns));
			</function>

			<function name="exclGetNS" params="elm" script="false">
				function inNewNS(name) {
					for (var i = 0; i < newNS.length; i++) {
						if (newNS[i].name == name)
							return(true);
					}
				}
				var newNS = [];
				// process element
				var ns = this.exclProcessNS(elm, elm.prefix ? elm.prefix: "xmlns");
				if (ns) newNS.push(ns);
				// process each attribute
				for (var i = 0, attrs = elm.attributes, l = attrs.length; i < l; i++) {
					var a = attrs[i];
					if (a.prefix && !inNewNS(a.prefix)) {
						var ns = this.exclProcessNS(elm, a.prefix);
						if (ns) newNS.push(ns);
					}
				}
				return(newNS);
			</function>

			<function name="exclRenderNS" params="elm" script="false">
				for (var i = 0, c = elm.children, l = c.length; i < l; i++) {
					var e = c[i];
					if (xs.isInstanceOf(e, xs.infoset.element)) {
						this.exclRenderNS(e);	// process the children before overwritting the namespace
						e.xmlnsAttributes = this.exclGetNS(e);
					}
				}
			</function>

			<function name="findInList" params="name, prefixList" script="false">
				for (var i = 0; i < prefixList.length; i++) {
					if (prefixList[i] == name || (prefixList[i] == "#default" && name == "xmlns"))
						return(true);
				}
				return(false);
			</function>

			<function name="findInAttributes" params="ns, attributes" script="false">
				for (var i = 0; i < attributes.length; i++) {
					if (attributes[i].prefix && attributes[i].prefix == ns)
						return(true);
				}
				return(false);
			</function>

			<function name="nsWalker" params="elm" script="false">
				var newNS = [];
				for (var i = 0, attrs = elm.xmlnsAttributes, l = attrs.length; i < l; i++) {
					var a = attrs[i];
					var aa = this.getAncientNS(elm, a.name);
					if (!((aa && aa.value == a.value) || (!aa && a.name == "xmlns" && a.value == "")))
						newNS.push(a);
				}
				elm.xmlnsAttributes = newNS;
			</function>

			<function name="normalizeNamespace" params="doc" script="false">
				for (var i = 0, c = doc.children, length = c.length; i < length; i++) {
					if ("xmlnsAttributes" in c[i])
						this.nsWalker(c[i]);
					if (xs.isInstanceOf(c[i], xs.infoset.element))
						this.normalizeNamespace(c[i]);
				}
			</function>

			<function name="canon" params="elm, algo, prefixListString">
				switch (algo) {
				case "http://www.w3.org/2001/10/xml-exc-c14n#WithComments":
					var excl = true;
					// fall thru
				case "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments":
					var withComments = true;
					break;
				case "http://www.w3.org/2001/10/xml-exc-c14n#":
					var excl = true;
					// fall thru
				case "http://www.w3.org/TR/2001/REC-xml-c14n-20010315":
					break;
				default:
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);	// unknown or not supported
				}
				if (xs.isInstanceOf(elm, xs.infoset.element))
					var doc = this.makeItRoot(elm, excl ? this.CANON_NO_XMLATTR: undefined);	// regardless of whether excl or not, inherit all namespaces and render them at the root element at this point to make it easy to find out ancient namespaces. later it will be eliminated in the case of excl
				else
					var doc = elm;
				if (excl) {
					// render namespace for each element for now, later it will be normalized
					this.exclRenderNS(doc.element);
					// eliminate superfluous namespaces at the root element unless they are on the list
					var prefixList = prefixListString ? prefixListString.split(/\s+/): [];
					var ns = doc.element.prefix;
					if (!ns) ns = "xmlns";
					var newNS = [];
					for (var i = 0, a = doc.element.xmlnsAttributes, l = a.length; i < l; i++) {
						var e = a[i];
						if (e.name == ns || this.findInAttributes(e.name, doc.element.attributes) || this.findInList(e.name, prefixList))
							newNS.push(e);
					}
					doc.element.xmlnsAttributes = newNS;
				}
				// normalize the namespace declarations according to the algorithm -- infoset can handle the rest of the stuff
				this.normalizeNamespace(doc);
				return(xs.infoset.print(doc, withComments ? xs.infoset.PRINT_DEFAULT: xs.infoset.PRINT_NO_COMMENT));
			</function>

			<function name="removeElement" params="e, name, ns" script="false">
				for (var i = 0, c = e.children, length = c.length; i < length; i++) {
					if (xs.isInstanceOf(c[i], xs.infoset.element)) {
						if (c[i].name == name && c[i].namespace && c[i].namespace == ns) {
							// remove this element from e.children and redo
							e.children = this.arrayRemoveElement(e.children, i);
							return(this.removeElement(e, name, ns));
						}
						else
							this.removeElement(c[i], name, ns);
					}
				}
				return(e);
			</function>

			<function name="transform" params="o, algo, optParams" script="false">
				switch (algo) {
				case "http://www.w3.org/2001/10/xml-exc-c14n#WithComments":
				case "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments":
					var withComments = true;
					// fall thru
				case "http://www.w3.org/2001/10/xml-exc-c14n#":
				case "http://www.w3.org/TR/2001/REC-xml-c14n-20010315":
					var ret = new Crypt.bin.chunk.String(this.canon(o, algo, optParams));
					ret.canonicalized = true;
					break;
				case "http://www.w3.org/2000/09/xmldsig#enveloped-signature":
					var ret = this.removeElement(o, "Signature", "http://www.w3.org/2000/09/xmldsig#");
					break;
				case "http://www.w3.org/2000/09/xmldsig#base64":
					// process only value
					if (xs.isInstanceOf(o, xs.infoset.element))
						var val = o.children[0].value;
					else
						var val = o.element.children[0].value;
					var ret = new Chunk(val);
					ret.canonicalized = true;
					break;
				default:
					if (this.transformCallback) {
						var ret = this.transformCallback(o, algo);
						if (ret) {
							ret.canonicalized = true;
							break;
						}
					}
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);	// unknown or not supported
				}
				return(ret);
			</function>

			<function name="transformBin" params="o, algo" script="false">
				switch (algo) {
				case "http://www.w3.org/2000/09/xmldsig#base64":
					var ret = new Chunk(o.toRawString());
					break;
				default:
					if (this.transformCallback) {
						var ret = this.transformCallback(o, algo);
						if (ret)
							break;
					}
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);	// unknown or not supported
				}
				return(ret);
			</function>

			<function name="transforms" params="target, ref" script="false">
				if (!ref.uri || ref.uri.charAt(0) == '#') {
					// node-set
					for (var i = 0; i < ref.transforms.length; i++) {
						var o = (target instanceof Chunk) ? xs.infoset.scan(target.toRawString()): target;
						target = this.transform(o, ref.transforms[i].algorithm, ref.transforms[i].prefixList);
					}
					if (!target.hasOwnProperty("canonicalized")) {
						// apply a default transform
						var o = (target instanceof Chunk) ? xs.infoset.scan(target.toRawString()): target;
						target = this.transform(o, "http://www.w3.org/TR/2001/REC-xml-c14n-20010315");
					}
				}
				else {
					// binary octets
					for (var i = 0; i < ref.transforms.length; i++)
						target = this.transformBin(target, ref.transforms[i].algorithm);
				}
				return(target);
			</function>

			<function name="sigDigest" params="o, ref" script="false">
				// o must be an instance of Chunk or xs.infoset.document or xs.infoset.element, depending on the algorithm and the URI type
				// type check is not done here because of the complexity of the conditions of c14n
				var t = this.transforms(o, ref);
				var digest_f = new this.DigestAlgorithm(ref.digest.algorithm);
				digest_f.hash.update(t);
				return(digest_f.hash.close());
			</function>

			<function name="sigCanon" params="elm, algo, sig_f" script="false">
				var target = new Crypt.bin.chunk.String(this.canon(elm, algo));
				if (sig_f.hash) {
					sig_f.hash.update(target);
					var H = sig_f.hash.close();
				}
				else
					var H = target;
				return(H);
			</function>

			<function name="findElement" params="elm, elmName" script="false">
				for (var i = 0, c = elm.children; i < c.length; i++) {
					if (!xs.isInstanceOf(c[i], xs.infoset.element))
						continue;
					if (c[i].hasOwnProperty("namespace") && c[i].namespace == "http://www.w3.org/2000/09/xmldsig#" && c[i].name == elmName)
						return(c[i]);
					var sigNode = this.findElement(c[i], elmName);
					if (sigNode) return(sigNode);
				}
			</function>

			<function name="findX509KeyBySubject" params="name" script="false">
				var list = this.certificates.getList();
				for (var i in list) {
					var c = list[i];
					if (c.tbsCertificate.subject == name)
						return(c.getKey());
				}
			</function>

			<function name="removeSpaces" params="s" script="false">
				return(s.replace(/^[ \r\n\t]*(.*)[ \r\n\t]*$/, "$1"));
			</function>

			<function name="findX509Key" params="x509o" script="false">
				if (x509o.subjectNames.length)
					var sn = this.removeSpaces(x509o.subjectNames[0]);
				if (x509o.issuerSerials.length) {
					var isName = this.removeSpaces(x509o.issuerSerials[0].name);
					var isNumber = new Arith.Integer(x509o.issuerSerials[0].serial);
				}
				var list = this.certificates.getList();
				for (var i in list) {
					var c = list[i];
					var cert = c.tbsCertificate;
					if (x509o.skis.length) {
						if (cert.extensions.subjectKeyId.comp(x509o.skis[0]) == 0)
							return(c.getKey());
					}
					else if (x509o.subjectNames.length) {
						if (cert.subject == sn)
							return(c.getKey());
					}
					else if (x509o.issuerSerials.length) {
						if (cert.issuer == isName && cert.serialNumber.comp(isNumber) == 0)
							return(c.getKey());
					}
				}
			</function>

			<!-- should be a constructor? remain as a function for now to contrast with the following function -->
			<function name="getKeyInfoFromKey" params="key">
				// set keyInfo by key
				var keyInfo = new this.KeyInfo();
				if (key.keyName)
					keyInfo.keyName = key.keyName;
				if (key.hasOwnProperty("rsaKey")) {
					keyInfo.rsaKey = xs.newInstanceOf(Crypt.xml.keyInfo.rsaKey);
					keyInfo.rsaKey.modulus = key.rsaKey.modulus.toChunk();
					keyInfo.rsaKey.exponent = key.rsaKey.exponent.toChunk();
				}
				if (key.hasOwnProperty("dsaKey")) {
					keyInfo.dsaKey = xs.newInstanceOf(Crypt.xml.keyInfo.dsaKey);
					keyInfo.dsaKey.p = key.dsaKey.p.toChunk();
					keyInfo.dsaKey.q = key.dsaKey.q.toChunk();
					keyInfo.dsaKey.g = key.dsaKey.g.toChunk();
					keyInfo.dsaKey.y = key.dsaKey.y.toChunk();
				}
				if (key.hasOwnProperty("dhKey")) {
					keyInfo.dhKey = xs.newInstanceOf(Crypt.xml.keyInfo.dhKey);
					keyInfo.dhKey.p = key.dhKey.p.toChunk();
					keyInfo.dhKey.generator = key.dhKey.g.toChunk();
					keyInfo.dhKey.pub = key.dhKey.pub.toChunk();
					// not fill out all the DH params
				}
				return(keyInfo);
			</function>

			<function name="getDHSharedKey" params="keyInfo, privDH">
				if (!privDH || !privDH.hasOwnProperty("dhKey"))
					throw new Crypt.Error(Crypt.error.kCryptKeyNotFound);

				// calculate the DH shared secret (ZZ)
				var originator = this.getKeyFromKeyInfo(keyInfo.agreementMethod.originator, privDH);
				var recipient = this.getKeyFromKeyInfo(keyInfo.agreementMethod.recipient, privDH);
				// check if the both parties share the same DH parameters
				if (originator.dhKey.p.comp(privDH.dhKey.p) != 0 || originator.dhKey.g.comp(privDH.dhKey.g) != 0)
					throw new Crypt.Error(Crypt.error.kCryptParameterError);
				var m = new Arith.Module(new Arith.Z(), originator.dhKey.p);
				var ss = m.exp(originator.dhKey.pub, privDH.dhKey.priv);
				var ZZ = Crypt.pkcs1.I2OSP(ss, originator.dhKey.p.sizeof());

				// get the shared key size
				var keySize = this.proto.encryptionMethod.keySize;
				if (!keySize) keySize = Crypt.xmlProto.encryptionAlgorithm.getKeySize(this.proto.encryptionMethod.encryptionAlgorithm).toString(10);

				// crunch the all materials
				var h = (new Crypt.xmlProto.DigestAlgorithm(keyInfo.agreementMethod.digestAlgorithm)).hash;
				var n = Math.ceil((keySize / 8) / h.outputSize);
				var km = new Chunk();
				for (var i = 1; i <= n; i++) {
					h.reset();
					/*
					var M = new Chunk();
					M.append(ZZ);
					var counter = i.toString(10);
					if (counter.length == 1) counter = "0" + counter;
					M.append(new Crypt.bin.chunk.String(counter));
					M.append(new Crypt.bin.chunk.String(this.proto.encryptionMethod.encryptionAlgorithm));
					M.append(keyInfo.agreementMethod.nonce);
					M.append(new Crypt.bin.chunk.String(keySize));
					h.update(M);
					*/
					h.update(ZZ);
					var counter = i.toString(10);
					if (counter.length == 1) counter = "0" + counter;
					h.update(new Crypt.bin.chunk.String(counter));
					h.update(new Crypt.bin.chunk.String(this.proto.encryptionMethod.encryptionAlgorithm));
					h.update(keyInfo.agreementMethod.nonce);
					h.update(new Crypt.bin.chunk.String(keySize));
					km.append(h.close());
				}
				return(new Crypt.KeyInfo({sym: km}));
			</function>

			<function name="getKeyFromKeyInfo" params="keyInfo, givenKey" script="false">
				var key;

				if (keyInfo.hasOwnProperty("encryptedKey") && keyInfo.encryptedKey) {
					// this is a predetermined way to get the key
					// if it has a different way to encode cipherValue or to make up keyInfo from the default way, you need to decrypt the encryptedKey element first and then set a result as the 'key' explicitly
					var sym = (new Crypt.XML(keyInfo.encryptedKey, givenKey, this.keyring, this.certificates)).decrypt();
					if (sym) return(new Crypt.KeyInfo({sym: sym}));
				}

				if (givenKey && givenKey.complete())
					return(givenKey);

				// first, look up keyring
				var keyName = givenKey ? givenKey.keyName: keyInfo.keyName;
				if (keyName) {
					key = this.keyring.get(keyName);
					if (!key)
						key = this.findX509KeyBySubject(keyName);
					if (key && key.complete()) return(key);
					key = undefined;
				}

				// next, make up RSA or DSA or DH key if present
				if (keyInfo.hasOwnProperty("rsaKey")) {
					key = new Crypt.KeyInfo();
					if (keyName) key.keyName = keyName;
					key.rsaKey = new key.RsaKey();
					key.rsaKey.modulus = new Arith.Integer(keyInfo.rsaKey.modulus);
					key.rsaKey.exponent = new Arith.Integer(keyInfo.rsaKey.exponent);
				}
				else if (keyInfo.hasOwnProperty("dsaKey")) {
					key = new Crypt.KeyInfo();
					if (keyName) key.keyName = keyName;
					key.dsaKey = new key.DsaKey();
					key.dsaKey.p = new Arith.Integer(keyInfo.dsaKey.p);
					key.dsaKey.q = new Arith.Integer(keyInfo.dsaKey.q);
					key.dsaKey.g = new Arith.Integer(keyInfo.dsaKey.g);
					key.dsaKey.y = new Arith.Integer(keyInfo.dsaKey.y);
				}
				else if (keyInfo.hasOwnProperty("dhKey")) {
					key = new Crypt.KeyInfo();
					if (keyName) key.keyName = keyName;
					key.dhKey = new key.DHKey();
					key.dhKey.p = new Arith.Integer(keyInfo.dhKey.p);
					key.dhKey.g = new Arith.Integer(keyInfo.dhKey.generator);
					key.dhKey.pub = new Arith.Integer(keyInfo.dhKey.pub);
				}
				if (key) return(key);

				// check out x509 
				for (var i = 0; i < keyInfo.x509.length; i++) {
					var x509o = keyInfo.x509[i];
					if (x509o.certs.length > 0) {
						// extract a public key at the end of the cert chain
						var cert = this.certificates.processCerts(x509o.certs);
						if (cert) key = cert.getKey();
					}
					if (key) return(key);
					key = this.findX509Key(x509o);
					if (key) {
						// try out the keyring
						var priv = this.keyring.get(key.keyName);
						return(priv ? priv: key);
					}
				}

				// key agreement method
				if (keyInfo.agreementMethod.algorithm) {
					switch (keyInfo.agreementMethod.algorithm) {
					case "http://www.w3.org/2001/04/xmlenc#dh":
						return(this.getDHSharedKey(keyInfo, givenKey));
					default:
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					}
				}

				// finally, try out retrievalMethod
				if (keyInfo.retrievalMethod.uri) {
					if (!this.urrInstance)
						throw new Crypt.Error(Crypt.error.kCryptNoRetrievalMethod);
					var refChunk = this.urrInstance.getURI(keyInfo.retrievalMethod.uri, this);	// must return a chunk
					switch (keyInfo.retrievalMethod.type) {
					case "http://www.w3.org/2000/09/xmldsig#rawX509Certificate":
						// must be a binary X509 certificate
						return((Crypt.x509.decode(refChunk)).getKey());
					case "http://www.w3.org/2000/09/xmldsig#DSAKeyValue":
					case "http://www.w3.org/2000/09/xmldsig#RSAKeyValue":
					case "http://www.w3.org/2000/09/xmldsig#X509Data":
					case "http://www.w3.org/2000/09/xmldsig#PGPData":
					case "http://www.w3.org/2000/09/xmldsig#SPKIData":
					case "http://www.w3.org/2000/09/xmldsig#MgmtData":
					case "http://www.w3.org/2001/04/xmlenc#DHKeyValue":
						return(this.getKeyFromKeyInfo(xs.parse(refChunk.toRawString()), givenKey));
					default:
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					}
				}
			</function>

			<function name="resolveKey" params="given">
				if (this.resolvedKey)
					return(this.resolvedKey);
				if (given && this.key)
					return(this.key);
				this.resolvedKey = this.getKeyFromKeyInfo(this.proto.keyInfo, this.key);
				if (!this.resolvedKey)
					throw new Crypt.Error(Crypt.error.kCryptKeyNotFound);
				return(this.resolvedKey);
			</function>

			<function name="setDefaultSignature" params="proto">
				var signedInfo = new Crypt.xml.signature.SignedInfo();
				signedInfo.canonicalizationAlgorithm = proto.signedInfo.canonicalizationAlgorithm ? proto.signedInfo.canonicalizationAlgorithm: this.defaultSignatureProperties.signedInfo.canonicalizationAlgorithm;
				signedInfo.signatureAlgorithm = proto.signedInfo.signatureAlgorithm ? proto.signedInfo.signatureAlgorithm: this.defaultSignatureProperties.signedInfo.signatureAlgorithm;
				if ("references" in proto.signedInfo && proto.signedInfo.references.length)
					signedInfo.references = proto.signedInfo.references;
				else
					signedInfo.references = new Array();	// @@ <array> seems not to be instantiated in rootvm...
				if ("hmacOutputLength" in proto.signedInfo && proto.signedInfo.hmacOutputLength)
					signedInfo.hmacOutputLength = proto.signedInfo.hmacOutputLength;
				proto.signedInfo = signedInfo;
			</function>

			<function name="setDefaultEncryption" params="proto">
				var encryptionMethod = new Crypt.xml.encryption.EncryptionMethod();
				if (proto instanceof this.EncryptedKey)
					var defprop = this.defaultEncryptionProperties.keyInfo.encryptedKey;
				else
					var defprop = this.defaultEncryptionProperties;
				encryptionMethod.encryptionAlgorithm = proto.encryptionMethod.encryptionAlgorithm ? proto.encryptionMethod.encryptionAlgorithm: defprop.encryptionMethod.encryptionAlgorithm;
				if (proto.encryptionMethod.digestAlgorithm)
					encryptionMethod.digestAlgorithm = proto.encryptionMethod.digestAlgorithm;
				if (proto.encryptionMethod.oaepParams.length)
					encryptionMethod.oaepParams = proto.encryptionMethod.oaepParams;
				proto.encryptionMethod = encryptionMethod;
			</function>
		</object>
		<function name="XMLProto" params="proto, key, keyring, certificates, urrInstance" prototype="Crypt.xmlProto">
			this.keyring = keyring ? keyring : new Crypt.Keyring();
			this.certificates = certificates ? certificates : new Crypt.Certificate();
			this.urrInstance = urrInstance;
			this.proto = proto;
			this.key = key;
		</function>

		<object name="xml" prototype="Crypt.xmlProto">
			<object name="sign" prototype="Crypt.xmlProto">
				<function name="addRef" params="o, ref, callback">
					this.transformCallback = callback;
					if (!ref)
						ref = new this.SignatureReference({});
					if (ref.transforms.length == 0)
						ref.transforms.push(new this.SignatureReferenceTransform({algorithm: this.defaultSignatureProperties.signedInfo.references[0].transforms[0].algorithm}));
					ref.digest = new Crypt.xml.signatureReference.Digest(ref.digest);	// for serialization
					if (ref.digest.algorithm == "")
						ref.digest.algorithm = this.defaultSignatureProperties.signedInfo.references[0].digest.algorithm;
					if (ref.digest.value.length == 0)
						ref.digest.value = this.sigDigest.call(this, o, ref);
					this.proto.signedInfo.references.push(ref);
					return(ref);
				</function>

				<function name="done">
					var key = this.resolveKey(true);
					var sig_f = new this.SignatureAlgorithm(this.proto, key, true);
					if (this.proto.signedInfo.hasOwnProperty("__xs__infoset"))
						var doc = this.proto.signedInfo.__xs__infoset;
					else {
						var is = xs.infoset.serialize(this.proto);
						var doc = this.findElement(is, "SignedInfo");
					}
					var H = this.sigCanon(doc, this.proto.signedInfo.canonicalizationAlgorithm, sig_f);
					if (sig_f.sig)
						this.proto.value = sig_f.sig.sign(H);
					else
						this.proto.value = H;
					return(xs.infoset.print(xs.infoset.serialize(this.proto)));
				</function>
			</object>
			<function name="Sign" prototype="Crypt.xml.sign">
				Crypt.XMLProto.apply(this, arguments);
			</function>

			<object name="verify" prototype="Crypt.xmlProto">
				<null name="verified" script="false"/>

				<function name="addRefID" params="o, id, callback">
					// o must include all namespaces referred in the element
					this.transformCallback = callback;
					for (var i = 0; i < this.proto.signedInfo.references.length; i++) {
						var ref = this.proto.signedInfo.references[i];
						if (ref.uri == '#' + id)
							return(this.verified[i] = ref.digest.value.comp(this.sigDigest.call(this, o, ref)) == 0);
					}
					// return undefined
				</function>

				<function name="addRef" params="o, ref, callback">
					this.transformCallback = callback;
					if (ref.digest.value.comp(this.sigDigest.call(this, o, ref)) != 0)
						return(false);
					// where am I
					for (var i = 0, protoRefs = this.proto.signedInfo.references; i < protoRefs.length; i++) {
						if (protoRefs[i].uri == ref.uri && ref.digest.value.comp(protoRefs[i].digest.value) == 0) {
							this.verified[i] = true;
							return(true);
						}
					}
				</function>

				<function name="done" params="originalDocument">
					var key = this.resolveKey(false);
					var sig_f = new this.SignatureAlgorithm(this.proto, key, false);
					if (originalDocument) {
						if (typeof originalDocument == "string")
							var doc = this.findElement(xs.infoset.scan(originalDocument), "SignedInfo");
						else if (xs.isInstanceOf(originalDocument, xs.infoset.element))
							var doc = this.findElement(originalDocument, "SignedInfo");
						if (!doc)
							throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
					}
					else {
						if (this.proto.signedInfo.hasOwnProperty("__xs__infoset"))
							var doc = this.proto.signedInfo.__xs__infoset;
						else if (this.proto.hasOwnProperty("__xs__infoset"))
							var doc = this.findElement(this.proto.__xs__infoset, "SignedInfo");
						if (!doc)
							throw new Crypt.Error(Crypt.error.kCryptMalformedInput);	// must be parsed by xs.infoset.parse()
					}
					var H = this.sigCanon(doc, this.proto.signedInfo.canonicalizationAlgorithm, sig_f);
					for (var i = 0; i < this.proto.signedInfo.references.length; i++) {
						if (!(i in this.verified && this.verified[i]))
							return(false);
					}
					return(sig_f.sig.verify(H, this.proto.value));
				</function>
			</object>
			<function name="Verify" prototype="Crypt.xml.verify">
				Crypt.XMLProto.apply(this, arguments);
				this.verified = new Array(this.proto.signedInfo.references.length);
			</function>

			<function name="encrypt" params="o, ref">
				if (!this.enc_f) {
					var key = this.resolveKey(true);
					this.enc_f = new this.EncryptionAlgorithm(this.proto, key, false);
				}
				var enc_f = this.enc_f;
				if (enc_f.encMode)
					var cipherValue = enc_f.encMode.encrypt(enc_f.enc, o);
				else
					var cipherValue = enc_f.enc.encrypt(o);
				this.proto.cipherData = xs.newInstanceOf(Crypt.xml.encryption.cipherData);	// to make sure to be subject to serialization
				if (ref)
					this.proto.cipherData.reference = ref;
				else
					this.proto.cipherData.value = cipherValue;
				return(cipherValue);
			</function>

			<function name="decrypt" params="o">
				if (!this.enc_f) {
					var key = this.resolveKey(false);
					this.enc_f = new this.EncryptionAlgorithm(this.proto, key, true);
				}
				if (!o)
					o = this.proto.cipherData.value;
				var enc_f = this.enc_f;
				if (enc_f.encMode)
					return(enc_f.encMode.decrypt(enc_f.enc, o));
				else
					return(enc_f.enc.decrypt(o));
			</function>

			<function name="c14n" params="doc, algo">
				if (!algo)
					algo = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
				try {
					var is = xs.infoset.scan(doc);
					if (!is)
						return("");	// seems like xs.infoset.scan doesn't throw an exception at "XML error"
				} catch (e) {
					return("");
				}
				return(this.canon(is, algo));
			</function>

			<!-- prototype of URI-Reference retriever -->
			<object name="uriReferenceRetriever">
				<function name="getURI" params="uri"/>	<!-- returns content of the URI in chunk -->
			</object>
		</object>
		<function name="XML" params="proto, key, keyring, certificates, urrInstance" prototype="Crypt.xml">
			if (xs.isInstanceOf(proto, Crypt.xml.signature)) {
				this.setDefaultSignature(proto);
				this.sign = new Crypt.xml.Sign(proto, key, keyring, certificates, urrInstance);
				this.verify = new Crypt.xml.Verify(proto, key, keyring, certificates, urrInstance);
			}
			else {	// must be EncryptedData or EncryptedKey
				this.setDefaultEncryption(proto);
				Crypt.XMLProto.apply(this, arguments);
			}
		</function>
	</patch>

	<import href="xmlGrammar.xs"/>

	<!-- templates with the default parameters -->
	<program>
		Crypt.xmlProto.defaultSignatureProperties = {
			id: "",
			signedInfo: {
				canonicalizationAlgorithm: "http://www.w3.org/TR/2001/REC-xml-c14n-20010315",
				signatureAlgorithm: "http://www.w3.org/2000/09/xmldsig#rsa-sha1",
				hmacOutputLength: 0,
				references: [{
					uri: "",
					transforms: [{algorithm: "http://www.w3.org/TR/2001/REC-xml-c14n-20010315"}],
					digest: {algorithm: "http://www.w3.org/2000/09/xmldsig#sha1"},
				}],
			},
			keyInfo: {},
			object: {}
		};

		Crypt.xmlProto.defaultEncryptionProperties = {
			type: "",
			id: "",
			mimeType: "",
			encryptionMethod: {
				encryptionAlgorithm: "http://www.w3.org/2001/04/xmlenc#aes128-cbc",
				digestAlgorithm: "http://www.w3.org/2000/09/xmldsig#sha1",
				oaepParams: new Chunk(),
			},
			keyInfo: {
				encryptedKey: {
					encryptionMethod: {
						encryptionAlgorithm: "http://www.w3.org/2001/04/xmlenc#rsa-1_5",
						digestAlgorithm: "http://www.w3.org/2000/09/xmldsig#sha1",
						oaepParams: new Chunk(),
					},
				},
			},
		};
	</program>
</package>