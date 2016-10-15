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
//Timer object constructor
class Timer{
	constructor (id, trueDelay){
		this.id = id;
		this.message = null;
		this.trueDelay = trueDelay;
	}
	timeOutHandler (callback,delay,id){
		this.message = new MessageWithObject("/timeOutHandler"+id, {delay:delay});
		let promise = this.message.invoke();
		promise.then(() => {
			callback.call(this);
			let idIndex = findIndex(id);
			timerArray.splice(idIndex,1);
		});
	}
	timeIntervalHandler (callback,delay,id){
		var time = Date.now();
		this.message = new MessageWithObject("/timeIntervalHandler"+id, {delay:delay});
		let promise = this.message.invoke();
		promise.then(() => {
			var current_delay = Date.now() - time;
			var delta_time_delay = this.trueDelay - (current_delay - this.trueDelay);
			if(Handler.get("/timeIntervalHandler"+id)){
				callback.call(null);
				this.timeIntervalHandler(callback,delay,id);
			}
		}).catch( err => {
		trace("err"+err+"\n");});
	}
}
// array for holding timer objects
var timerArray = [];

// global id for timer assignment
var id = 0;
function findIndex(id){
	for (let i=0;i < timerArray.length;i++){
		if (timerArray[i].id == id ){
			return i;
		}
	}
	return false;
}
// mimic Javascript window.setTimeout function
export function setTimeout(callback,delay){
	id = id+1;
	Handler.bind("/timeOutHandler"+id, {
	  onInvoke: (handler, message) => {
	    let delay = message.requestObject.delay;
	    handler.wait(delay);
	  },
	});
	//create new time Object
	timerArray.push(new Timer(id));
	timerArray[timerArray.length-1].timeOutHandler(callback,delay,id);
	// return id of timer Object
	return id;
}
// stop a timeout
export function clearTimeout(id){
	let idIndex = findIndex(id);
	if (idIndex !== false){
		timerArray[idIndex].message.cancel();
		timerArray.splice(idIndex,1);
	}
}
// mimic Javascript window.setInterval function
export function setInterval(callback,delay){
	id = id+1;
	Handler.bind("/timeIntervalHandler"+id, {
	  onInvoke: (handler, message) => {
	    let delay = message.requestObject.delay;
	    handler.wait(delay);
	  },
	});
	timerArray.push(new Timer(id,delay));
	timerArray[timerArray.length-1].timeIntervalHandler(callback,delay,id);
	return id;
}

// stop an interval
export function clearInterval(id){
	let idIndex = findIndex(id);
	if(idIndex !== false){
		timerArray[idIndex].message.cancel();
		Handler.remove(Handler.get("/timeIntervalHandler"+id));
		timerArray.splice(idIndex,1);
	}
}
