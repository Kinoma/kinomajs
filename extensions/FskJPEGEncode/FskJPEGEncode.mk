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

<input name="$(F_HOME)/libraries/libjpeg"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>

<wrap name="FskJPEGEncodeExtension.c"/>
<wrap name="FskJPEGEncode.c"/>

<platform name="android">
<!-- JPEG library -->
<common>
C_OPTIONS += -DNO_GETENV \
            -DKINOMA_YUV2RGB565_IFAST_ARM_SUPPORTED	\
            -DKINOMA_IDCT_IFAST_SUPPORTED \
			-DKINOMA_DCT_IFAST_SUPPORTED

OBJECTS += $(TMP_DIR)/jidctfst2-arm.gas.o
OBJECTS += $(TMP_DIR)/jidctfst2-arm-v6.gas.o
OBJECTS += $(TMP_DIR)/jfdctfst-arm-v7.gas7.o

$(TMP_DIR)/jidctfst2-arm.gas.o: $(F_HOME)/libraries/libjpeg/jidctfst2-arm.gas
	$(deviceAS) $(F_HOME)/libraries/libjpeg/jidctfst2-arm.gas -o $(TMP_DIR)/jidctfst2-arm.gas.o

$(TMP_DIR)/jidctfst2-arm-v6.gas.o: $(F_HOME)/libraries/libjpeg/jidctfst2-arm-v6.gas
	$(deviceAS) $(F_HOME)/libraries/libjpeg/jidctfst2-arm-v6.gas -o $(TMP_DIR)/jidctfst2-arm-v6.gas.o

$(TMP_DIR)/jfdctfst-arm-v7.gas7.o: $(F_HOME)/libraries/libjpeg/jfdctfst-arm-v7.gas7
	$(deviceAS) $(F_HOME)/libraries/libjpeg/jfdctfst-arm-v7.gas7 -o $(TMP_DIR)/jfdctfst-arm-v7.gas7.o
</common>

<input name="$(F_HOME)/libraries/libjpeg"/>
<wrap name="jcapimin.c"/>
<wrap name="jcapistd.c"/>
<wrap name="jccoefct.c"/>
<wrap name="jccolor.c"/>
<wrap name="jcdctmgr.c"/>
<wrap name="jchuff.c"/>
<wrap name="jcinit.c"/>
<wrap name="jcmainct.c"/>
<wrap name="jcmarker.c"/>
<wrap name="jcmaster.c"/>
<wrap name="jcomapi.c"/>
<wrap name="jcparam.c"/>
<wrap name="jcphuff.c"/>
<wrap name="jcprepct.c"/>
<wrap name="jcsample.c"/>
<wrap name="jctrans.c"/>

<wrap name="jdcolor.c"/>

<wrap name="jerror.c"/>
<wrap name="jfdctflt.c"/>
<wrap name="jfdctfst.c"/>
<wrap name="jfdctfst2.c"/>
<wrap name="jfdctint.c"/>
<wrap name="jidctflt.c"/>
<wrap name="jidctfst.c"/>
<wrap name="jidctfst2.c"/>
<wrap name="jidctint.c"/>
<wrap name="jidctred.c"/>
<wrap name="jquant1.c"/>
<wrap name="jquant2.c"/>
<wrap name="jutils.c"/>
<wrap name="jmemmgr.c"/>
<wrap name="jmemnobs.c"/>
</platform>

<platform name="iphone/device">
<wrap name="../../libraries/libjpeg/jfdctfst-arm-v7.gas7"/>
<common>
C_OPTIONS += \
			-DKINOMA_DCT_IFAST_SUPPORTED
</common>
</platform>

<platform name="MacOSX,iPhone">
<input name="$(F_HOME)/libraries/libjpeg"/>
<wrap name="jcapimin.c"/>
<wrap name="jcapistd.c"/>
<wrap name="jccoefct.c"/>
<wrap name="jccolor.c"/>
<wrap name="jcdctmgr.c"/>
<wrap name="jchuff.c"/>
<wrap name="jcinit.c"/>
<wrap name="jcmainct.c"/>
<wrap name="jcmarker.c"/>
<wrap name="jcmaster.c"/>
<wrap name="jcomapi.c"/>
<wrap name="jcparam.c"/>
<wrap name="jcphuff.c"/>
<wrap name="jcprepct.c"/>
<wrap name="jcsample.c"/>
<wrap name="jctrans.c"/>

<wrap name="jdcolor.c"/>

<wrap name="jerror.c"/>
<wrap name="jfdctflt.c"/>
<wrap name="jfdctfst.c"/>
<wrap name="jfdctfst2.c"/>
<wrap name="jfdctint.c"/>
<wrap name="jidctflt.c"/>
<wrap name="jidctfst.c"/>
<wrap name="jidctfst2.c"/>
<wrap name="jidctint.c"/>
<wrap name="jidctred.c"/>
<wrap name="jquant1.c"/>
<wrap name="jquant2.c"/>
<wrap name="jutils.c"/>
<wrap name="jmemmgr.c"/>
<wrap name="jmemnobs.c"/>
</platform>

<platform name="Windows">
<input name="$(F_HOME)/libraries/zlib"/>
<input name="$(F_HOME)/libraries/libjpeg"/>
<wrap name="jcapimin.c"/>
<wrap name="jcapistd.c"/>
<wrap name="jccoefct.c"/>
<wrap name="jccolor.c"/>
<wrap name="jcdctmgr.c"/>
<wrap name="jchuff.c"/>
<wrap name="jcinit.c"/>
<wrap name="jcmainct.c"/>
<wrap name="jcmarker.c"/>
<wrap name="jcmaster.c"/>
<wrap name="jcomapi.c"/>
<wrap name="jcparam.c"/>
<wrap name="jcphuff.c"/>
<wrap name="jcprepct.c"/>
<wrap name="jcsample.c"/>
<wrap name="jctrans.c"/>

<wrap name="jdcolor.c"/>

<wrap name="jerror.c"/>
<wrap name="jfdctflt.c"/>
<wrap name="jfdctfst.c"/>
<wrap name="jfdctfst2.c"/>
<wrap name="jfdctint.c"/>
<wrap name="jidctflt.c"/>
<wrap name="jidctfst.c"/>
<wrap name="jidctfst2.c"/>
<wrap name="jidctint.c"/>
<wrap name="jidctred.c"/>
<wrap name="jquant1.c"/>
<wrap name="jquant2.c"/>
<wrap name="jutils.c"/>
<wrap name="jmemmgr.c"/>
<wrap name="fskjmemnobs.c"/>
</platform>

<platform name="Linux">
<input name="$(F_HOME)/libraries/libjpeg"/>
<wrap name="jcapimin.c"/>
<wrap name="jcapistd.c"/>
<wrap name="jccoefct.c"/>
<wrap name="jccolor.c"/>
<wrap name="jcdctmgr.c"/>
<wrap name="jchuff.c"/>
<wrap name="jcinit.c"/>
<wrap name="jcmainct.c"/>
<wrap name="jcmarker.c"/>
<wrap name="jcmaster.c"/>
<wrap name="jcomapi.c"/>
<wrap name="jcparam.c"/>
<wrap name="jcphuff.c"/>
<wrap name="jcprepct.c"/>
<wrap name="jcsample.c"/>
<wrap name="jctrans.c"/>

<wrap name="jdcolor.c"/>

<wrap name="jerror.c"/>
<wrap name="jfdctflt.c"/>
<wrap name="jfdctfst.c"/>
<wrap name="jfdctfst2.c"/>
<wrap name="jfdctint.c"/>
<wrap name="jidctflt.c"/>
<wrap name="jidctfst.c"/>
<wrap name="jidctfst2.c"/>
<wrap name="jidctint.c"/>
<wrap name="jidctred.c"/>
<wrap name="jquant1.c"/>
<wrap name="jquant2.c"/>
<wrap name="jutils.c"/>
<wrap name="jmemmgr.c"/>
<wrap name="jmemnobs.c"/>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>
