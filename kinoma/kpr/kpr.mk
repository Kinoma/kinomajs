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

<input name="sources"/>
<input name="$(F_HOME)/core/base"/>
<input name="$(F_HOME)/core/graphics"/>
<input name="$(F_HOME)/kinoma/mediareader/sources"/>
<input name="$(F_HOME)/kinoma/qtvrpano/cubic"/>
<input name="$(F_HOME)/extensions/http/sources/"/>
<input name="$(F_HOME)/kinoma/kpr/extensions/templates/"/>
<input name="$(F_HOME)/kinoma/kpr/extensions/upnp/"/>

<wrap name="FskCanvas.c"/>
<wrap name="FskAAPolygon.c"/>
<wrap name="FskAATable.c"/>
<wrap name="FskAnimation.c"/>
<wrap name="FskClipLine2D.c"/>
<wrap name="FskClipPolygon2D.c"/>
<wrap name="FskFont.c"/>
<wrap name="FskGlyphPath.c"/>
<wrap name="FskGradientSpan.c"/>
<wrap name="FskGrowableStorage.c"/>
<wrap name="FskLine.c"/>
<wrap name="FskPath.c"/>
<wrap name="FskPick.c"/>
<wrap name="FskPixelOps.c"/>
<wrap name="FskPerspective.c"/>
<wrap name="FskPolygon.c"/>
<wrap name="FskPremultipliedAlpha.c"/>
<wrap name="FskSpan.c"/>
<wrap name="FskShadow.c"/>
<wrap name="FskTextureSpan.c"/>
<wrap name="FskWideLineToPolygon.c"/>

<wrap name="FskHTTPServer.c"/>

<header name="kpr.h"/>

<wrap name="kprApplication.c"/>
<wrap name="kprAuthentication.c"/>
<wrap name="kprBehavior.c"/>
<wrap name="kprContent.c"/>
<wrap name="kprEffect.c"/>
<wrap name="kprHandler.c"/>
<wrap name="kprHTTPCache.c"/>
<wrap name="kprHTTPClient.c"/>
<wrap name="kprHTTPCookies.c"/>
<wrap name="kprHTTPKeychain.c"/>
<wrap name="kprHTTPServer.c"/>
<wrap name="kprImage.c"/>
<wrap name="kprLabel.c"/>
<wrap name="kprLayer.c"/>
<wrap name="kprMedia.c"/>
<wrap name="kprMessage.c"/>
<wrap name="kprPort.c"/>
<wrap name="kprScroller.c"/>
<wrap name="kprShell.c"/>
<wrap name="kprSkin.c"/>
<wrap name="kprSound.c"/>
<wrap name="kprStyle.c"/>
<wrap name="kprTable.c"/>
<wrap name="kprText.c"/>
<wrap name="kprTransition.c"/>
<wrap name="kprURL.c"/>
<wrap name="kprUtilities.c"/>

<platform name="MacOSX">
<common>
XSC_OPTIONS += -xsID
C_OPTIONS += -DCLOSED_SSL=1 -DSUPPORT_DLLEXPORT=1
LIBRARIES += -framework SystemConfiguration -framework Cocoa -framework WebKit -framework QuartzCore
</common>
<debug>
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS)
UNIVERSAL_FLAGS = -arch i386
</debug>
</platform>

<platform name="Windows">
<common>
XSC_OPTIONS = $(XSC_OPTIONS) -xsID
</common>
</platform>

<platform name="iPhone">
<wrap name="kpr_iOS.m"/>
<common>
XSC_OPTIONS += -xsID
</common>
</platform>

<platform name="threadx">
<common>
XSC_OPTIONS += -xsID
C_OPTIONS += -I/usr/include/freetype2 -I$(F_HOME)/libraries/expat
</common>
</platform>

<platform name="linux">
<common>
XSC_OPTIONS += -xsID
C_OPTIONS += -I$(F_HOME)/libraries/expat
</common>
</platform>

<platform name="android">
<wrap name="kprJNI.c"/>
<common>
XSC_OPTIONS += -xsID
C_OPTIONS += -I$(F_HOME)/libraries/expat
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>
