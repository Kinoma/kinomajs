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
import {WebSocketClient} from "websocket";

export default class PinsWebSocketClientHandler {
	constructor(url) {
		this.ws = new WebSocketClient(url);
		this.id = (Math.random() * 10000000) | 0;
		this.messages = [];
	 	let self = this;
	 	self.open = false;
		this.ws.onopen = function(){
			trace("PinsWebSocketClientHandler: onopen\n")
			self.open = true;
			for(let i = 0; i < self.messages.length; i++)
				self.ws.send(JSON.stringify(self.messages[i].content));
		};
		this.ws.onclose = function(){
			trace("PinsWebSocketClientHandler: onclose\n")
			self.open = false;
		};
		this.ws.onmessage = function(e){
			let message = JSON.parse(e.data);
			// trace("onmessage: " + e.data + "\n");
			for(let i = 0; i < self.messages.length; i++){
				let item = self.messages[i];
				if(item.content.id != message.inReplyTo) continue;
				if(!("repeat" in item.content)) // invoke
					self.messages.splice(i, 1);
				if(item.callback)
					item.callback("body" in message ? message.body : undefined);
				return;
			}
		};
		this.ws.onerror = function(){
			trace("PinsWebSocketClientHandler: onerror\n");

			// TODO: we should do something here, maybe close the socket when server is gone unexpectedly

		}
	};
	close() {
		this.open = false;
		this.ws.close();
		this.messages = [];	// empty the sending queue
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
		this.messages.push(message);
		
		if(this.open)
			this.ws.send(JSON.stringify(message.content));

		return{
			ws: self.ws,
			close:function(){
				message.content.repeat = false;
				// should we remove the message from the queue
				if(self.open)
					self.ws.send(JSON.stringify(message.content));
			}
		}
	};
	invoke(path, opt, callback){
		if (typeof opt == "function") {
			callback = opt;
			opt = undefined;
		}
		let message = {callback: callback, content: {path: path, requestObject: opt, id: this.id++}};
		this.messages.push(message);
		if(this.open){
			this.ws.send(JSON.stringify(message.content));	
		}
		
	};
};
