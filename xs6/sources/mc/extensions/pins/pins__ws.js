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
import {WebSocketServer} from "websocket";
import Pins from "pins";
import GPIOPin from "pinmux";
import System from "system";

var PinsWebSocketHandler = {
	init(port, configuration) {
		let self = this;
		this.clients = [];
		this.repeats = [];	//multiple

		this.ws = new WebSocketServer(port);
		this.ws.onStart = function(client) {
			self.onStart.call(self, client);
		};
		this.ws.configuration = configuration;
	},
	close() {
		this.clients.forEach(function(e){
			this.removeClient(e);
		}, this);
		this.clients = [];
		this.repeats = [];
		this.ws.close();
	},
	removeClient(client){
		let index = this.clients.indexOf(client);
		if(index < 0) return; // arealdy removed all
		let repeats = this.repeats[index];
		//client may be already dead
		for(var id in repeats){
			repeats[id].close();
			delete repeats[id];
		}
		// client.close();	//close the client
		// should we close the websocket server or not? we should keep the server alive. 
		this.clients.splice(index, 1);
		this.repeats.splice(index, 1);
	},
	onStart(client){
		let self = this;
		this.repeats.push({}); 
		this.clients.push(client);
		client.onopen = function(){
		};
		client.onclose = function(){
		};
		let onClose = client.onClose;
		client.onClose = function(){	
			self.removeClient(client);// we need notification to remove our repeat
			onClose();
		};
		client.onmessage = function(ev){
			//trace("PinsWebSocketHandler: onmessage: "+ ev.data + "\n");
			try{
				let obj = JSON.parse(ev.data);
				let index = self.clients.indexOf(client);
				if("repeat" in obj){
					let repeats = self.repeats[index];
					let id = obj.id + "_"
					if(obj.repeat){
						if( id in repeats){ /* its already on repeat */
							repeats[id].close(); 
							delete repeats[id];
						}
						repeats[id] = Pins.repeat(obj.path, obj.interval ? obj.interval : obj.timer, function(result){
							client.send(JSON.stringify({body: result, inReplyTo: obj.id}));
						});	
					}
					else{
						// stop repeat
						if(id in repeats){
							repeats[id].close();
							delete repeats[id];
						}
					}
				}
				else{
					//invoke
					let cb = function(result){
						client.send(JSON.stringify({body: result, inReplyTo: obj.id}));
					};
					if (obj.requestObject != undefined)
						Pins.invoke(obj.path, obj.requestObject, cb)
					else
						Pins.invoke(obj.path, cb)
				}	
			}
			catch(error){
				trace("error happended!!!\n");
			}
		};
		client.onerror = function(){
			trace("onerror\n");
			self.removeClient(client);
			// client.close();	//close the client
		}
	},
};
export default PinsWebSocketHandler;
