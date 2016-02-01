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

export default class Android {
	constructor(tree) {
		this.tree = tree;
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
				var regex = new RegExp(map.key, "g");
				buffer = buffer.replace(regex, map.value);
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
			var regex = new RegExp(map.key, "g");
			buffer = buffer.replace(regex, map.value);
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
	copyNdkFile(tool, tmp, path, mapping) {
		var source = tool.homePath + "/build/android/inNDK/Play" + path;
		var destination = tmp + "/ndk" + path;
		this.copyFile(source, destination, mapping);
	}
	generateNdk(tool, tmp, bin) {
		var environment = this.tree.environment;
		var info = this.tree.info;

		// create directory structure
		FS.mkdirSync(tmp + "/ndk");
		this.copyNdkFile(tool, tmp, "/Application.mk", null);
		
		FS.mkdirSync(tmp + "/ndk/project");
		var features = "";
		for (let feature in info.features)
			features += '\t<uses-feature android:name="' + feature + '" android:required="' + info.features[feature] + '"/>\n';
		var permissions = "";
		for (let i = 0, c = info.permissions.length; i < c; i++)
			permissions += '\t<uses-permission android:name="android.permission.' + info.permissions[i] + '"/>\n';
		var versionCode = 1;
		if (environment.ANDROID_VERSION_CODE)
			versionCode = environment.ANDROID_VERSION_CODE;
		this.copyNdkFile(tool, tmp, "/project/AndroidManifest.xml", [
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
			{ "key": "#ANDROID_VERSION_CODE#", "value": versionCode },
		]);

		this.copyNdkFile(tool, tmp, "/project/build.xml", [{ "key": "#KPR_APPLICATION#", "value": info.name }]);
		this.copyNdkFile(tool, tmp, "/project/local.properties", null);
		this.copyNdkFile(tool, tmp, "/project/proguard.cfg", null);
		this.copyNdkFile(tool, tmp, "/project/project.properties", null);

		var localPropertiesPath = process.getenv("HOME") + "/.android.keystore.info";
		if (FS.existsSync(localPropertiesPath)) {
			var buffer = FS.readFileSync(tmp + "/ndk/project/local.properties");
			buffer += FS.readFileSync(localPropertiesPath);
			FS.writeFileSync(tmp + "/ndk/project/local.properties", buffer);
		}

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
		this.copyNdkFile(tool, tmp, "/project/res/layout/main.xml", [ { "key": "com.kinoma.kinomaplay", "value": environment.NAMESPACE } ]);
		this.copyNdkFile(tool, tmp, "/project/res/layout/splashscreen.xml", null);
		this.copyNdkFile(tool, tmp, "/project/res/layout/web_r.xml", null);
		this.copyNdkFile(tool, tmp, "/project/res/layout/web.xml", null);

		FS.mkdirSync(tmp + "/ndk/project/res/values");
		this.copyNdkFile(tool, tmp, "/project/res/values/strings.xml", [{ "key": "#APP_NAME#", "value": environment.NAME }]);
		this.copyNdkFile(tool, tmp, "/project/res/values/theme.xml", null);
		
//		FS.mkdirSync(tmp + "/ndk/project/res/xml");
//		this.copyNdkFile(tool, tmp, "/project/res/xml/kconfig.xml", null);

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
		this.copyNdkFile(tool, tmp, "/project/jni/Android.mk", [{ "key": "#OBJECT_BASE#", "value": info.base }]);

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
};
