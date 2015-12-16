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
	<input name="$(F_HOME)/xs6/patches"/>
	<input name="$(F_HOME)/core/base"/>
	<input name="$(F_HOME)/core/grammars"/>
	<input name="$(F_HOME)/core/graphics"/>
	<input name="$(F_HOME)/core/managers"/>
	<input name="$(F_HOME)/core/misc"/>
	<input name="$(F_HOME)/core/network"/>
	<input name="$(F_HOME)/core/scripting"/>
	<input name="$(F_HOME)/core/ui"/>
	<input name="$(F_HOME)/libraries/expat"/>
	<input name="$(F_HOME)/libraries/QTReader"/>
	<input name="$(F_HOME)/libraries/zlib"/>
	<input name="$(F_HOME)/libraries/zlib/arm"/>
	<input name="$(F_HOME)/kinoma/kpr"/>
	<input name="$(F_HOME)/kinoma/kpr/sources"/>
	<input name="$(F_HOME)/kinoma/qtvrpano/cubic"/>
	<input name="$(F_HOME)/xs6/extensions/chunk"/>
	<input name="$(F_HOME)/xs6/extensions/grammar"/>
	<input name="$(F_HOME)/xs6/extensions/grammar/sources"/>
	<input name="$(F_HOME)/xs6/includes"/>
	<input name="$(F_HOME)/xs6/sources"/>
	<input name="$(F_HOME)/xs6/sources/fsk"/>

	<source name="mainExtensions.c"/>

	<source name="FskAssociativeArray.c"/>
	<source name="FskAudio.c"/>
	<source name="FskAudioIn.c"/>
	<source name="FskBitmap.c"/>
	<source name="FskBlit.c"/>
	<source name="FskConsole.c"/>
	<source name="FskEffects.c"/>
	<source name="FskEndian.c"/>
	<source name="FskEnvironment.c"/>
	<source name="FskEvent.c"/>
	<source name="FskExtensions.c"/>
	<source name="FskFiles.c"/>
	<source name="FskFixedMath.c"/>
	<source name="FskFrameBuffer.c"/>
	<source name="FskInstrumentation.c"/>
	<source name="FskMain.c"/>
	<source name="FskMedia.c"/>
	<source name="FskMemory.c"/>
	<source name="FskMemoryAllocator.c"/>
	<source name="FskList.c"/>
	<source name="FskPort.c"/>
	<source name="FskPortEffects.c"/>
	<source name="FskRectangle.c"/>
	<source name="FskString.c"/>
	<source name="FskSynchronization.c"/>
	<source name="FskThread.c"/>
	<source name="FskTime.c"/>
	<source name="FskUtilities.c"/>
	<source name="FskUUID.c"/>

	<header name="FskArch.h"/>
	<header name="FskAssociativeArray.h"/>
	<header name="FskAudio.h"/>
	<header name="FskBitmap.h"/>
	<header name="FskBlit.h"/>
	<header name="FskConsole.h"/>
	<header name="FskEndian.h"/>
	<header name="FskEnvironment.h"/>
	<header name="FskErrors.h"/>
	<header name="FskEvent.h"/>
	<header name="FskExtensions.h"/>
	<header name="FskFiles.h"/>
	<header name="FskFixedMath.h"/>
	<header name="FskFrameBuffer.h"/>
	<header name="FskFS.h"/>
	<header name="FskGraphics.h"/>
	<header name="FskInstrumentation.h"/>
	<header name="Fsk.h"/>
	<header name="FskList.h"/>
	<header name="FskMain.h"/>
	<header name="FskMedia.h"/>
	<header name="FskMemory.h"/>
	<header name="FskMemoryAllocator.h"/>
	<header name="FskPlatformImplementation.h"/>
	<header name="FskPort.h"/>
	<header name="FskRectangle.h"/>
	<header name="FskString.h"/>
	<header name="FskSynchronization.h"/>
	<header name="FskThread.h"/>
	<header name="FskTime.h"/>
	<header name="FskUtilities.h"/>
	<header name="FskUUID.h"/>

	<source name="FskHeaders.c"/>
	<source name="FskHTTPAuth.c"/>
	<source name="FskHTTPClient.c"/>
	<source name="FskNetInterface.c"/>
	<source name="FskNetUtils.c"/>
	<source name="FskResolver.c"/>
	<source name="FskSSL.c"/>

	<header name="FskHeaders.h"/>
	<header name="FskHTTPAuth.h"/>
	<header name="FskHTTPClient.h"/>
	<header name="FskNetUtils.h"/>
	<header name="FskNetInterface.h"/>
	<header name="FskResolver.h"/>
	<header name="FskSSL.h"/>
	<header name="HTTP.h"/>
	<header name="md5.h"/>

	<source name="FskAAScaleBitmap.c"/>
	<source name="FskBlur.c"/>
	<source name="FskButtonShade.c"/>
	<source name="FskCompositeRect.c"/>
	<source name="FskDHClipPolygon.c"/>
	<source name="FskDMatrix.c"/>
	<source name="FskDilateErode.c"/>
	<source name="FskEdgeEnhancedText.c"/>
	<source name="FskGradientSpan.c"/>
	<source name="FskPerspective.c"/>
	<source name="FskPixelOps.c"/>
	<source name="FskPremultipliedAlpha.c"/>
	<source name="FskProjectImage.c"/>
	<source name="FskRectBlit.c"/>
	<source name="FskRotate90.c"/>
	<source name="FskSMatrix.c"/>
	<source name="FskSpan.c"/>
	<source name="FskTransferAlphaBitmap.c"/>
	<source name="FskVideoSprite.c"/>
	<source name="FskYUV420Copy.c"/>
	<source name="FskYUV422toYUV420.c"/>

	<header name="FskAAScaleBitmap.h"/>
	<header name="FskBlur.h"/>
	<header name="FskButtonShade.h"/>
	<header name="FskCompositeRect.h"/>
	<header name="FskDHClipPolygon.h"/>
	<header name="FskDilateErode.h"/>
	<header name="FskEdgeEnhancedText.h"/>
	<header name="FskGLBlit.h"/>
	<header name="FskMatrix.h"/>
	<header name="FskPerspective.h"/>
	<header name="FskPixelOps.h"/>
	<header name="FskPremultipliedAlpha.h"/>
	<header name="FskProjectImage.h"/>
	<header name="FskRectBlit.h"/>
	<header name="FskRectBlitPatch.h"/>
	<header name="FskRotate90.h"/>
	<header name="FskSpan.h"/>
	<header name="FskTransferAlphaBitmap.h"/>
	<header name="FskVideoSprite.h"/>
	<header name="FskYUV420Copy.h"/>
	<header name="FskYUV422toYUV420.h"/>

	<source name="FskAudioCodec.c"/>
	<source name="FskAudioFilter.c"/>
	<source name="FskImage.c"/>
	<source name="FskMediaPlayer.c"/>
	<source name="FskMuxer.c"/>

	<header name="FskAudioFilter.h"/>
	<header name="FskImage.h"/>
	<header name="FskMediaPlayer.h"/>
	<header name="FskMuxer.h"/>

	<source name="FskCursor.c"/>
	<source name="FskText.c"/>
	<source name="FskDragDrop.c"/>
	<source name="FskTextConvert.c"/>
	<source name="FskWindow.c"/>

	<header name="FskCursor.h"/>
	<header name="FskText.h"/>
	<header name="FskDragDrop.h"/>
	<header name="FskTextConvert.h"/>
	<header name="FskWindow.h"/>

	<source name="xmlparse.c"/>
	<source name="xmlrole.c"/>
	<source name="xmltok.c"/>

	<header name="expat.h"/>
	<header name="expat_external.h"/>
	<header name="fskconfig.h"/>
	<header name="nametab.h"/>
	<header name="xmlrole.h"/>
	<header name="xmltok.h"/>

	<source name="FskECMAScript.c"/>
	<source name="FskECMAScriptIO.c"/>

	<header name="FskECMAScript.h"/>

	<source name="FskDIDLGenMedia.c"/>

	<header name="FskDIDLGenMedia.h"/>

	<source name="xs_dtoa.c"/>
	<source name="xs_pcre.c"/>
	<source name="xs6All.c"/>
	<source name="xs6API.c"/>
	<source name="xs6Array.c"/>
	<source name="xs6Boolean.c"/>
	<source name="xs6Code.c"/>
	<source name="xs6Common.c"/>
	<source name="xs6DataView.c"/>
	<source name="xs6Date.c"/>
	<source name="xs6Debug.c"/>
	<source name="xs6Error.c"/>
	<source name="xs6Function.c"/>
	<source name="xs6Generator.c"/>
	<source name="xs6Global.c"/>
	<source name="xs6Host.c"/>
	<source name="xs6JSON.c"/>
	<source name="xs6Lexical.c"/>
	<source name="xs6MapSet.c"/>
	<source name="xs6Marshall.c"/>
	<source name="xs6Math.c"/>
	<source name="xs6Memory.c"/>
	<source name="xs6Module.c"/>
	<source name="xs6Number.c"/>
	<source name="xs6Object.c"/>
	<source name="xs6Platform.c"/>
	<source name="xs6Profile.c"/>
	<source name="xs6Promise.c"/>
	<source name="xs6Property.c"/>
	<source name="xs6Proxy.c"/>
	<source name="xs6RegExp.c"/>
	<source name="xs6Run.c"/>
	<source name="xs6Scope.c"/>
	<source name="xs6Script.c"/>
	<source name="xs6SourceMap.c"/>
	<source name="xs6String.c"/>
	<source name="xs6Symbol.c"/>
	<source name="xs6Syntaxical.c"/>
	<source name="xs6Tree.c"/>
	<source name="xs6Type.c"/>

	<header name="xs.h"/>
	<header name="xs6Platform.h"/>
	<header name="xs6Common.h"/>
	<header name="xs6All.h"/>
	<header name="xs6Script.h"/>

	<source name="QTReader.c"/>

	<header name="QTMoviesFormat.h"/>
	<header name="QTReader.h"/>

	<source name="adler32.c"/>
	<source name="compress.c"/>
	<source name="crc32.c"/>
	<source name="deflate.c"/>
	<source name="infback.c"/>
	<source name="inffast.c"/>
	<source name="inflate.c"/>
	<source name="inftrees.c"/>
	<source name="trees.c"/>
	<source name="uncompr.c"/>
	<source name="zutil.c"/>

	<header name="zlib.h"/>
	<header name="zconf.h"/>

	<platform name="android">
		<source name="main.c"/>
		<source name="FskFilesAndroid.c"/>
		<source name="FskGLBlit.c"/>
		<source name="FskTextFreeType.c"/>
		<source name="FskGLContext.c"/>
		<source name="FskGLEffects.c"/>

		<source name="yuv420torgb-arm-v4-v5.gas"/>
		<source name="yuv420torgb-arm-v6.gas"/>
		<source name="yuv420torgb565le-arm.gas"/>
		<source name="yuv420torgb565le-arm.gas7"/>

		<source name="FskYUV420Copy-arm.gas"/>
		<source name="FskYUV420iCopy-arm.gas"/>
		<source name="FskYUV420iCopy-arm-v6.gas"/>
		<source name="FskFixedMath-arm.gas"/>
		<source name="FskBlit-arm.gas7"/>
		<source name="FskBlit-arm.wmmx"/>
		<source name="yuv420torgb-arm.wmmx"/>

		<source name="FskTransferAlphaBlit-arm.wmmx"/>
		<source name="FskTransferAlphaBlit-arm.gas7"/>
		<source name="AAScaleBitmap-arm.gas7"/>
		<source name="HVfilter-arm.gas7"/>
		<source name="inflate_fast_copy_neon.gas"/>
		<source name="adler32_neon.gas"/>

		<source name="cpu-features.c"/>
		<source name="FskHardware.c"/>

		<input name="$(F_HOME)/libraries/resolv"/>

		<c option="-D__BIND_NOSTATIC"/>
		<source name="h_errno.c"/>
		<source name="ns_name.c"/>
		<source name="ns_netint.c"/>
		<source name="ns_samedomain.c"/>
		<source name="res_comp.c"/>
		<source name="res_data.c"/>
		<source name="res_mkquery.c"/>
		<source name="res_query.c"/>
		<source name="res_state.c"/>

		<input name="$(F_HOME)/libraries/libjpeg"/>

		<source name="jidctfst2-arm.gas"/>
		<source name="jidctfst2-arm-v6.gas"/>
		<source name="jidctfst2-arm-v7.gas7"/>

		<input name="$(F_HOME)/extensions/FskPNGDecode/sources"/>

		<source name="png_arm_v7.gas7"/>
		<!-- "asm option" will act like "c option" but is used for cmake specifically for gas/gas7 files -->
		<asm option="-mfpu=neon"/>
	</platform>

	<platform name="iphone">
		<c option="-std=gnu99"/>

		<source name="FskFilesMac.c"/>
		<source name="FskGLBlit.c"/>
		<source name="FskTextMac.c"/>
		<source name="FskGLContext.c"/>
		<source name="FskGLEffects.c"/>

		<source name="FskCocoaMainPhone.m"/>
		<source name="FskCocoaSupportCommon.m"/>
		<source name="FskCocoaSupportPhone.m"/>
		<source name="FskCocoaApplicationPhone.m"/>
		<source name="FskCocoaViewControllerPhone.m"/>
		<source name="FskCocoaViewPhone.m"/>

		<source name="FskFilesLinux.c"/>

		<source name="FskYUV420ToYUV422.c"/>
		<header name="FskYUV420ToYUV422.h"/>
		<source name="FskFixedMath-arm.gas"/>
		
		<library name="-framework AddressBook"/>
		<library name="-framework AssetsLibrary"/>
		<library name="-framework AudioToolbox"/>
		<library name="-framework AVFoundation"/>
		<library name="-framework CoreAudio"/>
		<library name="-framework CoreData"/>
		<library name="-framework CoreGraphics"/>
		<library name="-framework CoreLocation"/>
		<library name="-framework CoreMedia"/>
		<library name="-framework CoreTelephony"/>
		<library name="-framework CoreText"/>
		<library name="-framework CoreVideo"/>
		<library name="-framework Foundation"/>
		<library name="-framework MediaPlayer"/>
		<library name="-framework OpenGLES"/>
		<library name="-framework QuartzCore"/>
		<library name="-framework SystemConfiguration"/>
		<library name="-framework UIKit"/>
		<library name="-lresolv"/>
		<library name="-lstdc++"/>

	</platform>

	<platform name="linux/t7">
		<source name="FskGLContext.c"/>
	</platform>

	<platform name="linux">
 		<source name="main.c"/>
		<input name="$(F_HOME)/core/kpl"/>
 		<input name="$(F_HOME)/build/linux/kpl"/>

	 	<source name="FskAudioKpl.c"/>
		<source name="FskEventKpl.c"/>
		<source name="FskFilesKpl.c"/>
		<source name="FskHardware.c"/>
		<source name="FskTextFreeType.c"/>

 		<header name="KplAudio.h"/>
    	<header name="KplBitmap.h"/>
    	<header name="KplCommon.h"/>
    	<header name="KplDevice.h"/>
    	<header name="KplECMAScript.h"/>
    	<header name="KplEnvironment.h"/>
	<!--header name="KplErrors.h"/-->
    	<header name="KplExtensions.h"/>
    	<header name="KplFiles.h"/>
    	<header name="KplGL.h"/>
    	<header name="KplMain.h"/>
    	<header name="KplMemory.h"/>
    	<header name="KplNet.h"/>
		<header name="KplNetInterface.h"/>
    	<header name="KplProperty.h"/>
    	<header name="KplRectangle.h"/>
    	<header name="KplScreen.h"/>
    	<header name="KplSocket.h"/>
    	<header name="KplSynchronization.h"/>
    	<header name="KplThread.h"/>
    	<header name="KplTime.h"/>
    	<header name="KplUIEvents.h"/>
    	<header name="KplUtilities.h"/>

    	<header name="Kpl.h"/>
    	<header name="FskPlatformImplementation.Kpl.h"/>
    	<header name="KplThreadLinuxPriv.h"/>
    	<header name="KplTimeCallbackLinuxPriv.h"/>
    	<header name="xs_kpl.h"/>

		<source name="KplDeviceLinux.c"/>
		<source name="KplECMAScriptLinux.c"/>
		<source name="KplEnvironmentLinux.c"/>
		<source name="KplExtensionsLinux.c"/>
		<source name="KplFilesLinux.c"/>
		<source name="KplMainLinux.c"/>
		<source name="KplMemoryLinux.c"/>
		<source name="KplNetInterfaceLinux.c"/>
		<source name="KplNetLinux.c"/>
		<source name="KplSocketLinux.c"/>
		<source name="KplSynchronizationLinux.c"/>
		<source name="KplThreadLinux.c"/>
		<source name="KplTimeCallbackLinux.c"/>
		<source name="KplUIEvents.c"/>
		<source name="KplUtilitiesLinux.c"/>
		<!-- defined in FskPlatform.mk -->
		<!--source name="KplAudioLinux.c"/>
		<source name="KplScreenLinux.c"/-->

		<c option="-D_GNU_SOURCE"/>

		<source name="FskYUV420ToYUV422.c"/>
		<header name="FskYUV420ToYUV422.h"/>

	</platform>

	<platform name="threadx">
 		<source name="main.c"/>
		<input name="$(F_HOME)/core/kpl"/>
 		<input name="$(F_HOME)/build/unity/kpl"/>

	 	<source name="FskAudioKpl.c"/>
		<source name="FskEventKpl.c"/>
		<source name="FskFilesKpl.c"/>
		<source name="FskHardware.c"/>
		<source name="FskTextFreeType.c"/>

 		<header name="KplAudio.h"/>
    	<header name="KplBitmap.h"/>
    	<header name="KplCommon.h"/>
    	<header name="KplDevice.h"/>
    	<header name="KplECMAScript.h"/>
    	<header name="KplEnvironment.h"/>
	<!--header name="KplErrors.h"/-->
    	<header name="KplExtensions.h"/>
    	<header name="KplFiles.h"/>
    	<header name="KplGL.h"/>
    	<header name="KplMain.h"/>
    	<header name="KplMemory.h"/>
    	<header name="KplNet.h"/>
		<header name="KplNetInterface.h"/>
    	<header name="KplProperty.h"/>
    	<header name="KplRectangle.h"/>
    	<header name="KplScreen.h"/>
    	<header name="KplSocket.h"/>
    	<header name="KplSynchronization.h"/>
    	<header name="KplThread.h"/>
    	<header name="KplTime.h"/>
    	<header name="KplUIEvents.h"/>
    	<header name="KplUtilities.h"/>

    	<header name="Kpl.h"/>
    	<header name="FskPlatformImplementation.Kpl.h"/>
    	<header name="KplThreadLinuxPriv.h"/>
    	<header name="KplTimeCallbackLinuxPriv.h"/>
    	<header name="xs_kpl.h"/>

		<source name="KplDeviceLinux.c"/>
		<source name="KplECMAScriptLinux.c"/>
		<source name="KplEnvironmentLinux.c"/>
		<source name="KplExtensionsLinux.c"/>
		<source name="KplFilesLinux.c"/>
		<source name="KplMainLinux.c"/>
		<source name="KplMemoryLinux.c"/>
		<source name="KplNetInterfaceLinux.c"/>
		<source name="KplNetLinux.c"/>
		<source name="KplSocketLinux.c"/>
		<source name="KplSynchronizationLinux.c"/>
		<source name="KplThreadLinux.c"/>
		<source name="KplTimeCallbackLinux.c"/>
		<source name="KplUIEvents.c"/>
		<source name="KplUtilitiesLinux.c"/>
		<!-- defined in FskPlatform.mk -->
		<!--source name="KplAudioLinux.c"/>
		<source name="KplScreenLinux.c"/-->

		<c option="-D_GNU_SOURCE"/>

		<source name="FskYUV420ToYUV422.c"/>
		<header name="FskYUV420ToYUV422.h"/>

	</platform>

	<platform name="mac">
		<source name="main.c"/>
		<source name="FskFilesMac.c"/>
		<source name="FskGLBlit.c"/>
		<source name="FskTextMac.c"/>
		<source name="FskDragDropMac.c"/>
		<source name="FskGLContext.c"/>
		<source name="FskGLEffects.c"/>

		<source name="FskCocoaMain.m"/>
		<source name="FskCocoaSupport.m"/>
		<source name="FskCocoaSupportCommon.m"/>
		<source name="FskCocoaApplication.m"/>
		<source name="FskCocoaWindow.m"/>
		<source name="FskCocoaView.m"/>
		<source name="FskFilesLinux.c"/>

		<header name="FskCocoaSupport.h"/>
		<header name="FskCocoaSupportCommon.h"/>
		<header name="FskCocoaApplication.h"/>
		<header name="FskCocoaWindow.h"/>
		<header name="FskCocoaView.h"/>

		<library name="-framework CoreFoundation"/>
		<library name="-framework CoreServices"/>
		<library name="-framework QuickTime"/>
		<library name="-framework SystemConfiguration"/>
		<library name="-framework IOKit"/>
		<library name="-framework OpenGL"/>
		<library name="-framework Cocoa"/>
		<library name="-framework AudioUnit"/>
		<library name="-framework AudioToolbox"/>
		<library name="-framework CoreAudio"/>
		<library name="-framework CoreWLAN"/>
		<library name="-framework CoreMedia"/>
		<library name="-framework AVFoundation"/>
		<library name="-lresolv"/>
		<library name="-lstdc++"/>

		<source name="FskYUV420ToYUV422.c"/>
		<header name="FskYUV420ToYUV422.h"/>

	</platform>

	<platform name="win">
		<source name="main.c"/>
		<source name="FskFilesWin.c"/>
		<source name="FskTimeWin.c"/>
		<source name="FskDragDropWin.cpp"/>
		<source name="FskMenu.c"/>
		<source name="FskTextWin.c"/>

		<library name="advapi32.lib"/>
		<library name="comctl32.lib"/>
		<library name="comdlg32.lib"/>
		<library name="gdi32.lib"/>
		<library name="imm32.lib"/>
		<library name="iphlpapi.lib"/>
		<library name="kernel32.lib"/>
		<library name="Msacm32.lib"/>
		<library name="odbc32.lib"/>
		<library name="odbccp32.lib"/>
		<library name="ole32.lib"/>
		<library name="oleaut32.lib"/>
		<library name="Secur32.lib"/>
		<library name="shell32.lib"/>
		<library name="user32.lib"/>
		<library name="uuid.lib"/>
		<library name="Wininet.lib"/>
		<library name="Winmm.lib"/>
		<library name="winspool.lib"/>
		<library name="Ws2_32.lib"/>
		<library name="Dnsapi.lib"/>
		<library name="$(F_HOME)\libraries\DirectX\dxguid.lib"/>
		<library name="$(F_HOME)\libraries\DirectX\dxerr8.lib"/>
		<library name="$(F_HOME)\libraries\DirectX\dinput8.lib"/>
		<library name="$(F_HOME)\libraries\DirectX\dsound.lib"/>

		<source name="FskYUV420ToYUV422.c"/>
		<header name="FskYUV420ToYUV422.h"/>

	</platform>
</makefile>
