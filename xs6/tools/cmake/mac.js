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

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	getGenerator() {
		return "Unix Makefiles";
	}
	getIDEGenerator() {
		return "Xcode";
	}
	openIDE(tool, path) {
		process.then("open", `${path}${tool.slash}fsk.xcodeproj`);
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var icns = parts.directory + "/mac/fsk.icns";
		var nib = parts.directory + "/mac/fsk.nib";
		var plist = parts.directory + "/mac/fsk.plist";
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		return {
			APP_NAME: tool.application,

			APP_DIR: `${tool.outputPath}/bin/${tool.platform}/\${CONFIG_TYPE}/${this.tree.application}.app/Contents/MacOS`,
			BUILD_APP_DIR: `\${TMP_DIR}/\${CMAKE_CFG_INTDIR}/${this.tree.application}.app/Contents/MacOS`,
			
			ICNS: FS.existsSync(icns) ? icns : "$(F_HOME)/build/mac/fsk.icns",
			NIB: FS.existsSync(nib) ? nib : "$(F_HOME)/build/mac/fsk.nib",
			// FskPlatform.mk
		};
	}
	getTargetRules(tool) {
		var application = this.tree.application;
		var namespace = this.tree.environment.NAMESPACE;
		if (!namespace)
			namespace = `com.marvell.kinoma.${application.toLowerCase()}`;

		return `
add_executable(${application} MACOSX_BUNDLE \${SOURCES} \${FskPlatform_SOURCES} \${F_HOME}/kinoma/kpr/patches/main.m)
target_link_libraries(${application} \${LIBRARIES} \${OBJECTS} -ObjC)
target_include_directories(${application} PUBLIC \${C_INCLUDES})
target_compile_definitions(${application} PUBLIC \${C_DEFINITIONS})
target_compile_options(${application} PUBLIC \${C_OPTIONS})

set(MACOSX_BUNDLE_INFO_STRING "Kinoma Simulator 2.0.0 Copyright © ${new Date().getFullYear()} Marvell Semiconductor, Inc.")
set(MACOSX_BUNDLE_ICON_FILE "fsk.icns")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "${namespace}")
set(MACOSX_BUNDLE_LONG_VERSION_STRING "Kinoma Simulator 2.0.0")
set(MACOSX_BUNDLE_BUNDLE_NAME "Kinoma Simulator")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "2.0.0")

add_custom_command(
	TARGET ${application}
	POST_BUILD
	COMMAND \${CMAKE_COMMAND} -E make_directory \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${RES_DIR}/ \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy \${ICNS} \${APP_DIR}/../Resources/fsk.icns
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${NIB} \${APP_DIR}/../Resources/English.lproj/fsk.nib
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR}/../Resources \${BUILD_APP_DIR}/../Resources
	COMMAND \${CMAKE_COMMAND} -E copy $<TARGET_FILE:${application}> \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR} \${BUILD_APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${application}>/../Info.plist \${APP_DIR}/../
	COMMAND \${CMAKE_COMMAND} -E echo "APPLTINY" > \${APP_DIR}/../PkgInfo
	)
`;
	return output;
	}
}

export default Manifest;
