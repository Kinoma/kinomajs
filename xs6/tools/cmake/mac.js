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
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var icns = parts.directory + "/mac/fsk.icns";
		var nib = parts.directory + "/mac/fsk.nib";
		var plist = parts.directory + "/mac/fsk.plist";
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		return {
			APP_NAME: tool.application,

			APP_DIR: `${tool.outputPath}/bin/${tool.platform}/$<CONFIGURATION>/${this.tree.application}.app/Contents/MacOS`,
			TMP_DIR: tmp,
			
			ICNS: FS.existsSync(icns) ? icns : "$(F_HOME)/build/mac/fsk.icns",
			NIB: FS.existsSync(nib) ? nib : "$(F_HOME)/build/mac/fsk.nib",
			PLIST: FS.existsSync(plist) ? plist : "$(F_HOME)/build/mac/fsk.plist",
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

copy(SOURCE "\${PLIST}" DESTINATION "\${TMP_DIR}/MacOSXBundleInfo.plist.in")

set(MACOSX_BUNDLE_ICON_FILE "fsk.icns")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "${namespace}")

add_custom_target(
	Assemble
	ALL
	COMMAND \${CMAKE_COMMAND} -E make_directory \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${RES_DIR}/ \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy $<TARGET_FILE:\${APP_NAME}> \${APP_DIR}/fsk
	COMMAND \${CMAKE_COMMAND} -E copy \${ICNS} \${APP_DIR}/../Resources/fsk.icns
	COMMAND \${CMAKE_COMMAND} -E copy \${PLIST} \${APP_DIR}/../Info.plist
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR}/../Resources \$<TARGET_FILE_DIR:${application}>/../Resources
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${NIB} \${APP_DIR}/../Resources/English.lproj/fsk.nib
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR} \$<TARGET_FILE_DIR:\${APP_NAME}>
	COMMAND \${CMAKE_COMMAND} -E echo "APPLTINY" > \${APP_DIR}/../PkgInfo
	DEPENDS ${application} FskManifest.xsa
	)
`;
	return output;
	}
}

export default Manifest;
