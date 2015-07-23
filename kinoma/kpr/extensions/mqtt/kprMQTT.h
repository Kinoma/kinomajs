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
#ifndef __KPRMQTT__
#define __KPRMQTT__

//--------------------------------------------------
// MQTT
//--------------------------------------------------

void KPR_MQTTClient(xsMachine* the);
void KPR_mqttclient_destructor(void *it);
void KPR_mqttclient_connect(xsMachine* the);
void KPR_mqttclient_disconnect(xsMachine* the);
void KPR_mqttclient_publish(xsMachine* the);
void KPR_mqttclient_subscribe(xsMachine* the);
void KPR_mqttclient_unsubscribe(xsMachine* the);

void KPR_MQTTMessage(xsMachine* the);
void KPR_mqttmessage_destructor(void *it);
void KPR_mqttmessage_get_payload(xsMachine* the);
void KPR_mqttmessage_set_payload(xsMachine* the);
void KPR_mqttmessage_get_qos(xsMachine* the);
void KPR_mqttmessage_set_qos(xsMachine* the);
void KPR_mqttmessage_get_retain(xsMachine* the);
void KPR_mqttmessage_set_retain(xsMachine* the);

#endif
