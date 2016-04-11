/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
import Crypt from "crypt";
import Arith from "arith";
import Files from "files";
import File from "file";
import Bin from "bin";

var argv = process.execArgv();
var f1 = new File("/k2/ca.ski", 1);
var f2 = new File("/k2/ca.subject", 1);
var sha1 = new Crypt.SHA1();
for (var argi = 1, argc = argv.length; argi < argc; argi++) {
	var crt = Files.read(argv[argi]);
	var ski = Crypt.X509.decodeSKI(crt);
	if (ski.byteLength > 20) {
		trace("SKI too long!\n");
		continue;
	}
	if (ski.byteLength < 20) {
		var buf = new Uint8Array(20);
		buf.fill(0);
		buf.set(new Uint8Array(ski), 20 - ski.byteLength);
		ski = buf.buffer;
	}
	f1.write(ski);
	console.log((new Arith.Integer(ski)).toString(16, 40));
	var x509 = Crypt.X509.decode(crt);
	var tbs = Crypt.X509.decodeTBS(x509.tbs);
	sha1.reset();
	sha1.update(tbs.subject);
	var subject = sha1.close();
	f2.write(subject);
}
f1.close();
f2.close();
