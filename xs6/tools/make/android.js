/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
	}
	get Makefile() {
		return Makefile;
	}
	completeInfo(tool) {
		var environment = this.tree.environment;
		var info = this.tree.info;
		if (!("features" in info))
			info.features = {};
		if (!("location.gps" in info.features))
			info.features["location.gps"] = false;
		if (!("telephony" in info.features))
			info.features["telephony"] = false;
		if (!("permissions" in info))
			info.permissions = [];
		info.permissions.push("ACCESS_NETWORK_STATE");
		info.permissions.push("BLUETOOTH");
		info.permissions.push("BLUETOOTH_ADMIN");
		info.permissions.push("INTERNET");
		info.permissions.push("MODIFY_AUDIO_SETTINGS");
		info.permissions.push("READ_EXTERNAL_STORAGE");
		info.permissions.push("WAKE_LOCK");
		info.permissions.push("WRITE_EXTERNAL_STORAGE");

		if (!("version" in info))
			info.version = {};
		if (!("minimum" in info.version))
			info.version.minimum = 9;
		if (!("target" in info.version))
			info.version.target = 17;
		switch (info.orientation) {
			case "portrait":
				info.orientation = { type: "portrait", option: "" }
			break;
			case "landscape":
				info.orientation = { type: "landscape", option: "" }
			break;
			case "sensorPortrait":
				info.orientation = { type: "sensorPortrait", option: "|orientation" }
			break;
			case "sensorLandscape":
				info.orientation = { type: "sensorLandscape", option: "|orientation" }
			break;
			default:
				info.orientation = { type: "sensor", option: "|orientation" }
			break;
		}
		if (!("modules" in info))
			info.modules = "";
		info.debuggable = tool.debug;
		var namespace = environment.NAMESPACE.split(".");
		info.name = namespace[namespace.length - 1];
		info.base = namespace.join("_");
		info.path = namespace.join("/");
		this.info = info;
	}
	copyDrawableFile(tool, tmp, path) {
		var appPath = tool.manifestPath.split("/");
		appPath[appPath.length - 1] = "android";
		appPath = appPath.join("/");
		var source = appPath + path;
		if (!FS.existsSync(source))
			source = tool.homePath + "/build/android/inNDK/Play/project/res" + path;
		var destination = tmp + "/ndk/project/res" + path;
		FS.copyFileSync(source, destination);
	}
	copyFile(source, destination, mapping) {
		if (mapping) {
			var buffer = FS.readFileSync(source).toString();
			mapping.forEach(function(map) {
				buffer = buffer.replace(map.key, map.value);
			});
			FS.writeFileSync(destination, buffer);
		}
		else
			FS.copyFileSync(source, destination);
	}
	copyJavaFile(tool, tmp, path, name, mapping) {
		var allPermissions = [
			"ACCESS_FINE_LOCATION",
			"ACCESS_NETWORK_STATE",
			"ACCESS_WIFI_STATE",
			"C2D_MESSAGE",
			"CALL_PHONE",
			"READ_CONTACTS",
			"READ_PHONE_STATE",
			"READ_SMS",
			"RECEIVE_SMS",
			"SEND_SMS"
		];
		var source = tool.homePath + "/build/android/inNDK/Play/project/src/com/kinoma/kinomaplay/" + name;
		var destination = tmp + "/ndk" + path + "/" + name;
		var buffer = FS.readFileSync(source).toString();
		mapping.forEach(function(map) {
			buffer = buffer.replace(map.key, map.value);
		});
		var stop = "//#endif";
		for (let permission of allPermissions) {
			var exclude = this.info.permissions.indexOf(permission) == -1;
			var start = "//#ifdefined " + permission;
			var length = start.length;
			var startIndex, stopIndex;
			while ((startIndex = buffer.lastIndexOf(start)) >= 0) {
				if (exclude) {
					stopIndex = buffer.indexOf(stop, startIndex);
					buffer = buffer.substr(0, startIndex - 1) + buffer.substr(stopIndex + 8);
				}
				else {
					buffer = buffer.substr(0, startIndex - 1) + buffer.substr(startIndex + length);
					stopIndex = buffer.indexOf(stop, startIndex);
					buffer = buffer.substr(0, stopIndex - 1) + buffer.substr(stopIndex + 8);
				}
			}
		}
		FS.writeFileSync(destination, buffer);
	}
	copyKinomaFile(tool, tmp, path, mapping) {
		var source = tool.homePath + "/build/android/inNDK/kinoma" + path;
		var destination = tmp + "/ndk/project/jni" + path;
		this.copyFile(source, destination, mapping);
	}
	copyNdkFile(tool, tmp, cmake, path, mapping) {
		var source = (cmake ? tool.makePath + "/ndk" : tool.homePath + "/build/android/inNDK/Play") + path;
		var destination = tmp + "/ndk" + path;
		this.copyFile(source, destination, mapping);
	}
	generate(tool, tmp, bin) {
		this.completeInfo(tool);
		super.generate(tool, tmp, bin);
		this.generateNdk(tool, tmp, bin);
	}
	generateNdk(tool, tmp, bin) {
		var environment = this.tree.environment;
		var info = this.tree.info;

		// create directory structure
		FS.mkdirSync(tmp + "/ndk");
		this.copyNdkFile(tool, tmp, true, "/Application.mk", null);
		
		FS.mkdirSync(tmp + "/ndk/project");
		var features = "";
		for (let feature in info.features)
			features += '\t<uses-feature android:name="' + feature + '" android:required="' + info.features[feature] + '"/>\n';
		var permissions = "";
		for (let i = 0, c = info.permissions.length; i < c; i++)
			permissions += '\t<uses-permission android:name="android.permission.' + info.permissions[i] + '"/>\n';
		this.copyNdkFile(tool, tmp, true, "/project/AndroidManifest.xml", [
			{ "key": "#NAMESPACE#", "value": environment.NAMESPACE },
			{ "key": "#VERSION#", "value": environment.VERSION },
			{ "key": "#VERSION_MINIMUM#", "value": info.version.minimum },
			{ "key": "#VERSION_TARGET#", "value": info.version.target },
			{ "key": "#ANDROID_DEBUGGABLE#", "value": info.debuggable },
			{ "key": "#ORIENTATION_TYPE#", "value": info.orientation.type },
			{ "key": "#ORIENTATION_OPTION#", "value": info.orientation.option },
			{ "key": "#MANIFEST_MODULES#", "value": "" },
			{ "key": "#MANIFEST_PERMISSIONS#", "value": permissions },
			{ "key": "#MANIFEST_FEATURES#", "value": features },
		]);

		this.copyNdkFile(tool, tmp, true, "/project/build.xml", [{ "key": "#KPR_APPLICATION#", "value": info.name }]);
		this.copyNdkFile(tool, tmp, true, "/project/local.properties", null);
		this.copyNdkFile(tool, tmp, false, "/project/proguard.cfg", null);
		this.copyNdkFile(tool, tmp, false, "/project/project.properties", null);

		FS.mkdirSync(tmp + "/ndk/project/res");

		FS.mkdirSync(tmp + "/ndk/project/res/drawable");
		this.copyDrawableFile(tool, tmp, "/drawable/icon.png");
		this.copyDrawableFile(tool, tmp, "/drawable/splashscreen.png");
		this.copyDrawableFile(tool, tmp, "/drawable/web_return_icon.png");
		FS.mkdirSync(tmp + "/ndk/project/res/drawable-hdpi");
		this.copyDrawableFile(tool, tmp, "/drawable-hdpi/ball.png");
		this.copyDrawableFile(tool, tmp, "/drawable-hdpi/icon.png");
		FS.mkdirSync(tmp + "/ndk/project/res/drawable-ldpi");
		this.copyDrawableFile(tool, tmp, "/drawable-hdpi/ball.png");
		this.copyDrawableFile(tool, tmp, "/drawable-ldpi/icon.png");
		FS.mkdirSync(tmp + "/ndk/project/res/drawable-mdpi");
		this.copyDrawableFile(tool, tmp, "/drawable-hdpi/ball.png");
		this.copyDrawableFile(tool, tmp, "/drawable-mdpi/icon.png");
		FS.mkdirSync(tmp + "/ndk/project/res/drawable-xhdpi");
		this.copyDrawableFile(tool, tmp, "/drawable-xhdpi/ball.png");

		FS.mkdirSync(tmp + "/ndk/project/res/layout");
		this.copyNdkFile(tool, tmp, false, "/project/res/layout/main.xml", [ { "key": "com.kinoma.kinomaplay", "value": environment.NAMESPACE } ]);
		this.copyNdkFile(tool, tmp, false, "/project/res/layout/splashscreen.xml", null);
		this.copyNdkFile(tool, tmp, false, "/project/res/layout/web_r.xml", null);
		this.copyNdkFile(tool, tmp, false, "/project/res/layout/web.xml", null);

		FS.mkdirSync(tmp + "/ndk/project/res/values");
		this.copyNdkFile(tool, tmp, true, "/project/res/values/strings.xml", [{ "key": "#APP_NAME#", "value": environment.NAME }]);
		this.copyNdkFile(tool, tmp, false, "/project/res/values/theme.xml", null);
		
		FS.mkdirSync(tmp + "/ndk/project/res/xml");
		this.copyNdkFile(tool, tmp, true, "/project/res/xml/kconfig.xml", null);

		// transform java sources
		FS.mkdirSync(tmp + "/ndk/project/src");
		var namespace = environment.NAMESPACE.split(".");
		for (let i = 0, c = namespace.length, path = tmp + "/ndk/project/src"; i < c; i++) {
			path += "/" + namespace[i];
			FS.mkdirSync(path);
		}
		var javaPath = "/project/src/" + namespace.join("/");
		var javaMapping = [
			{ "key": "com.kinoma.kinomaplay", "value": environment.NAMESPACE },
			{ "key": "Kinoma Play", "value": "Kinoma " + environment.NAME },
			{ "key": "\r\n", "value": "\n" } // prevent mixed line endings
		];
		this.copyJavaFile(tool, tmp, javaPath, "FskEditText.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "FskProperties.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "FskView.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "FskViewGL.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "FskCamera.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "IFskView.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "KinomaPlay.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "KinomaService.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "MediaCodecCore.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "Play2Android.java", javaMapping);
		this.copyJavaFile(tool, tmp, javaPath, "RemoteControlReceiver.java", javaMapping);

		FS.mkdirSync(tmp + "/ndk/project/jni");
		this.copyNdkFile(tool, tmp, true, "/project/jni/Android.mk", [{ "key": "#OBJECT_BASE#", "value": info.base }]);

		var kinomaMapping = [
			{ "key": "com_kinoma_kinomaplay", "value": info.base },
			{ "key": "com/kinoma/kinomaplay", "value": info.path }
		];
		FS.mkdirSync(tmp + "/ndk/project/jni/KinomaLibCommon");
		this.copyKinomaFile(tool, tmp, "/KinomaLibCommon/KinomaFiles.c", kinomaMapping);
		this.copyKinomaFile(tool, tmp, "/KinomaLibCommon/KinomaInterface.cpp", kinomaMapping);
		this.copyKinomaFile(tool, tmp, "/KinomaLibCommon/KinomaInterfaceLib.h", kinomaMapping);
		this.copyKinomaFile(tool, tmp, "/KinomaLibCommon/KinomaLib.c", kinomaMapping);
		
		FS.mkdirSync(tmp + "/ndk/project/jni/KinomaLibG");
		this.copyKinomaFile(tool, tmp, "/KinomaLibG/gingerbreadStuff.cpp", kinomaMapping);
		
		FS.mkdirSync(tmp + "/ndk/project/jni/libFsk");
		this.copyKinomaFile(tool, tmp, "/libFsk/mainHelper.c", kinomaMapping);

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

			APP_DIR: bin,
			
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
	PATH=$(NDK_DIR):$(PATH) ; export KPR_TMP_DIR=$(TMP_DIR) ; export NDK_PROJECT_PATH=$(NDK_PROJECT_PATH) ; cd $(NDK_PROJECT_PATH)/.. ; ndk-build clean ; ndk-build $(NDB_OPTIONS) V=1

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
