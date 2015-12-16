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
import * as CMAKE from "cmake";
import Android from "shared/android";

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
		file.line("include_directories(${FREETYPE_DIR})");
		if (this.separate) {
			file.line("link_directories(${NDK_PROJECT_LIBRARIES})");
			file.line("add_library(", this.name, " SHARED ${", this.name, "_SOURCES})");
			file.line("target_link_libraries(", this.name, " -lFsk -landroid -lOpenSLES)");
			file.line("set_target_properties(", this.name, " PROPERTIES SUFFIX .so LIBRARY_OUTPUT_DIRECTORY ${NDK_PROJECT_LIBRARIES})");
			file.line("add_dependencies(", this.name, " ndk)");
			if (!tool.debug)
				file.line("add_custom_command(TARGET ", this.name, " POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:", this.name, ">)");
		} else {
			file.line("add_library(", this.name, " OBJECT ${", this.name, "_SOURCES})");
			file.line("list(APPEND TARGET_OBJECTS $<TARGET_OBJECTS:", this.name, ">)");
			file.line("set(TARGET_OBJECTS ${TARGET_OBJECTS} PARENT_SCOPE)");
			file.line("add_dependencies(", this.name, " FskManifest.xsa)");
		}
	}
};

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
		this.android = new Android(tree);
	}
	get Makefile() {
		return Makefile;
	}
	getPlatformLanguages() {
		return ["ASM", "ASM_WMMX"];
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
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		parts.name = "android";
		parts.extension = "";
		return {
			KPR_APPLICATION: tool.application,
			KPR_NAMESPACE: environment.NAMESPACE,

			FSK_JAVA_NAMESPACE: info.path,
			
			APP_DIR: "$(RES_DIR)",
			BIN_DIR: bin,
			
			NAME: environment.NAME,
			NAMESPACE: environment.NAMESPACE,
			OBJECTBASE: info.base,
			VERSION: environment.VERSION,

			ANT_CONFIGURATION: FS.existsSync(process.getenv("HOME") + "/.android.keystore.info") ? "release" : "debug",
			
			KPR_RESOURCE_PATH: tool.joinPath(parts),
			KPR_MAKE_PATH: tool.makePath,
			
			NDB_OPTIONS: tool.debug ? "NDK_DEBUG=1" : "",
		}
	}
	getTargetRules(tool) {
		if (this.tree.xsdebug.enabled || tool.xsdebug)
			var xsdebug = "1";
		else
			var xsdebug = "$<$<CONFIG:Debug>:1>$<$<CONFIG:Release>:0>";

		var application = tool.application;

		return `
file(MAKE_DIRECTORY \${FREETYPE_DIR})

add_subdirectory(\${F_HOME}/libraries/freetype/xs6 FreeType)

add_library(fsk \${SOURCES} \${OBJECTS} \${TARGET_OBJECTS})
add_dependencies(fsk FreeType)

add_custom_command(
	OUTPUT \${NDK_PROJECT_OBJECTS}/libfsk.a
	COMMAND \${CMAKE_COMMAND} -E copy \$<TARGET_FILE:fsk> \${NDK_PROJECT_OBJECTS}/libfsk.a
	DEPENDS fsk
	)
add_custom_target(copy DEPENDS \${NDK_PROJECT_OBJECTS}/libfsk.a)

add_custom_command(
	OUTPUT \${NDK_PROJECT_LIBRARIES}/libFsk.so
	COMMAND KPR_TMP_DIR="\${TMP_DIR}" NDK_PROJECT_PATH="\${NDK_PROJECT_PATH}" ndk-build clean
	COMMAND SUPPORT_XS_DEBUG=${xsdebug} KPR_TMP_DIR=\${TMP_DIR} NDK_PROJECT_PATH=\${NDK_PROJECT_PATH} NDK_TOOLCHAIN_VERSION=\${NDK_TOOLCHAIN_VERSION} ndk-build \${NDB_OPTIONS} V=1
	DEPENDS copy \${NDK_PROJECT_OBJECTS}/libfsk.a
	WORKING_DIRECTORY \${NDK_PROJECT_PATH}
	)
add_custom_target(ndk DEPENDS \${NDK_PROJECT_LIBRARIES}/libFsk.so)

add_custom_target(
	jet
	COMMAND \${CMAKE_COMMAND} -E make_directory \${NDK_PROJECT_PATH}/res/raw
	COMMAND \${CMAKE_COMMAND} -E remove \${NDK_PROJECT_PATH}/res/raw/kinoma.jet
	COMMAND zip -8qrn .jpg:.png:.m4a \${NDK_PROJECT_PATH}/res/raw/kinoma.jet "*"
	DEPENDS FskManifest.xsa
	WORKING_DIRECTORY \${APP_DIR}
	)

add_custom_command(
	OUTPUT \${NDK_PROJECT_BIN}/runtime-\${ANT_CONFIGURATION}.apk
	COMMAND android update project -p .
	COMMAND ant -Dsdk.dir=\${ANDROID_SDK} \${ANT_CONFIGURATION}
	DEPENDS ndk jet \${SEPARATE}
	WORKING_DIRECTORY \${NDK_PROJECT_PATH}
	)
add_custom_target(ant DEPENDS \${NDK_PROJECT_BIN}/runtime-\${ANT_CONFIGURATION}.apk)

add_custom_command(
	OUTPUT \${BIN_DIR}/${application}.apk
	COMMAND android update project -p .
	COMMAND \${CMAKE_COMMAND} -E copy \${NDK_PROJECT_BIN}/${application.toLowerCase()}-\${ANT_CONFIGURATION}.apk \${BIN_DIR}/${application}.apk
	DEPENDS ndk jet ant \${SEPARATE}
	WORKING_DIRECTORY \${NDK_PROJECT_PATH}
	)
add_custom_target(apk ALL DEPENDS \${BIN_DIR}/${application}.apk)

add_custom_target(message ALL
	COMMAND \${CMAKE_COMMAND} -E echo  "--------------------------------------------------------------------------------"
	COMMAND \${CMAKE_COMMAND} -E echo  "-   Install: adb -d install -r \${BIN_DIR}/${application}.apk"
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
