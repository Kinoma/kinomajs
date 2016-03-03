/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
import * as FS from "fs";
import * as MAKE from "make";
import Android from "shared/android";

class Makefile extends MAKE.Makefile {
	constructor(tree) {
		tree.cIncludes.push("$(NDK_DIR)/platforms/android-$(NDK_PLATFORM_VER)/arch-arm/usr/include");
		tree.cIncludes.push("$(FREETYPE_DIR)/include");
		super(tree);
		this.objects = "$(" + this.name + "_OBJECTS)";
	}
	generateRules(tool, file) {
		for (let item of this.sources)
			this.generateRule(tool, file, item);
	}
};

class Manifest extends MAKE.Manifest {
	constructor(tree) {
		super(tree);
		this.android = new Android(tree);
	}
	get Makefile() {
		return Makefile;
	}
	generate(tool, tmp, bin) {
		this.android.completeInfo(tool);
		super.generate(tool, tmp, bin);
		this.android.generateNdk(tool, tmp, bin);
	}
	getPlatformVariables(tool, tmp, bin) {
		var environment = this.tree.environment;
		var parts = tool.splitPath(tool.manifestPath);
		var info = this.tree.info;
		parts.name = "android";
		parts.extension = "";
		return {
			KPR_APPLICATION: tool.application,
			KPR_NAMESPACE: environment.NAMESPACE,

			KPR2JS: "$(F_HOME)/xs6/bin/mac/debug/kpr2js6",
			XS2JS: "$(F_HOME)/xs6/bin/mac/debug/xs2js6",
			XSC: "$(F_HOME)/xs6/bin/mac/debug/xsc6",
			XSL: "$(F_HOME)/xs6/bin/mac/debug/xsl6",

			CC: "arm-linux-androideabi-gcc",
			CXX: "arm-linux-androideabi-g++",
			AS: "arm-linux-androideabi-gcc",
			AS_OPTIONS: "-x assembler-with-cpp -c -march=armv7-a",
			AS_V7: "$(AS)",
			AS_V7_OPTIONS: "$(AS_OPTIONS)",
			AS_NEON: "$(AS)",
			AS_NEON_OPTIONS: "$(AS_OPTIONS) -mfpu=neon",
			AS_WMMX: "$(F_HOME)/tools/wmmx/arm-marvell-eabi-as",
			AS_WMMX_OPTIONS: "-mwmmxt",
			AR: "arm-linux-androideabi-ar cru",
			LINK: "arm-linux-androideabi-g++",
			STRIP: "arm-linux-androideabi-strip",

			APP_DIR: bin,
			RES_DIR: bin,
			TMP_DIR: tmp,

			OSS: "$(F_HOME)/build/android/OSS/",
			OSS2: "$(F_HOME)/build/android/OSS2/",

			FSK_JAVA_NAMESPACE: info.path,
			
			NDK_PLAY_PATH: "$(F_HOME)/build/android/inNDK",
			NDK_PROJECT_PATH: "$(TMP_DIR)/ndk/project",
			NDK_PROJECT_BIN: "$(NDK_PROJECT_PATH)/bin",
			NDK_PROJECT_GEN: "$(NDK_PROJECT_PATH)/gen",
			NDK_PROJECT_LIBRARIES: "$(NDK_PROJECT_PATH)/libs/armeabi",
			NDK_PROJECT_OBJECTS: "$(NDK_PROJECT_PATH)/obj/local/armeabi",

			NDK_PLATFORM_VER: "14",
			NDK_PLATFORM: "platforms/android-$(NDK_PLATFORM_VER)/arch-arm/usr/lib",
			NDK_TOOLCHAIN_VERSION: "4.4.3",

			KEYSTORE: "$(TMP_DIR)/$(KPR_APPLICATION)-release.keystore",
			KEYSTORE_ALIAS: "$(KPR_APPLICATION)",

			FREETYPE_PLATFORM_C_OPTIONS: "--sysroot=$(ANDROID_NDK)/platforms/android-$(NDK_PLATFORM_VER)/arch-arm",
	
			NAME: environment.NAME,
			NAMESPACE: environment.NAMESPACE,
			VERSION: environment.VERSION,
			
			KPR_RESOURCE_PATH: tool.joinPath(parts),
			KPR_MAKE_PATH: tool.makePath,
			
			NDK_PLATFORM_VER: 14,
			NDK_PLATFORM: "platforms/android-$(NDK_PLATFORM_VER)/arch-arm/usr/lib",
			SEPARATE_LIBRARIES: "$(ANDROID_NDK_TOOLCHAIN_BIN)//lib/gcc/arm-linux-androideabi/$(NDK_TOOLCHAIN_VERSION)/libgcc.a -L$(NDK_DIR)/$(NDK_PLATFORM)/ -lc -lstdc++ -lm $(NDK_PROJECT_PATH)/libs/armeabi/libFsk.so -ldl -llog",
			SEPARATE_LINK_OPTIONS: "-nostdlib -Wl,-shared,-Bsymbolic -Wl,--whole-archive -Wl,--fix-cortex-a8 -Wl,-rpath-link=$(NDK_PLATFORM)" + (tool.debug ? "-g" : ""),
			SEPARATE_DIR: "$(NDK_PROJECT_LIBRARIES)",
			NDB_OPTIONS: tool.debug ? "NDK_DEBUG=1" : "",
			KPR_CONFIGURATION: tool.debug ? "debug" : "release",
		}
	}
	getTargetRules(tool) {
		var rules = `

all: $(APP_DIR) $(HEADERS) FREETYPE $(TMP_DIR)/libFsk.so $(SEPARATE) $(NDK_PROJECT_BIN)/$(KPR_APPLICATION)-$(KPR_CONFIGURATION).apk
	@ echo "--------------------------------------------------------------------------------"
	@ echo "-   Install: adb -d install -r $(NDK_PROJECT_BIN)/$(KPR_APPLICATION)-$(KPR_CONFIGURATION).apk"
	@ echo "- Uninstall: adb -d uninstall $(KPR_NAMESPACE)"
	@ echo "-     Start: adb shell am start -n $(KPR_NAMESPACE)/$(KPR_NAMESPACE).KinomaPlay"
	@ echo "-      Stop: adb shell am force-stop $(KPR_NAMESPACE)"
	@ echo "-     Debug: cd $(NDK_PROJECT_PATH) ; export NDK_PROJECT_PATH=$(NDK_PROJECT_PATH) ; $(NDK_DIR)/ndk-gdb --verbose --force --start"
	@ echo "--------------------------------------------------------------------------------"

$(APP_DIR):
	mkdir -p $(APP_DIR)

$(NDK_PROJECT_OBJECTS)/libfsk.a: $(OBJECTS)
	rm -rf $(NDK_PROJECT_BIN)
	rm -rf $(NDK_PROJECT_GEN)
	rm -rf $(NDK_PROJECT_LIBRARIES)
	rm -rf $(NDK_PROJECT_OBJECTS)
	mkdir -p $(NDK_PROJECT_OBJECTS)
	$(AR) $@ $(OBJECTS)

$(TMP_DIR)/libFsk.so: $(NDK_PROJECT_OBJECTS)/libfsk.a
	PATH=$(NDK_DIR):$(PATH) ; export KPR_TMP_DIR=$(TMP_DIR) ; export NDK_PROJECT_PATH=$(NDK_PROJECT_PATH) ; cd $(NDK_PROJECT_PATH)/.. ; ndk-build clean ; ndk-build SUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG) $(NDB_OPTIONS) V=1

##################################################
# apk
##################################################

application: $(FOLDERS) $(FILES)
	mkdir -p $(NDK_PROJECT_PATH)/res/raw
	rm -f $(NDK_PROJECT_PATH)/res/raw/kinoma.jet
	cd $(APP_DIR) ; zip -8qrn .jpg:.png:.m4a $(NDK_PROJECT_PATH)/res/raw/kinoma.jet *
	cd $(NDK_PROJECT_PATH); rm -rf bin ; ant -Dsdk.dir=$(ANDROID_SDK) $(KPR_CONFIGURATION)

$(KEYSTORE):
	keytool -genkey -v -keystore $@ -alias $(KEYSTORE_ALIAS) -keyalg RSA -keysize 2048 -validity 10000

$(NDK_PROJECT_BIN)/$(KPR_APPLICATION)-debug.apk: application

$(NDK_PROJECT_BIN)/$(KPR_APPLICATION)-release.apk: application | $(KEYSTORE)
	rm -f $@
	jarsigner -verbose -sigalg MD5withRSA -digestalg SHA1 -keystore $(KEYSTORE) $(KEYSTORE_PASSWORD) $(NDK_PROJECT_BIN)/$(KPR_APPLICATION)-release-unsigned.apk $(KEYSTORE_ALIAS)
	$(ANDROID_SDK)/bin/zipalign -v 4 $(NDK_PROJECT_BIN)/$(KPR_APPLICATION)-release-unsigned.apk $@

`;
		var source = tool.homePath + "/libraries/freetype/freetype.make";
		var buffer = FS.readFileSync(source).toString();
		
		return rules + "\n" + buffer + "\n";
	}
};

export default Manifest;
