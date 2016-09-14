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

import System from "system";
import Files from "files";
import Crypt from "crypt";
import Bin from "bin";

class Archive @ "xs_archive_destructor" {
	constructor(archive) @ "xs_archive_constructor";
	getCode(path) @ "xs_archive_getCode";
};

const KEYFILE = "xssig.priv";
let SIGFILE = "mc.xsa.sig";
if (System.device == "host")
	SIGFILE += ".host";

export default {
	verify(path) {
		let archive = new Archive();
		let code = archive.getCode(path);
		let h, sig;
		if (code) {
			let sigfile = Files.read(SIGFILE);
			if (!sigfile)
				return false;
			let elm = JSON.parse(String.fromArrayBuffer(sigfile)).find(a => a.path == path);
			if (!elm || !(sig = elm.sig))
				return false;
			h = (new Crypt.SHA1()).process(code);
			sig = Bin.decode(sig);
		}
		else {
			let File = require.weak("file");
			path = Files.applicationDirectory + "/" + path;
			let f = new File(path);
			if (!f)
				return false;
			let buf = new ArrayBuffer(128);
			let dgst = new Crypt.SHA1();
			while (f.read(undefined, 128, buf))
				dgst.update(buf);
			f.close();
			h = dgst.close();
			sig = Files.read(path + ".sig");
			if (!sig)
				return false;
		}
		let pk = new Crypt.PKCS1_5(Crypt.X509.decodeSPKI(Files.read("xssig.der")));
		return pk.verify(h, sig);
	},
	sign(paths, archive) {
		if (!Files.getInfo(KEYFILE))
			return [];
		let a = archive ? new Archive(archive) : undefined;
		let pk = new Crypt.PKCS1_5(Crypt.PKCS8.parse(Files.read(KEYFILE)), true);
		let sign = c => c ? pk.sign((new Crypt.SHA1()).process(c)) : undefined;
		return paths.map(path => sign(a ? a.getCode(path) : Files.read(path)));
	},
};
