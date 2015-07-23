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
<makefile>

<input name="$(F_HOME)/libraries/expat"/>
<input name="$(F_HOME)/extensions/http/sources/"/>
<input name="$(F_HOME)/kinoma/kpr/"/>
<input name="$(F_HOME)/kinoma/kpr/sources/"/>
<input name="$(F_HOME)/kinoma/mediatranscoder/sources/"/>
<input name="$(F_HOME)/extensions/crypt/sources/"/>
<header name="kpr.h"/>

<source name="kprCoAP.c"/>
<source name="kprCoAPClient.c"/>
<source name="kprCoAPClientResolver.c"/>
<source name="kprCoAPClientRequest.c"/>
<source name="kprCoAPServer.c"/>
<source name="kprCoAPServerSession.c"/>
<source name="kprCoAPReceiver.c"/>
<source name="kprCoAPEndpoint.c"/>
<source name="kprCoAPMessage.c"/>
<source name="kprCoAPCommon.c"/>

</makefile>
