<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<makefile>

<include name="/makefiles/xsFskDefaults.mk"/>
<input name="../../sources/"/>

<wrap name="FskMediaReaderExtension.c"/>
<wrap name="FskMediaReader.c"/>
<wrap name="FskMediaReaderRTSP.c"/>
<wrap name="RTCPPacketParser.c"/>
<wrap name="RTPPacketBuffer.c"/>
<wrap name="RTPPacketParser.c"/>
<wrap name="RTPPacketParserAAC.c"/>
<wrap name="RTPPacketParserAMR.c"/>
<wrap name="RTPPacketParserAVC.c"/>
<wrap name="RTPPacketParserH263.c"/>
<wrap name="RTPPacketParserLATM.c"/>
<wrap name="RTPPacketParserLPCM.c"/>
<wrap name="RTPPacketParserMP4V.c"/>
<wrap name="RTPPacketParserNULL.c"/>
<wrap name="RTPPacketParserQCELP.c"/>
<wrap name="SDP.c"/>
<wrap name="RTSPSession.c"/>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>