<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2016 Marvell International Ltd.
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

<input name="$(F_HOME)/kinoma/kpr/"/>
<input name="$(F_HOME)/kinoma/kpr/sources/"/>

<header name="kpr.h"/>
<source name="kprZeroconfAdvertisement.c"/>
<source name="kprZeroconfBrowser.c"/>
<source name="kprZeroconfCommon.c"/>

<platform name="android">
	<input name="$(F_HOME)/libraries/mDNSResponder/"/>
	<input name="$(F_HOME)/libraries/mDNSResponder/mDNSCore/"/>
	<input name="$(F_HOME)/libraries/mDNSResponder/mDNSPosix/"/>
	<input name="$(F_HOME)/libraries/mDNSResponder/mDNSShared/"/>
	
	<source name="kprZeroconfApple.c"/>
	
	<source name="mDNSCore/anonymous.c"/>
	<source name="mDNSCore/CryptoAlg.c"/>
	<source name="mDNSCore/mDNS.c"/>
	<source name="mDNSCore/uDNS.c"/>
	<source name="mDNSCore/DNSCommon.c"/>
	<source name="mDNSCore/DNSDigest.c"/>
	<source name="mDNSPosix/mDNSPosix.c"/>
	<source name="mDNSPosix/mDNSUNP.c"/>
	<source name="mDNSShared/mDNSDebug.c"/>
	<source name="mDNSShared/dnssd_clientlib.c"/>
	<source name="mDNSShared/dnssd_clientshim.c"/>
	<source name="mDNSShared/dnssd_ipc.c"/>
	<source name="mDNSShared/GenLinkedList.c"/>
	<source name="mDNSShared/PlatformCommon.c"/>
	
	<library name="libcutils"/>
	<library name="liblog"/>
	<common>
C_OPTIONS += \
    -D_GNU_SOURCE \
    -DHAVE_IPV4 \
    -DHAVE_LINUX \
    -DNOT_HAVE_SA_LEN \
    -DPLATFORM_NO_RLIMIT \
    -DTARGET_OS_LINUX \
    -DUSES_NETLINK \
    -DMDNS_DEBUGMSGS=0 \
    -DMDNS_UDS_SERVERPATH=\"/dev/socket/mdnsd\" \
    -DMDNS_USERNAME=\"mdnsr\"
	</common>
</platform>
<platform name="iphone">
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="mac">
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="linux/iap140,linux/linkit7688,linux/pine64,linux/bg3cdp,linux/gtk,linux/headless,linux/pi_gl,linux/pi,linux/beaglebone,linux/edison,linux/andro">
	<cmake name="mDNSResponder" build="$(F_HOME)/libraries/mDNSResponder/CMakeLists.txt" />
	<input name="$(F_HOME)/libraries/mDNSResponder/mDNSShared/"/>
	<!--library name="-ldns_sd"/-->
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="linux/aspen,linux/poky">
	<input name="$(F_HOME)/libraries/mDNSResponder/mDNSShared/"/>
	<library name="-ldns_sd"/>
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="win">
	<input name="$(F_HOME)/libraries/BonjourSDK/Include/"/>
	<source name="kprZeroconfApple.c"/>
	<library name="$(F_HOME)/libraries/BonjourSDK\Lib\Win32\dnssd.lib"/>
</platform>
</makefile>
