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
<package><program><![CDATA[


const CoAP = {
    Type: {
        Con: 0,
        Non: 1,
        Ack: 2,
        Rst: 3,
    },

    Method: {
        GET: 1,
        POST: 2,
        PUT: 3,
        DELETE: 4
    },

    Option: {
        // RFC 7252 core
        IfMatch: 1,
        UriHost: 3,
        ETag: 4,
        IfNoneMatch: 5,
        UriPort: 7,
        LocationPath: 8,
        UriPath: 11,
        ContentFormat: 12,
        MaxAge: 14,
        UriQuery: 15,
        Accept: 17,
        LocationQuery: 20,
        ProxyUri: 35,
        ProxyScheme: 39,
        Size1: 60,

        // draft-ietf-core-block-15
        Block2: 23,
        Block1: 27,
        Size2: 28,

        // draft-ietf-core-observe-14
        Observe: 6,
    },

    OptionFormat: {
        Empty: 0,
        Opaque: 1,
        Uint: 2,
        String: 3,
    },

    ContentFormat: {
        PlainText: 0,
        LinkFormat: 40,
        XML: 41,
        OctetStream: 42,
        EXI: 47,
        JSON: 50
    },

    Observe: {
        Register: 0,
        Deregister: 1
    },


    Port: 5683,

    get useChunk() @ "KPR_CoAP_get_useChunk",
    set useChunk(val) @ "KPR_CoAP_set_useChunk"
};

CoAP.Client = class @ "KPR_CoAP_client_destructor" {
    constructor() {
        this._constructor();
    }

    createRequest(url, method=CoAP.Type.GET) {
        let request = this._createRequest(url, method);
        return request;
    }

    get(url, callback) {
        return this.send({url, method:CoAP.Method.GET}, callback);
    }

    post(url, payload, callback) {
        return this.send({url, method:CoAP.Method.POST}, payload, callback);
    }

    put(url, payload, callback) {
        return this.send({url, method:CoAP.Method.PUT}, payload, callback);
    }

    observe(url, callback) {
        return this.send({
            url,
            method:CoAP.Method.GET,
            observe: true,
        }, callback);
    }

    send(request, payload, callback) {
        if (typeof request == 'object') {
            if (!(request instanceof CoAP.Request)) {
                let url = request.url ? request.url : request.uri;
                if (!url) throw "Bad request";

                let method = request.method;

                let request2 = this.createRequest(url, method)

                delete request.url;
                delete request.uri;
                delete request.method;
                for (let key in request) {
                    request2[key] = request[key];
                }

                request = request2;
            }
        } else {
            request = this.createRequest(url, CoAP.Method.GET);
        }

        switch (typeof payload) {
            case 'function':
                callback = payload;
                break;

            case 'object':
                request.payload = payload;
                if (!request.method || request.method == CoAP.Method.GET) request.method = CoAP.Method.POST;
                break;
        }

        if (callback) {
            request.onResponse = callback;
        }

        return this._send(request);
    }

    // private stuff
    _constructor(/*  */) @ "KPR_CoAP_Client";
    _createRequest(/* url, method */) @ "KPR_CoAP_client_createRequest";
    _send(/* request */) @ "KPR_CoAP_client_send";
};

CoAP.Request = class @ "KPR_CoAP_request_destructor" {
    get confirmable() @ "KPR_CoAP_request_is_confirmable";
    set confirmable(val) @ "KPR_CoAP_request_set_confirmable";

    get type() @ "KPR_CoAP_request_get_type";

    get messageId() @ "KPR_CoAP_request_get_messageId";

    get method() @ "KPR_CoAP_request_get_method";
    set method(val) @ "KPR_CoAP_request_set_method";

    get token() @ "KPR_CoAP_request_get_token";
    set token(val) @ "KPR_CoAP_request_set_token";

    get payload() @ "KPR_CoAP_request_get_payload";
    set payload(val) @ "KPR_CoAP_request_set_payload";

    get contentFormat() @ "KPR_CoAP_request_get_contentFormat";

    setPayload(payload, contentFormat) @ "KPR_CoAP_request_setPayload";

    get options() @ "KPR_CoAP_request_get_options";
    addOption(option, value) @ "KPR_CoAP_request_addOption";

    get observe() @ "KPR_CoAP_request_is_observe";
    set observe(val) @ "KPR_CoAP_request_set_observe";

    get uri() @ "KPR_CoAP_request_get_uri";
    get url() @ "KPR_CoAP_request_get_uri";
    get host() @ "KPR_CoAP_request_get_host";
    get port() @ "KPR_CoAP_request_get_port";
    get path() @ "KPR_CoAP_request_get_path";
    get query() @ "KPR_CoAP_request_get_query";

};

CoAP.Server = class @ "KPR_CoAP_server_destructor" {
    constructor() @ "KPR_CoAP_Server";

    get port() @ "KPR_CoAP_server_get_port";

    start(port) @ "KPR_CoAP_server_start";
    stop() @ "KPR_CoAP_server_stop";

    bind(path, callback, info) @ "KPR_CoAP_server_bind";

    getSession(sessionId) @ "KPR_CoAP_server_getSession";
};

CoAP.Session = class @ "KPR_CoAP_session_destructor" {
    get id() @ "KPR_CoAP_session_get_id";

    get confirmable() @ "KPR_CoAP_session_is_confirmable";
    get type() @ "KPR_CoAP_session_get_type";
    get messageId() @ "KPR_CoAP_session_get_messageId";
    get method() @ "KPR_CoAP_session_get_method";
    get token() @ "KPR_CoAP_session_get_token";

    get payload() @ "KPR_CoAP_session_get_payload";
    get contentFormat() @ "KPR_CoAP_session_get_contentFormat";

    get options() @ "KPR_CoAP_session_get_options";

    get remoteIP() @ "KPR_CoAP_session_get_remoteIP";
    get remotePort() @ "KPR_CoAP_session_get_remotePort";

    get uri() @ "KPR_CoAP_session_get_uri";
    get url() @ "KPR_CoAP_session_get_uri";
    get host() @ "KPR_CoAP_session_get_host";
    get port() @ "KPR_CoAP_session_get_port";
    get path() @ "KPR_CoAP_session_get_path";
    get query() @ "KPR_CoAP_session_get_query";

    createResponse() @ "KPR_CoAP_session_createResponse";
    send(response) @ "KPR_CoAP_session_send";

    /* Observe */
    get observe() @ "KPR_CoAP_session_is_observe";

    acceptObserve() @ "KPR_CoAP_session_acceptObserve";
    endObserve() @ "KPR_CoAP_session_endObserve";

    /* manual ack */
    get autoAck() @ "KPR_CoAP_server_is_autoAck";
    set autoAck() @ "KPR_CoAP_server_set_autoAck";
    sendAck() @ "KPR_CoAP_session_sendAck";
};


CoAP.Response = class @ "KPR_CoAP_response_destructor" {
    get confirmable() @ "KPR_CoAP_response_is_confirmable";
    set confirmable(val) @ "KPR_CoAP_response_set_confirmable";

    get type() @ "KPR_CoAP_response_get_type";

    get messageId() @ "KPR_CoAP_response_get_messageId";

    get code() @ "KPR_CoAP_response_get_code";

    setCode(cls, detail) @ "KPR_CoAP_response_setCode";

    get token() @ "KPR_CoAP_response_get_token";

    get payload() @ "KPR_CoAP_response_get_payload";
    set payload(val) @ "KPR_CoAP_response_set_payload";

    get contentFormat() @ "KPR_CoAP_response_get_contentFormat";

    setPayload(payload, contentFormat) @ "KPR_CoAP_response_setPayload";

    get options() @ "KPR_CoAP_response_get_options";
    addOption(option, value) @ "KPR_CoAP_response_addOption";

    get observe() @ "KPR_CoAP_response_is_observe";

    get uri() @ "KPR_CoAP_response_get_uri";
    get url() @ "KPR_CoAP_response_get_uri";
    get host() @ "KPR_CoAP_response_get_host";
    get port() @ "KPR_CoAP_response_get_port";
    get path() @ "KPR_CoAP_response_get_path";
    get query() @ "KPR_CoAP_response_get_query";
};

]]></program></package>
