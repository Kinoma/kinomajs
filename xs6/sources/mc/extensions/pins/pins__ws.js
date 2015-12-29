/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
		this.clients = [];
		this.repeats = [];	//multiple
		this.ws = new WebSocketServer(port);
		this.ws.onStart = this.onStart;		
		this.ws.configuration = configuration;
	},
	close() {
		this.ws.close();
	},
	onStart(client){
		PinsWebSocketHandler.repeats.push({}); 
		PinsWebSocketHandler.clients.push(client);
		client.onopen = function(){
		};
		client.onclose = function(){
		};
		client.onmessage = function(ev){
			// trace("onmessage: "+ ev.data + "\n");
			GPIOPin.led(1,1);
			try{
				let obj = JSON.parse(ev.data);
				let index = PinsWebSocketHandler.clients.indexOf(client);
				if("repeat" in obj){
					let repeats = PinsWebSocketHandler.repeats[index];
					let id = obj.id + "_"
					if(obj.repeat){
						if( id in repeats){ /* its already on repeat */
							repeats[id].close(); 
							delete repeats[id];
						}
						repeats[id] = Pins.repeat(obj.path, obj.interval ? obj.interval : obj.timer, function(result){
							GPIOPin.led(0, 1);
							client.send(JSON.stringify({body: result, inReplyTo: obj.id}));
							GPIOPin.led(0,0);
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
					Pins.invoke(obj.path, obj.requestObject ? obj.requestObject : undefined, function(result){
						GPIOPin.led(0, 1);
						client.send(JSON.stringify({body: result, inReplyTo: obj.id}));
						GPIOPin.led(0, 0);
					})
				}	
			}
			catch(error){
				trace("error happended!!!\n");
			}
			GPIOPin.led(1, 0);
		};
		client.onerror = function(){
			trace("onerror\n");
			let index = PinsWebSocketHandler.clients.indexOf(client);
			if(index < 0) return; // arealdy removed all
			let repeats = PinsWebSocketHandler.repeats[index];
			//client may be already dead
			for(var id in repeats){
				repeats[id].close();
				delete repeats[id];
			}
			client.close();	//close the client
			// should we close the websocket server or not? we should keep the server alive. 
			PinsWebSocketHandler.clients.splice(index, 1);
			PinsWebSocketHandler.repeats.splice(index, 1);
		}
	},
};
export default PinsWebSocketHandler;
