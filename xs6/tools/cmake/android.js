import * as FS from "fs";
import * as CMAKE from "cmake";
import * as MAKE_ANDROID from "make/android";

function getToolDirectory(tool, path) {
	let toolPath = tool.execute("which " + path);
	let parts = tool.splitPath(toolPath);
	return parts.directory;
}

class Makefile extends CMAKE.Makefile {
	constructor(tree) {
		tree.cIncludes.push("${ANDROID_NDK}/platforms/android-${NDK_PLATFORM_VER}/arch-arm/usr/include");
		tree.cIncludes.push("${FREETYPE_DIR}/include");
		super(tree);
		this.objects = "$(" + this.name + "_OBJECTS)";
	}
	generateRules(tool, file, path) {
		file.line("INCLUDE_DIRECTORIES(${FREETYPE_DIR})");
		if (this.separate) {
			file.line("LINK_DIRECTORIES(${NDK_PROJECT_LIBRARIES})");
			file.line("ADD_LIBRARY(", this.name, " SHARED ${", this.name, "_SOURCES})");
			file.line("TARGET_LINK_LIBRARIES(", this.name, " -lFsk -landroid -lOpenSLES)");
			file.line("SET_TARGET_PROPERTIES(", this.name, " PROPERTIES SUFFIX .so LIBRARY_OUTPUT_DIRECTORY ${NDK_PROJECT_LIBRARIES})");
			file.line("ADD_DEPENDENCIES(", this.name, " ndk)");
			if (!tool.debug)
				file.line("ADD_CUSTOM_COMMAND(TARGET ", this.name, " POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:", this.name, ">)");
		} else {
			file.line("ADD_LIBRARY(", this.name, " OBJECT ${", this.name, "_SOURCES})");
			file.line("LIST(APPEND TARGET_OBJECTS $<TARGET_OBJECTS:", this.name, ">)");
			file.line("SET(TARGET_OBJECTS ${TARGET_OBJECTS} PARENT_SCOPE)");
			file.line("ADD_DEPENDENCIES(", this.name, " FskManifest.xsa)");
		}
	}
};

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	get Makefile() {
		return Makefile;
	}
	getPlatformLanguages() {
		return ["ASM", "ASM_WMMX"];
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
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		parts.name = "android";
		parts.extension = "";
		return {
			KPR_APPLICATION: tool.application,
			KPR_NAMESPACE: environment.NAMESPACE,

			KPR2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa kpr2js",
			XS2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa xs2js",
			XSC: path + "/xsc6",
			XSL: path + "/xsl6",


			FSK_JAVA_NAMESPACE: info.path,
			
			APP_DIR: tmp + "/jet",
			
			NAME: environment.NAME,
			NAMESPACE: environment.NAMESPACE,
			OBJECTBASE: environment.NAMESPACE.replace(".", "_"),
			VERSION: environment.VERSION,
			
			KPR_RESOURCE_PATH: tool.joinPath(parts),
			KPR_MAKE_PATH: tool.makePath,
			
			NDB_OPTIONS: tool.debug ? "NDK_DEBUG=1" : "",
		}
	}
	getTargetRules(tool) {
		return `
FILE(MAKE_DIRECTORY \${FREETYPE_DIR})

ADD_SUBDIRECTORY(\${F_HOME}/libraries/freetype/xs6 FreeType)

ADD_LIBRARY(fsk \${SOURCES} \${OBJECTS} \${TARGET_OBJECTS})
ADD_DEPENDENCIES(fsk FreeType)

SET(ANT_CONFIGURATION debug)
IF(EXISTS \$ENV{HOME}/.android.keystore.info)
	SET(ANT_CONFIGURATION release)
	FILE(READ \$ENV{HOME}/.android.keystore.info LOCAL_KEYSTORE)
	FILE(APPEND \${NDK_PROJECT_PATH}/local.properties \${LOCAL_KEYSTORE})
ENDIF()

ADD_CUSTOM_COMMAND(
	OUTPUT \${NDK_PROJECT_OBJECTS}/libfsk.a
	COMMAND \${CMAKE_COMMAND} -E copy \$<TARGET_FILE:fsk> \${NDK_PROJECT_OBJECTS}/libfsk.a
	DEPENDS fsk
	)
ADD_CUSTOM_TARGET(copy DEPENDS \${NDK_PROJECT_OBJECTS}/libfsk.a)

ADD_CUSTOM_COMMAND(
	OUTPUT \${NDK_PROJECT_LIBRARIES}/libFsk.so
	COMMAND KPR_TMP_DIR=\${TMP_DIR} NDK_PROJECT_PATH=\${NDK_PROJECT_PATH} ndk-build clean
	COMMAND SUPPORT_XS_DEBUG=\${SUPPORT_XS_DEBUG} KPR_TMP_DIR=\${TMP_DIR} NDK_PROJECT_PATH=\${NDK_PROJECT_PATH} ndk-build \${NDB_OPTIONS} V=1
	DEPENDS copy
	WORKING_DIRECTORY \${NDK_PROJECT_PATH}
	)
ADD_CUSTOM_TARGET(ndk DEPENDS \${NDK_PROJECT_LIBRARIES}/libFsk.so)

ADD_CUSTOM_COMMAND(
	OUTPUT \${NDK_PROJECT_PATH}/res/raw/kinoma.jet
	COMMAND \${CMAKE_COMMAND} -E make_directory \${NDK_PROJECT_PATH}/res/raw
	COMMAND \${CMAKE_COMMAND} -E remove \${NDK_PROJECT_PATH}/res/raw/kinoma.jet
	COMMAND zip -8qrn .jpg:.png:.m4a \${NDK_PROJECT_PATH}/res/raw/kinoma.jet "*"
	DEPENDS FskManifest.xsa
	WORKING_DIRECTORY \${APP_DIR}
	)
ADD_CUSTOM_TARGET(jet DEPENDS \${NDK_PROJECT_PATH}/res/raw/kinoma.jet)

ADD_CUSTOM_COMMAND(
	OUTPUT \${BIN_DIR}/\${KPR_APPLICATION}.apk
	COMMAND android update project -p .
	COMMAND ant -Dsdk.dir=\${ANDROID_SDK} \${ANT_CONFIGURATION}
	COMMAND \${CMAKE_COMMAND} -E copy \${NDK_PROJECT_BIN}/\${KPR_APPLICATION}-\${ANT_CONFIGURATION}.apk \${BIN_DIR}/\${KPR_APPLICATION}.apk
	DEPENDS ndk jet \${SEPARATE}
	WORKING_DIRECTORY \${NDK_PROJECT_PATH}
	)
ADD_CUSTOM_TARGET(apk ALL DEPENDS \${BIN_DIR}/\${KPR_APPLICATION}.apk)

ADD_CUSTOM_TARGET(message ALL
	COMMAND \${CMAKE_COMMAND} -E echo  "--------------------------------------------------------------------------------"
	COMMAND \${CMAKE_COMMAND} -E echo  "-   Install: adb -d install -r \${BIN_DIR}/\${KPR_APPLICATION}.apk"
	COMMAND \${CMAKE_COMMAND} -E echo  "- Uninstall: adb -d uninstall \${KPR_NAMESPACE}"
	COMMAND \${CMAKE_COMMAND} -E echo  "-     Start: adb shell am start -n \${KPR_NAMESPACE}/\${KPR_NAMESPACE}.KinomaPlay"
	COMMAND \${CMAKE_COMMAND} -E echo  "-      Stop: adb shell am force-stop \${KPR_NAMESPACE}"
	COMMAND \${CMAKE_COMMAND} -E echo  "-     Debug: cd \${NDK_PROJECT_PATH} \\; export NDK_PROJECT_PATH=\${NDK_PROJECT_PATH} \\; \${ANDROID_NDK}/ndk-gdb --verbose --force --start"
	COMMAND \${CMAKE_COMMAND} -E echo  "--------------------------------------------------------------------------------"
	DEPENDS apk
	)

`
	}
};

export default Manifest;
