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
import PLIST from "plistGrammar";
import IOS from "shared/ios";

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
		this.ios = new IOS(tree);
	}
	getPlatformLanguages() {
		return ["ASM"];
	}
	generate(tool, tmp, bin) {
		this.ios.findIdentity(tool);
		this.ios.completeInfo();
		this.ios.addResources(tool);
		this.ios.info.CFBundleExecutable = this.tree.application;
		super.generate(tool, tmp, bin);
		FS.writeFileSync(tmp + "/Entitlements.plist", PLIST.stringify(this.ios.entitlements));
		FS.writeFileSync(tmp + "/Info.plist", PLIST.stringify(this.ios.info));
	}
	getPlatformVariables(tool, tmp, bin) {
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		return {
			APP_DIR: `${tool.outputPath}/bin/${tool.platform}/\${CMAKE_BUILD_TYPE}/${tool.application}/${this.tree.application}.app`,
			TMP_DIR: tmp,

			APP_IPA: `${bin}/\${CMAKE_BUILD_TYPE}/${this.tree.application}.ipa`,
			APP_IPA: `${tool.outputPath}/bin/${tool.platform}/\${CMAKE_BUILD_TYPE}/${tool.application}/${this.tree.application}.ipa`,
			MANIFEST: tool.manifestPath,
			PROVISION: this.ios.provisionPath,
		};
	}
	getTargetRules(tool, file, tmp, bin) {
		var output = `
add_executable(${this.tree.application} \${APP_TYPE} \${SOURCES} \${FskPlatform_SOURCES} \${F_HOME}/kinoma/kpr/patches/main.m)
target_link_libraries(${this.tree.application} \${LIBRARIES} \${OBJECTS} -ObjC)

set_target_properties(${this.tree.application}
	PROPERTIES
	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${this.ios.identityName}"
	MACOSX_BUNDLE_INFO_PLIST \${TMP_DIR}/Info.plist
	MACOSX_BUNDLE_GUI_IDENTIFIER ${this.ios.info.CFBundleIdentifier}
	)

add_custom_target(
	Assemble
	ALL
	COMMAND \${CMAKE_COMMAND} -E make_directory \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${RES_DIR}/ \${APP_DIR}
	`;
	if (tool.ide || tool.cmakeGenerator == "Xcode") {
		output += `COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR} \${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>-iphoneos/${this.tree.application}.app
	COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>-iphoneos/${this.tree.application}.app/${this.tree.application} \${APP_DIR}`;
	} else {
		output += `COMMAND \${CMAKE_COMMAND} -E copy  $<TARGET_FILE:${this.tree.application}> \${APP_DIR}
	COMMAND dsymutil $<TARGET_FILE:${this.tree.application}> -o $<TARGET_FILE:${this.tree.application}>.dSYM`;
	}
	output += `
	COMMAND \${CMAKE_COMMAND} -E copy \${TMP_DIR}/Info.plist \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy \${PROVISION} \${APP_DIR}/embedded.mobileprovision
	COMMAND codesign -f -v -s ${this.ios.identityHash} --entitlements \${TMP_DIR}/Entitlements.plist \${APP_DIR}
	COMMAND xcrun -sdk iphoneos PackageApplication \${APP_DIR} -o \${APP_IPA}
	DEPENDS ${this.tree.application} FskManifest.xsa
	VERBATIM
	)
`;
		return output;
	}
};

export default Manifest;
