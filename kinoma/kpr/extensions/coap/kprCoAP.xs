<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package script="true">
    <import href="kpr.xs" link="dynamic"/>

    <object name="CoAP">
        <!-- Client -->

        <object name="client" c="KPR_CoAP_client_destructor">
            <function name="createRequest" params="uri, method" c="KPR_CoAP_client_createRequest"/>
            <function name="send" params="request" c="KPR_CoAP_client_send"/>

            <function name="get autoToken" c="KPR_CoAP_client_is_autoToken" enum="true"/>
            <function name="set autoToken" c="KPR_CoAP_client_set_autoToken"/>

            <!--
                onResponse(request, response)
                onAck(request)
                onDeliveryFailure(request, failure)
                onError(err)
            -->
        </object>

        <function name="Client" params="" prototype="CoAP.client" c="KPR_CoAP_Client"/>

        <!-- Request -->

        <object name="request" c="KPR_CoAP_request_destructor">
            <function name="get confirmable" c="KPR_CoAP_request_is_confirmable" enum="true"/>
            <function name="set confirmable" c="KPR_CoAP_request_set_confirmable"/>

            <function name="get type" c="KPR_CoAP_request_get_type" enum="true"/>

            <function name="get messageId" c="KPR_CoAP_request_get_messageId" enum="true"/>

            <function name="get method" c="KPR_CoAP_request_get_method" enum="true"/>
            <function name="set method" c="KPR_CoAP_request_set_method"/>

            <function name="get token" c="KPR_CoAP_request_get_token" enum="true"/>
            <function name="set token" c="KPR_CoAP_request_set_token"/>

            <function name="get payload" c="KPR_CoAP_request_get_payload" enum="true"/>
            <function name="set payload" c="KPR_CoAP_request_set_payload"/>

            <function name="get contentFormat" c="KPR_CoAP_request_get_contentFormat" enum="true"/>

            <function name="setPayload" params="payload, contentFormat" c="KPR_CoAP_request_setPayload"/>

            <function name="get options" c="KPR_CoAP_request_get_options" enum="true"/>
            <function name="addOption" params="option, value" c="KPR_CoAP_request_addOption"/>

            <function name="get observe" c="KPR_CoAP_request_is_observe" enum="true"/>
            <function name="set observe" c="KPR_CoAP_request_set_observe"/>

            <function name="get uri" c="KPR_CoAP_request_get_uri" enum="true"/>
            <function name="get host" c="KPR_CoAP_request_get_host" enum="true"/>
            <function name="get port" c="KPR_CoAP_request_get_port" enum="true"/>
            <function name="get path" c="KPR_CoAP_request_get_path" enum="true"/>
            <function name="get query" c="KPR_CoAP_request_get_query" enum="true"/>
            <!--
                onResponse(response)
                onAck()
                onDeliveryFailure(failure)
            -->
        </object>

        <!-- Server -->

        <object name="server" c="KPR_CoAP_server_destructor">
            <function name="get port" c="KPR_CoAP_server_get_port" enum="true"/>

            <function name="start" params="port" c="KPR_CoAP_server_start"/>
            <function name="stop" params="" c="KPR_CoAP_server_stop"/>

            <function name="bind" params="path, callback, info" c="KPR_CoAP_server_bind"/>

            <function name="getSession" params="sessionId" c="KPR_CoAP_server_getSession"/>
        </object>

        <function name="Server" params="" prototype="CoAP.server" c="KPR_CoAP_Server"/>

        <!-- Session -->

        <object name="session" c="KPR_CoAP_session_destructor">
            <function name="get id" c="KPR_CoAP_session_get_id" enum="true"/>

            <function name="get confirmable" c="KPR_CoAP_session_is_confirmable" enum="true"/>
            <function name="get type" c="KPR_CoAP_session_get_type" enum="true"/>
            <function name="get messageId" c="KPR_CoAP_session_get_messageId" enum="true"/>
            <function name="get method" c="KPR_CoAP_session_get_method" enum="true"/>
            <function name="get token" c="KPR_CoAP_session_get_token" enum="true"/>

            <function name="get payload" c="KPR_CoAP_session_get_payload" enum="true"/>
            <function name="get contentFormat" c="KPR_CoAP_session_get_contentFormat" enum="true"/>

            <function name="get options" c="KPR_CoAP_session_get_options" enum="true"/>

            <function name="get remoteIP" c="KPR_CoAP_session_get_remoteIP" enum="true"/>
            <function name="get remotePort" c="KPR_CoAP_session_get_remotePort" enum="true"/>

            <function name="get uri" c="KPR_CoAP_session_get_uri" enum="true"/>
            <function name="get host" c="KPR_CoAP_session_get_host" enum="true"/>
            <function name="get port" c="KPR_CoAP_session_get_port" enum="true"/>
            <function name="get path" c="KPR_CoAP_session_get_path" enum="true"/>
            <function name="get query" c="KPR_CoAP_session_get_query" enum="true"/>

            <function name="createResponse" params="" c="KPR_CoAP_session_createResponse"/>
            <function name="send" params="response" c="KPR_CoAP_session_send"/>

            <!-- Observe -->
            <function name="get observe" c="KPR_CoAP_session_is_observe" enum="true"/>

            <function name="acceptObserve" params="" c="KPR_CoAP_session_acceptObserve"/>
            <function name="endObserve" params="" c="KPR_CoAP_session_endObserve"/>

            <!-- manual ack -->
            <function name="get autoAck" c="KPR_CoAP_server_is_autoAck" enum="true"/>
            <function name="set autoAck" c="KPR_CoAP_server_set_autoAck"/>
            <function name="sendAck" params="" c="KPR_CoAP_session_sendAck"/>
        </object>

        <!-- Response -->

        <object name="response" c="KPR_CoAP_response_destructor">
            <function name="get confirmable" c="KPR_CoAP_response_is_confirmable" enum="true"/>
            <function name="set confirmable" c="KPR_CoAP_response_set_confirmable"/>

            <function name="get type" c="KPR_CoAP_response_get_type" enum="true"/>

            <function name="get messageId" c="KPR_CoAP_response_get_messageId" enum="true"/>

            <function name="get code" c="KPR_CoAP_response_get_code" enum="true"/>

            <function name="setCode" params="cls, detail" c="KPR_CoAP_response_setCode"/>

            <function name="get token" c="KPR_CoAP_response_get_token" enum="true"/>

            <function name="get payload" c="KPR_CoAP_response_get_payload" enum="true"/>
            <function name="set payload" c="KPR_CoAP_response_set_payload"/>

            <function name="get contentFormat" c="KPR_CoAP_response_get_contentFormat" enum="true"/>

            <function name="setPayload" params="payload, contentFormat" c="KPR_CoAP_response_setPayload"/>

            <function name="get options" c="KPR_CoAP_response_get_options" enum="true"/>
            <function name="addOption" params="option, value" c="KPR_CoAP_response_addOption"/>

            <function name="get observe" c="KPR_CoAP_response_is_observe" enum="true"/>

            <function name="get uri" c="KPR_CoAP_response_get_uri" enum="true"/>
            <function name="get host" c="KPR_CoAP_response_get_host" enum="true"/>
            <function name="get port" c="KPR_CoAP_response_get_port" enum="true"/>
            <function name="get path" c="KPR_CoAP_response_get_path" enum="true"/>
            <function name="get query" c="KPR_CoAP_response_get_query" enum="true"/>
        </object>

        <object name="Method">
            <!--
            GET    =  1
            POST   =  2
            PUT    =  3
            DELETE =  4
            -->
        </object>

        <object name="Option">
            <!--
            IfMatch        =  1
            UriHost        =  3
            ETag           =  4
            IfNoneMatch    =  5
            UriPort        =  7
            LocationPath   =  8
            UriPath        = 11
            ContentFormat  = 12
            MaxAge         = 14
            UriQuery       = 15
            Accept         = 17
            LocationQuery  = 20
            ProxyUri       = 35
            ProxyScheme    = 39
            Size1          = 60
            -->
        </object>

        <function name="get useChunk" c="KPR_CoAP_get_useChunk" enum="true"/>
        <function name="set useChunk" c="KPR_CoAP_set_useChunk" enum="true"/>
    </object>

    <program c="KPR_CoAP_patch"/>
</package>
