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
	<namespace prefix="fsk" uri="http://www.kinoma.com/Fsk/1"/>
	<namespace prefix="ds" uri="http://www.w3.org/2000/09/xmldsig#"/>
	<namespace prefix="xenc" uri="http://www.w3.org/2001/04/xmlenc#"/>

	<patch prototype="Crypt">
		<object name="keyInfo">
			<chunk name="__ber__"/>

			<!-- I don't know the reason but it causes a fatal error in xsGrammar if 'pattern' appears in an object which is not defined in xs. (i.e. as a host object) -->
			<object name="keyInfoProto" pattern="/ds:KeyInfo">
				<string name="keyName" pattern="ds:KeyName"/>
				<object name="rsaKey" pattern="ds:KeyValue/ds:RSAKeyValue">
					<custom name="modulus" pattern="ds:Modulus" io="Arith.integer"/>
					<custom name="exponent" pattern="ds:Exponent" io="Arith.integer"/>
					<custom name="privExponent" pattern="fsk:PrivExponent" io="Arith.integer"/>
					<custom name="prim1" pattern="fsk:Prim1" io="Arith.integer"/>
					<custom name="prim2" pattern="fsk:Prim2" io="Arith.integer"/>
					<custom name="exponent1" pattern="fsk:Exponent1" io="Arith.integer"/>
					<custom name="exponent2" pattern="fsk:Exponent2" io="Arith.integer"/>
					<custom name="coefficient" pattern="fsk:Coefficient" io="Arith.integer"/>
				</object>
				<function name="RsaKey" params="properties" prototype="Crypt.keyInfo.keyInfoProto.rsaKey">
					if (properties) {
						for (var p in properties)
							this[p] = properties[p];
					}
				</function>
				<object name="dsaKey" pattern="ds:KeyValue/ds:DSAKeyValue">
					<custom name="p" pattern="ds:P" io="Arith.integer"/>
					<custom name="q" pattern="ds:Q" io="Arith.integer"/>
					<custom name="g" pattern="ds:G" io="Arith.integer"/>
					<custom name="y" pattern="ds:Y" io="Arith.integer"/>
					<custom name="x" pattern="fsk:X" io="Arith.integer"/>
				</object>
				<function name="DsaKey" params="properties" prototype="Crypt.keyInfo.keyInfoProto.dsaKey">
					if (properties) {
						for (var p in properties)
							this[p] = properties[p];
					}
				</function>
				<object name="ecdsaKey" pattern="ds:KeyValue/fsk:ECDSAKeyValue">	<!-- not a standard keyInfo -->
					<custom name="p" pattern="fsk:p" io="Arith.integer"/>
					<custom name="a" pattern="fsk:a" io="Arith.integer"/>
					<custom name="b" pattern="fsk:b" io="Arith.integer"/>
					<custom name="G" pattern="fsk:G" io="Arith.ecpoint"/>
					<custom name="n" pattern="fsk:n" io="Arith.integer"/>
					<custom name="Qu" pattern="fsk:Qu" io="Arith.ecpoint"/>
					<custom name="du" pattern="fsk:du" io="Arith.integer"/>
				</object>
				<function name="ECDsaKey" params="properties" prototype="Crypt.keyInfo.keyInfoProto.ecdsaKey">
					if (properties) {
						for (var p in properties)
							this[p] = properties[p];
					}
				</function>
				<object name="dhKey" pattern="ds:KeyValue/ds:DHKeyValue">
					<custom name="p" pattern="xenc:Prime" io="Arith.integer"/>
					<custom name="g" pattern="xenc:Generator" io="Arith.integer"/>
					<custom name="pub" pattern="xenc:Public" io="Arith.integer"/>
					<custom name="priv" pattern="fsk:Private" io="Arith.integer"/>
				</object>
				<function name="DHKey" params="properties" prototype="Crypt.keyInfo.keyInfoProto.dhKey">
					if (properties) {
						for (var p in properties)
							this[p] = properties[p];
					}
				</function>
				<chunk name="sym" pattern="ds:KeyValue/fsk:SymKeyValue"/>

				<function name="parse" params="b" script="false">
					return Crypt.keyInfo.pem.parse(b, "RSA PRIVATE KEY");	// DOES NOT SUPPORT any kind of the key format except PEM/RSA
					// return(xs.parse(b));	// @@ the returned object seems not to be recognized as an instance of keyInfoProto... so it needs to 'cast' to toString on this object for example...
				</function>

				<function name="serialize" params="o" script="false">
					// !!! NO xs.serialize in xs6...
					if (o.__ber__)
						return o.__ber__;
					return(xs.serialize(o));
				</function>

				<function name="complete" params="">
					return(
						(this.hasOwnProperty("rsaKey")) ||
						(this.hasOwnProperty("dsaKey")) ||
						(this.hasOwnProperty("sym")));
					// DH key is never complete by itself
				</function>

				<target name="debug">
					<function name="toString" params="">
						var s = "";
						if (this.keyName)
							s += "keyName: " + this.keyName + "\n";
						else
							s += "(anonymous key name)\n";
						if (this.hasOwnProperty("rsaKey")) {
							s += "rsa.modulus: " + this.rsaKey.modulus.toString(16) + "\n";
							s += "rsa.exponent: " + this.rsaKey.exponent.toString(16) + "\n";
							if (!this.rsaKey.privExponent.isNaN())
								s += "rsa.privExponent: " + this.rsaKey.privExponent.toString(16) + "\n";
							if (!this.rsaKey.prim1.isNaN())
								s += "rsa.prim1: " + this.rsaKey.prim1.toString(16) + "\n";
							if (!this.rsaKey.prim2.isNaN())
								s += "rsa.prim2: " + this.rsaKey.prim2.toString(16) + "\n";
							if (!this.rsaKey.exponent1.isNaN())
								s += "rsa.exponent1: " + this.rsaKey.exponent1.toString(16) + "\n";
							if (!this.rsaKey.exponent2.isNaN())
								s += "rsa.exponent2: " + this.rsaKey.exponent2.toString(16) + "\n";
							if (!this.rsaKey.coefficient.isNaN())
								s += "rsa.coefficient: " + this.rsaKey.coefficient.toString(16) + "\n";
						}
						if (this.hasOwnProperty("dsaKey")) {
							s += "dsa.p: " + this.dsaKey.p.toString(16) + "\n";
							s += "dsa.q: " + this.dsaKey.q.toString(16) + "\n";
							s += "dsa.g: " + this.dsaKey.g.toString(16) + "\n";
							s += "dsa.y: " + this.dsaKey.y.toString(16) + "\n";
							if (!this.dsaKey.x.isNaN())
								s += "dsa.x: " + this.dsaKey.x.toString(16) + "\n";
						}
						if (this.hasOwnProperty("dhKey")) {
							s += "dh.p: " + this.dhKey.p.toString(16) + "\n";
							s += "dh.g: " + this.dhKey.g.toString(16) + "\n";
							if (!this.dhKey.pub.isNaN())
								s += "dh.pub: " + this.dhKey.pub.toString(16) + "\n";
							if (!this.dhKey.priv.isNaN())
								s += "dh.priv: " + this.dhKey.priv.toString(16) + "\n";
						}
						if (this.hasOwnProperty("sym"))
							s += "sym: " + (new Integer(this.sym)).toString(16) + "\n";
						return(s);
					</function>
				</target>
			</object>	<!-- keyInfo.keyInfoProto -->

			<!-- the following functions returns an keyInfo object as a result -->
			<object name="pkcs8">
				<function name="parse" params="ber">
					// keyFile must be PrivateKeyInfo defined in pkcs#8
					var keyFile = Crypt.ber.decode(ber);
					var tmpKey = new Crypt.KeyInfo();
					var pub = keyFile[2];
					var priv = Crypt.ber.decode(keyFile[3][1]);
					var oid = pub[1][1];
					if (Crypt.ber.oideq(oid, [1, 2, 840, 113549, 1, 1, 1])) {
						// RSA
						tmpKey.rsaKey = new tmpKey.RsaKey({modulus: priv[2][1], exponent: priv[3][1], privExponent: priv[4][1], prim1: priv[5][1], prim2: priv[6][1], exponent1: priv[7][1], exponent2: priv[8][1], coefficient: priv[9][1]});
					}
					else if (Crypt.ber.oideq(oid, [1, 2, 840, 10040, 4, 1])) {
						// DSA
						tmpKey.dsaKey = new tmpKey.DsaKey({p: pub[2][1][1], q: pub[2][2][1], g: pub[2][3][1], y: priv[1], x: priv[1]});
					}
					else if (Crypt.ber.oideq(oid, [1, 2, 840, 10046, 2, 1])) {
						// DH
						var params = pub[2];
						tmpKey.dhKey = new tmpKey.DHKey({p: params[1][1], g: params[2][1], priv: priv[1]});
						// q: params[3][1], j: params[4][1], seed: params[5][1][1], pgenCounter: params[5][2][1]
					}
					else
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					return(tmpKey);
				</function>
			</object>

			<object name="pkcs1">
				<function name="parsePK" params="algo, pubKey">
					var tmpKey = new Crypt.KeyInfo();
					var algoId = algo.algorithm;
					if (Crypt.ber.oideq(algoId, [1, 2, 840, 113549, 1, 1, 1]) ||
					    Crypt.ber.oideq(algoId, [1, 3, 14, 3, 2, 11]) ||
					    Crypt.ber.oideq(algoId, [2, 5, 8, 1, 1])) {
						// RSA
						var pub = Crypt.ber.decode(pubKey);
						tmpKey.rsaKey = new tmpKey.RsaKey({modulus: pub[1][1], exponent: pub[2][1]});
					}
					else if (Crypt.ber.oideq(algoId, [1, 2, 840, 10040, 4, 1]) ||
						 Crypt.ber.oideq(algoId, [1, 3, 14, 3, 2, 12])) {
						// DSA
						var pub = Crypt.ber.decode(pubKey);
						tmpKey.dsaKey = new tmpKey.DsaKey();
						if (algo.parameters) {
							var params = algo.parameters;	// should be already integers
							tmpKey.dsaKey.p = params[1][1];
							tmpKey.dsaKey.q = params[2][1];
							tmpKey.dsaKey.g = params[3][1];
						}
						else
							throw new Crypt.Error(Crypt.error.kCryptParameterError);
						if (pub[0] == 0x02)	// integer	// @@ in practical? depends on the algorithm?
							tmpKey.dsaKey.y = pub[1];
						else
							tmpKey.dsaKey.y = pub[1][1];
					}
					else if (Crypt.ber.oideq(algoId, [1, 2, 840, 10046, 2, 1])) {
						// DH
						var pub = Crypt.ber.decode(pubKey);
						tmpKey.dhKey = new tmpKey.DHKey({p: algo.parameters[1][1], g: algo.parameters[2][1], pub: pub[1]});
					}
					else if (Crypt.ber.oideq(algoId, [1, 2, 840, 10045, 2, 1])) {
						// ECDSA
						tmpKey.ecdsaKey = new tmpKey.ECDsaKey();
						// throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);
					}
					else
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					return(tmpKey);
				</function>

				<function name="parse" params="ber">
					// keyfile must be SubjectPublicKeyInfo defined in X509
					// this is almost the same as pkcs8 but slightly different...
					var keyFile = Crypt.ber.decode(ber);
					var pubKey = Crypt.x509.decodePublicKeyInfo(keyFile);
					return(this.parsePK(pubKey.algorithm, pubKey.subjectPublicKey));
				</function>
			</object>

			<object name="pem">
				<function name="parse" params="ber, keyword">
					var keyFile = Crypt.ber.decode(ber);
					// what format is this?
					var tmpKey = new Crypt.KeyInfo();
					switch (keyword) {
					case "RSA PRIVATE KEY":
						tmpKey.rsaKey = new tmpKey.RsaKey({modulus: keyFile[2][1], exponent: keyFile[3][1], privExponent: keyFile[4][1], prim1: keyFile[5][1], prim2: keyFile[6][1], exponent1: keyFile[7][1], exponent2: keyFile[8][1], coefficient: keyFile[9][1]});
						tmpKey.__ber__ = ber;
						break;
					case "DSA PRIVATE KEY":
						tmpKey.dsaKey = new tmpKey.DsaKey({p: keyFile[2][1], q: keyFile[3][1], g: keyFile[4][1], x: keyFile[5][1], y: keyFile[6][1]});
						break;
					default:
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					}
					return(tmpKey);
				</function>
			</object>
		</object>
		<function name="KeyInfo" params="properties" prototype="Crypt.keyInfo.keyInfoProto">
			if (properties) {
				for (var p in properties)
					this[p] = properties[p];
				for (var p in properties.sandbox)
					this[p] = properties.sandbox[p];
			}
		</function>
	</patch>
</package>

