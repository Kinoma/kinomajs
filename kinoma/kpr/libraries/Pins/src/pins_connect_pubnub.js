//@module
//
//     Copyright (C) 2010-2015 Marvell International Ltd.
//     Copyright (C) 2002-2010 Kinoma, Inc.
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//
exports.instantiate = function(Pins, settings)
{
    var result = Object.create(pnPins);
    result.Pins = Pins;
    result.settings = settings;
    result.configure();
    return result;
}

var pnPins = {
    Pins: null,
    settings: null,
    pubnub: null,
    pubKey: null,
    subKey: null,
    channelName: null,
    sendQueue: null,
    open: true,
    deviceID: null,
    requestID: null,

    configure: function() {
        var pubKey = this.pubKey = ("pubKey" in this.settings) ? this.settings.pubKey : undefined;
        var subKey = this.subKey = ("subKey" in this.settings) ? this.settings.subKey : undefined;
        var channelName = this.channelName = ("channelName" in this.settings) ? this.settings.channelName : undefined;
        var deviceID = this.deviceID = Math.floor(Math.random() * (1000 - 0) + 0);
        sendQueue = this.sendQueue = [];
        this.requestID = 0;

        pubnub = this.pubnub = PUBNUB.init({
            publish_key: pubKey,
            subscribe_key: subKey
        });

        this.open = true;
        this.waiting = false;

        pubnub.subscribe({
            channel : channelName,
            message : function(message, env, channel) {
                if ((message['type'] == 'response') && (message['to'] == deviceID)) {
                    for (var i = 0; i < sendQueue.length; i++) {
                        var item = sendQueue[i];
                        if (item.message.requestID != message.requestID)
                            continue;
                        if (!("repeat" in item.message))
                            sendQueue.splice(i, 1);
                        if (item.callback)
                            item.callback.call(null, ("body" in message) ? message.body : undefined);
                        return;
                    }
                }
            }
         });

        /* Sanity check to make sure device connected to PubNub channel */
        pubnub.publish({                                     
            channel : channelName,
            message : {body: "Device with ID "+deviceID+" is connected.\n"}
        });
    },
    invoke: function(path, requestObject, callback) {
        if (typeof requestObject == "function") {
            callback = requestObject;
            requestObject = undefined;
        } 
        var item = {callback: callback, message: {type: "request", from: this.deviceID, requestID: this.requestID++, path: path, requestObject: requestObject}};
        this.sendQueue.push(item);
        if (this.open){
            this.pubnub.publish({                                     
                channel : this.channelName,
                message : item.message
            });
        }
    },
    repeat: function(path, requestObject, condition, callback) {
        if (typeof condition == "function") {
            callback = condition;
            condition = requestObject;
            requestObject = undefined;
        }
        var item = {callback: callback, message: {type: "request", from:this.deviceID, requestID: this.requestID++, path: path, requestObject: requestObject, repeat: true}};

        if (typeof condition == "number") {
            item.message.interval = condition;
        } else {
            item.message.timer = condition;
        }
        this.sendQueue.push(item);
        if (this.open) {
            this.pubnub.publish({                                     
                channel : this.channelName,
                message : item.message
            });
        }
        return {
            close: function() {
                item.message.repeat = false;
                this.pubnub.publish({                                     
                    channel : this.channelName,
                    message : item.message
                });
            }
        };
    },

    close: function(callback) {
        this.sendQueue.forEach(function(item) {
            item.message.repeat = false;
            this.pubnub.publish({                                     
                channel : this.channelName,
                message : item.message
            });
        }, this);
        this.open = false;
        this.sendQueue = [];
    }
};


