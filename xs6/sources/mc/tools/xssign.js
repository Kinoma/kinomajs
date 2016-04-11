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
import Files from "files";
import Arith from "arith";

var pk = new Crypt.PKCS1_5(Crypt.PKCS8.parse(Files.read("xssig.priv")), true);

var argv = process.execArgv();
for (var argi = 1, argc = argv.length; argi < argc; argi++) {
	var xsb = argv[argi];
	var i = xsb.indexOf(".xsb");
	var sig = (i >= 0 ? xsb.substr(0, i) : xsb) + ".sig";
	var h = (new Crypt.SHA1()).process(Files.read(xsb));
	console.log("h = " + (new Arith.Integer(h)).toString(16));
	Files.write(sig, pk.sign(h));
}
