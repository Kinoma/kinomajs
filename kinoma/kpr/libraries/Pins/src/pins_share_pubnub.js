//@module
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
var Pins = require("pins.js");
 
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
    pubKey: null,
    subKey: null,
    channelName: null,

    configure: function() {
        var pubKey = this.pubKey = ("pubKey" in this.settings) ? this.settings.pubKey : undefined;
        var subKey = this.subKey = ("subKey" in this.settings) ? this.settings.subKey : undefined;
        var channelName = this.channelName = ("channelName" in this.settings) ? this.settings.channelName : undefined;

        var pubnub = this.pubnub = PUBNUB.init({
            publish_key: pubKey,
            subscribe_key: subKey
        });

        pubnub.repeats = [];

        pubnub.subscribe({
            channel : channelName,
            message : function(message, env, channel) {
                if (message['type'] == 'request') {
                    var requestPath = message['path'];
                    var sender = message['from'];
                    var requestID = message['requestID'];
                    var callback = message['callback'];
                    var requestObject = message['requestObject'];


                    if (!(message['repeat'] === undefined)) {
                        if (message['repeat']) {
                            var repeat = Pins.repeat(requestPath, ("interval" in message) ? parseInt(message['interval']) : message['timer'], function(result)  {
                                pubnub.publish({                                     
                                    channel : channelName,
                                    message : {type: "response", to: sender, requestID: requestID, body: result}
                                });
                            });
                            pubnub.repeats.push({repeat: repeat, message: message});
                        } else {
                            pubnub.repeats.every(function(item, index) {
                                if (item.message['requestID'] !== message['requestID']) return true;        //@@ not enough to compare request.id - need something connection specific.
                                item.repeat.close();
                                pubnub.repeats.splice(index, 1);
                            });
                        }
                    } else {
                        Pins.invoke(requestPath, ("requestObject" in message) ? requestObject : undefined, function(result) {
                            pubnub.publish({                                     
                                channel : channelName,
                                message : {type: "response", to: sender, requestID: requestID, body: result}
                            });
                        });
                    }
                }
            },
            connect: function pub() {
                pubnub.publish({                                     
                    channel : channelName,
                    message : {body: "hello from pins share!"}
                });
            }
        });



    },
    close: function(callback) {
        this.pubnub.open = false;
        this.pubnub.repeats.every(function(item) {
            item.repeat.close();
        });
        this.pubnub.repeats.length = 0;
    },
};


