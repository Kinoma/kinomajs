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
<include name="/makefiles/xsFskDefaults.mk"/>

<wrap name="primitives_ltc.c"/>

<input name="sources/libs/libtomcrypt-1.13/src/headers"/>
<input name="sources/libs/libtomcrypt-1.13/src/ciphers"/>
<input name="sources/libs/libtomcrypt-1.13/src/ciphers/aes"/>
<input name="sources/libs/libtomcrypt-1.13/src/hashes"/>
<!--wrap name="md5_ltc.c"/-->	<!-- md5 is in core -->
<wrap name="sha1.c"/>

<platform name="MacOSX,Solaris,iPhone,Windows,mac-cmake">
<wrap name="aes.c"/>	<!-- aes in kps -->
</platform>

<platform name="android,Linux,MacOSX,Solaris,iPhone,mac-cmake">
<debug>
XSC_OPTIONS += -t debug
</debug>
<common>
C_OPTIONS += -DARGTYPE=4 -DLTM_DESC -DLTC_NO_ASM -DLTC_NO_PK -DLTC_NO_PKCS -DLTC_NO_TEST -DFSK_NO_SHA256 -DFSK_NO_SHA512 -DFSK_NO_DES -DFSK_NO_TDES -DFSK_NO_RC4
</common>
</platform>

<platform name="Windows">
<debug>
XSC_OPTIONS = $(XSC_OPTIONS) -t debug
</debug>
<common>
C_OPTIONS = $(C_OPTIONS) -DARGTYPE=4 -DLTM_DESC -DLTC_NO_ASM -DLTC_NO_PK -DLTC_NO_PKCS -DLTC_NO_TEST -DFSK_NO_SHA256 -DFSK_NO_SHA512 -DFSK_NO_DES -DFSK_NO_TDES -DFSK_NO_RC4
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

<platform name="android,Linux,MacOSX,iPhone,Solaris,Windows,mac-cmake">
<common>
$(OBJECTS): sources/common.h
</common>
</platform>

</makefile>