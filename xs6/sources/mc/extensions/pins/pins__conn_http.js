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
import HTTPClient from "HTTPClient";
import HTTPServer from "HTTPServer";

var PinsHTTPRepeatCallbackServer;
var clientID = (Math.random() * 1000) | 0;

export default class PinsHTTPClientHandler {
	constructor(url) {
		this.host = url;
		this.clientID = clientID++;	//increase ID, should be uniquie
		this.id = (Math.random() * 10000000) | 0;
		this.callbacks = {}; //map
		let self = this;
		
		if (!PinsHTTPRepeatCallbackServer){
			PinsHTTPRepeatCallbackServer = new HTTPServer({port: 9900});
			PinsHTTPRepeatCallbackServer.onRequest = function(http){ // when should we shut down the server???
				// trace("http.url: " + http.url + "\n");
				let dec = http.decomposeUrl(http.url);	
				let result = http.content ? JSON.parse(String.fromArrayBuffer(http.content)) : undefined;
				// http.setHeader("Connection", "close");
				http.response();
				let clientID = parseInt(dec.path[1]);
				let callbackID = String(dec.path[2]);
				if(self.clientID == clientID && (callbackID in self.callbacks) && result){
					self.callbacks[callbackID](result);
				}
			}
		}
	};
	send(message){
		let obj = this._makeQuery(message);
		let url = this.host;
		let callback = message.callback;
		let id = message.content.id;
		let self = this;
		if(obj.query)
			url = this.host + "?" + obj.query;

		if(callback && message.content.repeat){
			let callbackURL = "http://*:" + PinsHTTPRepeatCallbackServer.sock.port + "/callback/" + this.clientID + "/"  + message.content.id;
			url = url + "&callback=" + encodeURIComponent(callbackURL);
			this.callbacks[String(message.content.id)] = callback;
		}
		let client = new HTTPClient(url);
		// trace("request url: " + url + "\n");
		client.setHeader("Connection", "close");
		client.onDataReady = function(n){
			if(callback) callback(String.fromArrayBuffer(n));
		};
		if (obj.params){
			client.method = "POST";
			client.setHeader("Content-Type", "application/json");
			client.setHeader("Content-Length", obj.params.length);
			client.start(obj.params);
		}
		else
			client.start();
	};
	close() {
		PinsHTTPRepeatCallbackServer.close();
	};
	_makeQuery(message){
		let query = "path=" + encodeURIComponent(message.content.path);
		let params = "";
		if("repeat" in message.content){
			query = query +  "&repeat=" + (message.content.repeat ? "on" : "off");
			if("interval" in message.content)
				query = query + "&interval=" + message.content.interval;
			else if ("timer" in message.content)
				query = query + "&timer=" + message.content.timer;
			else
				trace("Bad parameter\n");
		}
		else{// Invoke
			if("requestObject" in message.content)
				params = JSON.stringify(message.content.requestObject);
		}
		
		return {
			query: query,
			params: params, 
		}	

	};
	repeat(path, opt, ti, callback){
		let self = this;
		if(typeof ti == "function")	{
			callback = ti;
			ti = opt;
			opt = undefined;
		}
		let message = {callback: callback,content: {path: path, requestObject: opt, repeat: true, id: this.id++}};		
		if(typeof ti == "number") message.content.interval = ti;
		else message.content.timer = ti;
		this.send(message);

		return{
			close:function(){
				message.content.repeat = false;
				// should we remove the message from the queue
				self.send(message);
			}
		}
	};
	invoke(path, opt, callback){
		if (typeof opt == "function") {
			callback = opt;
			opt = undefined;
		}
		let message = {callback: callback, content: {path: path, requestObject: opt, id: this.id++}};
		this.send(message);
	};
};
