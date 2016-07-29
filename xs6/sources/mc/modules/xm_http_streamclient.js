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

import HTTP from "http";
import {Socket} from "socket";
import HTTPClient from "HTTPClient";
import TimeInterval from "timeinterval";

class HTTPStreamClient extends HTTPClient {
	constructor(uri, options) {
		super(uri, options);
	}
	start(s) {
		// @@ !!! file object only!!! object needs to have length property
		super.start(s);
		this.streamLength = this.stream.length;
		this.remainingLength = this.stream.length;
		this.sock.onWritable = this.onWritable;	
	}
	// callbacks from the socket
	onConnect() {
		let http = this.http;
		this.send(http.method + " " + http.url + " HTTP/" + http.version + "\r\n");
		http.sendHeaders();
		http.initMessage();
		http.content = null;
		let o = new TimeInterval(()=>{
			if(http.remainingLength <= 0) {
				o.close();
				return;
			}
			if(this.bytesWritable <=0) return;
			let n = this.bytesWritable < 1024 ? this.bytesWritable:1024;
			n = http.remainingLength <= n ? http.remainingLength : n;
			let ab = http.stream.read(undefined, n);
			try{
				this.send(ab);	
			}
			catch(error){
				trace("error in socket send\n");
				// need to close timer??
			}
			http.onDataSent(n);
			http.remainingLength -= n;
		}, 40);
		http.intv = o;
		o.start();
	}
	onWritable(n){
		trace("onWritable: " + n + "\n");
	}
	onError() {
		if(this.http.intv) this.http.intv.close();
		super.onError();
	}
	onClose() {
		if(this.http.intv) this.http.intv.close();
		super.onClose()
		
	}
}

export default HTTPStreamClient;
