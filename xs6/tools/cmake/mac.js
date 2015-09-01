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
			KPR2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa kpr2js",
			XS2JS: path + "/xsr6 -a " +  path + "/modules/tools.xsa xs2js",
			XSC: path + "/xsc6",
			XSL: path + "/xsl6",

			APP_DIR: bin + ".app/Contents/MacOS",
			TMP_DIR: tmp,
			
			CMAKE_OSX_ARCHITECTURES: "i386 CACHE STRING \"Build architecture of MacOS\"",
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
ADD_EXECUTABLE(${application} MACOSX_BUNDLE \${SOURCES} \${FskPlatform_SOURCES} \${F_HOME}/kinoma/kpr/patches/main.m)
TARGET_LINK_LIBRARIES(${application} \${LIBRARIES} \${OBJECTS} -ObjC)
TARGET_INCLUDE_DIRECTORIES(${application} PUBLIC \${C_INCLUDES})
TARGET_COMPILE_DEFINITIONS(${application} PUBLIC \${C_DEFINITIONS})
TARGET_COMPILE_OPTIONS(${application} PUBLIC \${C_OPTIONS})

SET(MACOSX_BUNDLE_ICON_FILE "fsk.icns")
SET(MACOSX_BUNDLE_GUI_IDENTIFIER "${namespace}")

ADD_CUSTOM_COMMAND(
	TARGET ${application}
	POST_BUILD
	COMMAND \${CMAKE_COMMAND} -E copy \$<TARGET_FILE:${application}> \${APP_DIR}/fsk
	COMMAND \${CMAKE_COMMAND} -E copy \${APP_DIR}/FskManifest.xsa \$<TARGET_FILE_DIR:${application}>
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR}/modules \$<TARGET_FILE_DIR:${application}>/modules
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR}/program \$<TARGET_FILE_DIR:${application}>/program
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR}/../Resources \$<TARGET_FILE_DIR:${application}>/../Resources
	)

COPY(SOURCE \${ICNS} DESTINATION \${APP_DIR}/../Resources/fsk.icns)
COPY(SOURCE \${NIB} DESTINATION \${APP_DIR}/../Resources/English.lproj/fsk.nib)
COPY(SOURCE \${PLIST} DESTINATION \${APP_DIR}/../Info.plist)
FILE(WRITE \${APP_DIR}/../PkgInfo "APPLTINY")
`;
	return output;
	}
}

export default Manifest;
